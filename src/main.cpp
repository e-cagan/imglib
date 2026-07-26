#include <iostream>
#include <imglib/image.hpp>
#include <imglib/ppm.hpp>
#include <imglib/filters.hpp>

int main()
{
    imglib::Image img(3, 3, 3);

    // Assign pixel values for box blur
    // black except mid (red) pixel
    img(1,1,0)=255;

    // Apply box blur
    auto result = imglib::box_blur(img, 3);
    std::cout << "result(1, 1, 0): " << static_cast<int>(result(1, 1, 0)) << "\n";    // ~28
    
    return 0;
}