#include <iostream>
#include <vector>
#include <cstdint>
#include <imglib/image.hpp>

int main()
{
    imglib::Image img(2, 2, 3);

    std::cout << "First pixel test: " << static_cast<int>(img(0, 0, 0)) << std::endl;

    img(1, 1, 0) = 200;

    std::cout << "Update pixel test: " << static_cast<int>(img(1, 1, 0)) << std::endl;
    
    return 0;
}