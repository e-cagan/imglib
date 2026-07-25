#include <fstream>
#include <imglib/image.hpp>
#include <imglib/ppm.hpp>

namespace imglib
{
    bool save_ppm(const std::string& path, const Image& img)
    {
        // Open file in binary mode
        std::ofstream file(path, std::ios::binary);
        if (!file)
        {
            return false;
        }

        // Read width, height and channels of image
        std::size_t width = img.width();
        std::size_t height = img.height();
        uint8_t channels = img.channels();
        if (channels != 3)
        {
            return false;
        }

        // Write ppm prefixed file
        file << "P6\n" << width << " " << height << "\n" << 255 << "\n";

        for (size_t y = 0; y < height; y++)
        {
            for (size_t x = 0; x < width; x++)
            {
                for (uint8_t c = 0; c < channels; c++)
                {
                    // Write pixel information to the file
                    file.put(static_cast<char>(img(x, y, c)));
                }
            }
        }
        
        return true;

    }

    std::optional<Image> load_ppm(const std::string& path)
    {
        return std::nullopt;
    }
}