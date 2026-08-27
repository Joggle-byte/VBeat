#pragma once

#include <string>


struct AudioTrack {
    std::string name;
    std::string file_path;
    float volume;
    int device_id;
};