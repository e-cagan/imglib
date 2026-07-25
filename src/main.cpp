#include <iostream>
#include <imglib/image.hpp>
#include <imglib/ppm.hpp>

int main()
{
    // Save
    imglib::Image img(2, 2, 3);
    img(0, 0, 0) = 255;
    imglib::save_ppm("test.ppm", img);

    // Load back
    auto loaded = imglib::load_ppm("test.ppm");
    if (!loaded) { std::cout << "load failed\n"; return 1; }

    // Validate and compare the images
    std::cout << "width: " << loaded->width() << "\n";                            // 2
    std::cout << "pixel(0,0,0): " << static_cast<int>((*loaded)(0,0,0)) << "\n";  // 255
    std::cout << "pixel(1,1,0): " << static_cast<int>((*loaded)(0,1,0)) << "\n";  // 0
    
    return 0;
}