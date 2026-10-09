#include <iostream>
#include <vector>
#include <cmath>

#include "../include/audiobus.hpp"
#include "../include/logger.hpp"


/*

    TO-DO:

    * Route each song to 1 single device
    * Allow each track to be routed to a different output on that device
      (so that the Audioplayer only has 1 mixer that mixes all the tracks together)
    * Introduce a song_t type that only stores the song's id in the songbank,
      and grant the songbank complete control and authority over song objects
    
    * Implement a playlistbank class

*/

bool AudioBus::generate_channel_map(int channel_idx, const int max_channels) {
    // if the selected channel idx is not available
    // it defaults to 0

    if(!handle) return false;

    if(max_channels <= 1) return false;

    BASS_CHANNELINFO info;
    BASS_ChannelGetInfo(handle, &info);

    if(channel_idx < 0) {
        Logger::get_instance().log_warn("[AudioBus " + track.name + "] invalid channel index : " + std::to_string(channel_idx) + ".\n        Audio channel will be set to default (0). Audio device channel count : " + std::to_string(max_channels));
        channel_idx = 0;
    }

    int effective_channels = std::floor(max_channels / 2);

    if(channel_idx >= effective_channels) {
        Logger::get_instance().log_warn("[AudioBus " + track.name + "] invalid channel index : " + std::to_string(channel_idx) + ".\n        Audio channel will be set to default (0). Audio device channel count : " + std::to_string(max_channels));
        channel_idx = 0;
    }

    std::vector<int> matrix(max_channels);

    for(int i = 0; i < max_channels; i++) {
        matrix[i] = -1;
    }

    matrix[channel_idx * 2] = 0;
    matrix[channel_idx * 2 + 1] = (info.chans >= 2) ? 1 : 0; // mono -> 0; stereo -> 1

    track.channel_index = channel_idx;

    device_channel_map = matrix;

    return true;
}


AudioBus::~AudioBus() {
    free();
}

bool AudioBus::load(const AudioTrack& new_track, bool verbose) {
    this->track = new_track;

    free();

    BASS_INFO info;
    BASS_GetInfo(&info);

    handle = BASS_StreamCreateFile(FALSE, new_track.file_path.c_str(), 0, 0, BASS_STREAM_DECODE);

    if (!handle) {
        if(verbose) Logger::get_instance().log_err("[AudioBus " + new_track.name + "] unable to load file '" + new_track.file_path + "' : " + std::to_string(BASS_ErrorGetCode()));
        
        handle = 0;
        return false;
    }

    set_volume(new_track.volume);

    if(!generate_channel_map(track.channel_index, info.speakers)) {
        return false;
    }

    return true;
}

bool AudioBus::route_bus() {
    if(!handle) return false;

    if(!BASS_Mixer_ChannelSetMap(handle, device_channel_map.data())) {
        return false;
    }

    return true;
}


bool AudioBus::is_playing() {
    if (!handle) return false;
    return BASS_Mixer_ChannelIsActive(handle) == BASS_ACTIVE_PLAYING;
}

bool AudioBus::is_stopped() {
    if (!handle) return false;
    return BASS_Mixer_ChannelIsActive(handle) == BASS_ACTIVE_STOPPED;
}


void AudioBus::set_volume(float vol) {
    if (handle) {
        BASS_ChannelSetAttribute(handle, BASS_ATTRIB_VOL, vol);
        track.volume = vol;   
    }
}

void AudioBus::free() {
    if (handle) {
        BASS_StreamFree(handle);
        handle = 0;
    }
}

bool AudioBus::set_playback_pos(double seconds) {
    if (!handle) return false;
    if (seconds < 0.0) seconds = 0.0;

    QWORD len = BASS_ChannelGetLength(handle, BASS_POS_BYTE);
    QWORD pos = BASS_ChannelSeconds2Bytes(handle, seconds);

    if (pos > len) pos = len;

    if (!BASS_Mixer_ChannelSetPosition(handle, pos, BASS_POS_BYTE | BASS_POS_MIXER_RESET)) {
        return false;
    }

    return true;
}

std::pair<double, double> AudioBus::get_playback_duration_info() const {
    QWORD pos = BASS_Mixer_ChannelGetPosition(handle, BASS_POS_BYTE);
    QWORD len = BASS_ChannelGetLength(handle, BASS_POS_BYTE);

    double posSec = BASS_ChannelBytes2Seconds(handle, pos);
    double lenSec = BASS_ChannelBytes2Seconds(handle, len);
    return std::pair<double, double>(posSec, lenSec);
}

std::pair<float, float> AudioBus::get_stereo_audio_levels() const {
    float levels[2] = {0.0f, 0.0f};

    if (BASS_Mixer_ChannelGetLevelEx(handle, levels, 0.02f, BASS_LEVEL_STEREO)) {
        return {levels[0], levels[1]};
    }
    
    return {0.0f, 0.0f};
}
