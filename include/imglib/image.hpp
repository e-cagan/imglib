#pragma once
#include <vector>
#include <cstdint>

namespace imglib
{
    class Image
    {
    private:
        std::size_t width_;
        std::size_t height_;
        uint8_t channels_;
        std::vector<uint8_t> pixels_;
    public:
        Image(std::size_t width, std::size_t height, uint8_t channels)
            : width_(width), height_(height), channels_(channels),
            pixels_(width * height * channels, 0)
        {}

        uint8_t& operator()(std::size_t x, std::size_t y, uint8_t c)
        {
            std::size_t index = (y * width_ * channels_) + (x * channels_) + c;
            return pixels_[index];
        }

        uint8_t operator()(std::size_t x, std::size_t y, uint8_t c) const
        {
            std::size_t index = (y * width_ * channels_) + (x * channels_) + c;
            return pixels_[index];
        }

        std::size_t width() const
        {
            return width_;
        }

        std::size_t height() const
        {
            return height_;
        }

        uint8_t channels() const
        {
            return channels_;
        }
    };
}