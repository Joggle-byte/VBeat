#pragma once

#include "bass.h"
#include "audiotrack.hpp"
#include <string>


class AudioBus {
public:

    AudioBus() = default;
    ~AudioBus();

    bool load(const AudioTrack& new_track);

    bool route_to_device(int new_device);

    void play(bool restart);
    void prepare_for_play();
    void pause();
    void stop();

    void set_volume(float vol);

    bool is_playing();
    bool is_paused();
    bool is_stopped();

    void free();

    std::pair<double, double> get_playback_duration_info() const;

    std::pair<float, float> get_stereo_audio_levels() const;

    HSTREAM get_handle() const { return handle; }
    int get_device() const { return track.device_id; }
    const std::string& get_file_path() const { return file_path; }
    AudioTrack get_track() const { return track; }

private:
    
    AudioTrack track;

    HSTREAM handle = 0;
    std::string file_path;
};