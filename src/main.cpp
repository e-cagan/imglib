#include <iostream>
#include <imglib/image.hpp>
#include <imglib/ppm.hpp>
#include <imglib/filters.hpp>

int main()
{
    imglib::Image img(1, 3, 3);

    // Assign pixel values
    // red
    img(0,0,0)=255;
    img(0,0,1)=0;
    img(0,0,2)=0;
    // green
    img(0,1,0)=0;
    img(0,1,1)=255;
    img(0,1,2)=0;
    // white
    img(0,2,0)=255;
    img(0,2,1)=255;
    img(0,2,2)=255;

    // Grayscale the image
    auto gray = imglib::to_grayscale(img);
    std::cout << static_cast<int>(gray(0,0,0)) << "\n";  // ~76
    std::cout << static_cast<int>(gray(0,1,0)) << "\n";  // ~150
    std::cout << static_cast<int>(gray(0,2,0)) << "\n";  // 255

    // Check the channel amount
    std::cout << "Num Channels: " << int(gray.channels()) << "\n";
    
    return 0;
}