#pragma once

#include <string>


enum class TrackState {
    OK,
    DEVICE_MISSING,
    FILE_MISSING,
    FILE_UNREADABLE,
    FILE_AND_DEVICE_MISSING
};


struct AudioTrack {
    std::string name;
    std::string file_path;
    float volume;
    std::string device_id;

    TrackState state;
};