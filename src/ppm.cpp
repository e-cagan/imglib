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

        // Write ppm prefixed file for 3 and 1 channels
        if (channels == 3)
        {
            file << "P6\n" << width << " " << height << "\n" << 255 << "\n";
        }
        else if (channels == 1)
        {
            file << "P5\n" << width << " " << height << "\n" << 255 << "\n";
        }
        else
        {
            return false;
        }

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
        // Open file in binary mode
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            return std::nullopt;
        }

        // Read the magic number and validate
        std::string magic;
        file >> magic;
        if (magic != "P6" && magic != "P5")
        {
            return std::nullopt;
        }

        // Read the width, height and maxval
        std::size_t width; 
        std::size_t height; 
        int maxval;
        file >> width >> height >> maxval;
        if (!file || width == 0 || height == 0)
        {
            return std::nullopt;
        } 
        if (maxval != 255)
        {
            return std::nullopt;
        }
        file.ignore(1);     // Skip the last '\n'
        
        // Construct the image based on channels
        uint8_t channels = (magic == "P6") ? 3 : 1;
        Image result(width, height, channels);

        for (size_t y = 0; y < height; y++)
        {
            for (size_t x = 0; x < width; x++)
            {
                for (uint8_t c = 0; c < channels; c++)
                {
                    // Write pixel information to the file
                    int byte = file.get();
                    result(x, y, c) = static_cast<uint8_t>(byte);
                }
            }
        }

        return result;
    }
}