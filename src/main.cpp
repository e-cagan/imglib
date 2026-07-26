#include <iostream>
#include <imglib/image.hpp>
#include <imglib/ppm.hpp>
#include <imglib/filters.hpp>

int main()
{
    // Vertical edge: left half black (0), right half white (255), single channel.
    // Columns 0,1 -> 0   |   columns 2,3 -> 255
    imglib::Image img(4, 3, 1);

    for (std::size_t y = 0; y < 3; y++)
    {
        img(2, y, 0) = 255;
        img(3, y, 0) = 255;
        // columns 0 and 1 stay 0 (constructor zero-fills)
    }

    imglib::Image edges = imglib::sobel(img);

    // Print one full row of the edge map.
    // Expect: high response at the 0->255 boundary (around column 1-2),
    // low (~0) in the flat regions (columns 0 and 3).
    std::cout << "sobel row y=1: ";
    for (std::size_t x = 0; x < 4; x++)
    {
        std::cout << static_cast<int>(edges(x, 1, 0)) << " ";
    }
    std::cout << "\n";

    return 0;
}