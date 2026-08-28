#pragma once

#include <filesystem>

#define ERROR_COL "\033[31m"
#define WARNING "\033[33m"
#define END "\033[0m"

namespace fs = std::filesystem;


inline std::vector<fs::path> get_files_by_extension(const fs::path& dir_path, const std::string& target_ext) {
    std::vector<fs::path> matching_files;

    if (fs::exists(dir_path) && fs::is_directory(dir_path)) {
        for (const auto& entry : fs::directory_iterator(dir_path)) {
            if (fs::is_regular_file(entry.status())) {
                if (entry.path().extension() == target_ext) {
                    matching_files.push_back(entry.path());
                }
            }
        }
    }

    return matching_files;
}