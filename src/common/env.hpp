#pragma once

#include <filesystem>
#include <optional>


namespace shar::env {

std::optional<std::filesystem::path> shar_dir();
std::filesystem::path config_path();
std::filesystem::path logs_dir();

}