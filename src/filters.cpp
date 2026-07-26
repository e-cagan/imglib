#include <cmath>
#include <algorithm>
#include <imglib/image.hpp>
#include <imglib/filters.hpp>

namespace imglib
{
    Image to_grayscale(const Image& src)
    {
        std::size_t width = src.width();
        std::size_t height = src.height();
        uint8_t channels = src.channels();
        if (channels != 3)
        {
            return src;
        }

        Image result(width, height, 1);
        
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

    Image box_blur(const Image& src, int kernel_size)
    {
        if (kernel_size % 2 == 0 || kernel_size <= 0)
        {
            return src;
        }
        int radius = kernel_size / 2;

        std::size_t width = src.width();
        std::size_t height = src.height();
        uint8_t channels = src.channels();

        Image result(width, height, channels);

        for (size_t y = 0; y < height; y++)
        {
            for (size_t x = 0; x < width; x++)
            {
                for (uint8_t c = 0; c < channels; c++)
                {
                    float total = 0;
                    int counter = 0;

                    // Iterate trough radiuses (dY, dX)
                    for (int dy = -radius; dy <= radius; dy++)
                    {
                        for (int dx = -radius; dx <= radius; dx++)
                        {
                            // Calculate neighbours
                            int nx = static_cast<int>(x) + dx;   // int, could be negative
                            int ny = static_cast<int>(y) + dy;

                            // Clamp the possible negative values
                            int clamped_x = std::clamp(nx, 0, static_cast<int>(width) - 1);
                            int clamped_y = std::clamp(ny, 0, static_cast<int>(height) - 1);

                            total += src(size_t(clamped_x), size_t(clamped_y), c);
                            counter++;
                        }
                    }

                    // Assign averaged pixels
                    result(x, y, c) = static_cast<uint8_t>(std::round(total / counter));
                }
            }
        }

        return result;
    }

    Image sobel(const Image& src)
    {
        std::size_t width = src.width();
        std::size_t height = src.height();
        uint8_t channels = src.channels();
        if (channels != 1)
        {
            return src;
        }

        // Define sobel kernels
        int sobel_x[3][3] = {
            {-1, 0, 1},
            {-2, 0, 2},
            {-1, 0, 1}
        };
        int sobel_y[3][3] = {
            {-1, -2, -1},
            {0, 0, 0},
            {1, 2, 1}
        };

        Image result(width, height, 1);

        for (std::size_t y = 0; y < height; y++)
        {
            for (std::size_t x = 0; x < width; x++)
            {
                // Gradients
                int gx = 0;
                int gy = 0;

                for (int dy = -1; dy <= 1; dy++)
                {
                    for (int dx = -1; dx <= 1; dx++)
                    {
                        // Calculate neighbours
                        int nx = static_cast<int>(x) + dx;   // int, could be negative
                        int ny = static_cast<int>(y) + dy;

                        // Clamp the possible negative values
                        int clamped_x = std::clamp(nx, 0, static_cast<int>(width) - 1);
                        int clamped_y = std::clamp(ny, 0, static_cast<int>(height) - 1);
                        int clamped_value = src(size_t(clamped_x), size_t(clamped_y), 0);

                        // Calculate the gradients
                        gx += clamped_value * sobel_x[dy + 1][dx + 1];
                        gy += clamped_value * sobel_y[dy + 1][dx + 1];
                    }
                }

                // Calculate the magnitude and clamp it
                double magnitude = std::sqrt(gx * gx + gy * gy);
                result(x, y, 0) = static_cast<uint8_t>(std::clamp(magnitude, 0.0, 255.0));
            }
        }

        return result;
    }
}