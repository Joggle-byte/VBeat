#pragma once

#include <string>


enum class TrackState {
    INVALID_TRACK,
    OK,
    FILE_MISSING,
    FILE_UNREADABLE,
};


struct AudioTrack {
    std::string name;
    std::string file_path;
    float volume = 1.0;
    int channel_index = 0;

    TrackState state;
};