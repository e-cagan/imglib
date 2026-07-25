#pragma once
#include <imglib/image.hpp>
#include <optional>
#include <string>

namespace imglib
{
    bool save_ppm(const std::string& path, const Image& img);
    std::optional<Image> load_ppm(const std::string& path);
}