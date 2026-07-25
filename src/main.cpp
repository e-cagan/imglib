#include <iostream>
#include <imglib/image.hpp>
#include <imglib/ppm.hpp>

int main()
{
    imglib::Image img(2, 2, 3);
    img(0, 0, 0) = 255;   // Top left pixel is red
    imglib::save_ppm("test.ppm", img);
    
    return 0;
}