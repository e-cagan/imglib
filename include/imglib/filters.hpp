#pragma once
#include <imglib/image.hpp>

namespace imglib
{
    Image to_grayscale(const Image& src);
    Image box_blur(const Image& src, int kernel_size);
}