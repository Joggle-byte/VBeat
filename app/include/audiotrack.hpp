#pragma once

#include <string>


enum class TrackState {
    INVALID_TRACK,
    OK,
    FILE_MISSING,
    FILE_UNREADABLE,
};


struct AudioTrack {
    float volume = 1.0;
    int channel_index = 0;
    TrackState state;
    
    std::string name;
    std::string file_path;

    bool muted = false;

    AudioTrack() {}

    AudioTrack(const std::string& _name, const std::string& _file_path, float _vol, int _ch_idx, bool _muted) :
        volume(_vol),
        channel_index(_ch_idx),
        name(_name),
        file_path(_file_path),
        muted(_muted) {}
};