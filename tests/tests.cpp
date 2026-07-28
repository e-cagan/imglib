#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <imglib/image.hpp>
#include <imglib/ppm.hpp>
#include <imglib/filters.hpp>

TEST_CASE("Image basics")
{
    imglib::Image img(4, 3, 3);

    SUBCASE("dimensions") { CHECK(img.width() == 4); CHECK(img.height() == 3); CHECK(img.channels() == 3); }
    SUBCASE("zero-initialized") { CHECK(img(0,0,0) == 0); }
    SUBCASE("pixel write/read") { img(1,1,0) = 200; CHECK(img(1,1,0) == 200); }
}

TEST_CASE("load_ppm rejects invalid input")
{
    SUBCASE("nonexistent file")
    {
        auto result = imglib::load_ppm("this_file_does_not_exist.ppm");
        CHECK(result == std::nullopt);
    }

    SUBCASE("invalid magic")
    {
        {
            std::ofstream f("bad_magic.ppm", std::ios::binary);
            f << "P3\n2 2\n255\n";   // magic is P3 -> invalid
        }

        auto result = imglib::load_ppm("bad_magic.ppm");
        CHECK(result == std::nullopt);
    }

    SUBCASE("zero-dimensioned file")
    {
        {
            std::ofstream f("zerodim.ppm", std::ios::binary);
            f << "P6\n0 0\n255\n";
        }
        
        auto result = imglib::load_ppm("zerodim.ppm");
        CHECK(result == std::nullopt);
    }

    SUBCASE("invalid maxval")
    {
        {
            std::ofstream f("invalid_maxval.ppm", std::ios::binary);
            f << "P6\n2 2\n370\n";
        }
        
        auto result = imglib::load_ppm("invalid_maxval.ppm");
        CHECK(result == std::nullopt);
    }

    SUBCASE("missing pixel info")
    {
        {
            std::ofstream f("missing.ppm", std::ios::binary);
            f << "P6\n2 2\n255\n";
        }
        
        auto result = imglib::load_ppm("missing.ppm");
        CHECK(result == std::nullopt);
    }

    SUBCASE("P6 round-trip")
    {
        // 1. Create original image and modify some pixels
        imglib::Image original(2, 2, 3);
        original(0,0,0) = 255;
        original(1,1,1) = 128;

        // 2. Save the ppm to disk
        imglib::save_ppm("rt6.ppm", original);

        // 3. Read back with load ppm
        auto loaded = imglib::load_ppm("rt6.ppm");

        // Is loaded? Are the values same?
        REQUIRE(loaded);   // Be sure that the ppm is loaded, in advance
        CHECK(loaded->width() == 2);
        CHECK(loaded->channels() == 3);
        CHECK((*loaded)(0,0,0) == 255);
        CHECK((*loaded)(1,1,1) == 128);
    }

    SUBCASE("P5 round-trip")
    {
        // 1. Create original image and modify some pixels
        imglib::Image original(2, 2, 1);
        original(0,0,0) = 255;
        original(1,1,0) = 128;

        // 2. Save the pgm to disk
        imglib::save_ppm("rt5.pgm", original);

        // 3. Read back with load ppm
        auto loaded = imglib::load_ppm("rt5.pgm");

        // Is loaded? Are the values same?
        REQUIRE(loaded);   // Be sure that the ppm is loaded, in advance
        CHECK(loaded->width() == 2);
        CHECK(loaded->channels() == 1);
        CHECK((*loaded)(0,0,0) == 255);
        CHECK((*loaded)(1,1,0) == 128);
    }
}

TEST_CASE("Filtre happy paths")
{
    SUBCASE("grayscale: red->76, green->150, white->255")
    {
        imglib::Image src(3, 1, 3);
        
        // Red
        src(0,0,0) = 255; src(0,0,1) = 0; src(0,0,2) = 0;
        // Green
        src(1,0,0) = 0; src(1,0,1) = 255; src(1,0,2) = 0;
        // White
        src(2,0,0) = 255; src(2,0,1) = 255; src(2,0,2) = 255;

        auto gray = imglib::to_grayscale(src);
        
        CHECK(gray.channels() == 1);
        CHECK(gray(0,0,0) == 76);
        CHECK(gray(1,0,0) == 150);
        CHECK(gray(2,0,0) == 255);
    }

    SUBCASE("box_blur: Single point->28 spreading")
    {
        imglib::Image src(3, 3, 1);
        src(1,1,0) = 255; // Center pixel 255, around pixels 0
        
        auto blurred = imglib::box_blur(src, 3);
        
        // 255 / 9 = 28.33 => with std::round, 28
        CHECK(blurred(0,0,0) == 28);
        CHECK(blurred(1,1,0) == 28);
        CHECK(blurred(2,2,0) == 28);
    }

    SUBCASE("sobel: vertical edge->0 255 255 0")
    {
        imglib::Image src(4, 3, 1);
        for (std::size_t y = 0; y < 3; y++)
        {
            src(2, y, 0) = 255;
            src(3, y, 0) = 255;
        }
        
        auto edges = imglib::sobel(src);
        
        CHECK(edges(0, 1, 0) == 0);
        CHECK(edges(1, 1, 0) == 255);
        CHECK(edges(2, 1, 0) == 255);
        CHECK(edges(3, 1, 0) == 0);
    }
}

TEST_CASE("Filtre edge cases")
{
    SUBCASE("grayscale on single channel -> returns src")
    {
        imglib::Image src(2, 2, 1);
        src(0,0,0) = 42;
        auto gray = imglib::to_grayscale(src);
        
        CHECK(gray.channels() == 1);
        CHECK(gray(0,0,0) == 42);
    }

    SUBCASE("box_blur with even kernels -> returns src")
    {
        imglib::Image src(2, 2, 3);
        src(1,1,1) = 84;
        auto blurred = imglib::box_blur(src, 2);
        
        CHECK(blurred(1,1,1) == 84); 
    }

    SUBCASE("sobel on 3 channels -> returns src")
    {
        imglib::Image src(3, 3, 3);
        src(2,2,2) = 111;
        auto edges = imglib::sobel(src);
        
        CHECK(edges.channels() == 3);
        CHECK(edges(2,2,2) == 111);
    }

    SUBCASE("box_blur on 1x1 image -> should be same")
    {
        imglib::Image src(1, 1, 1);
        src(0,0,0) = 150;
        auto blurred = imglib::box_blur(src, 3);
        
        CHECK(blurred(0,0,0) == 150);
    }
}