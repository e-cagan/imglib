#include <cmath>
#include <imglib/image.hpp>
#include <imglib/filters.hpp>

namespace imglib
{
    imglib::Image to_grayscale(const imglib::Image& src)
    {
        std::size_t width = src.width();
        std::size_t height = src.height();
        uint8_t channels = src.channels();
        if (channels != 3)
        {
            return src;
        }

        imglib::Image result(width, height, 1);
        
        for (std::size_t y = 0; y < height; y++)
        {
            for (std::size_t x = 0; x < width; x++)
            {
                // Seperate R, G, B pixels
                uint8_t R = src(x, y, 0);
                uint8_t G = src(x, y, 1);
                uint8_t B = src(x, y, 2);

                // Apply luma calculation formula
                float gray = 0.299f * R + 0.587f * G + 0.114f * B;
                result(x, y, 0) = static_cast<uint8_t>(std::round(gray));
            }
        }

        return result;
    }
}