#pragma once

#include "bass.h"
#include "bassmix.h"
#include "audiotrack.hpp"

#include <string>


class AudioBus {
public:

    AudioBus() = default;
    ~AudioBus();

    bool load(const AudioTrack& new_track, bool verbose = true);

    bool route_bus();

    void set_volume(float vol);

    bool is_playing();

    bool is_stopped();

    void free();

    bool set_playback_pos(double pos);

    std::pair<double, double> get_playback_duration_info() const;

    std::pair<float, float> get_stereo_audio_levels() const;

    HSTREAM get_handle() const { return handle; }

    int get_channel_index() const { return track.channel_index; }

    AudioTrack get_track() const { return track; }

private:
    AudioTrack track;

    HSTREAM handle = 0;

    std::vector<int> device_channel_map;

    bool has_finished_playing = false;

    bool generate_channel_map(const int channel_idx, const int max_channels);
};