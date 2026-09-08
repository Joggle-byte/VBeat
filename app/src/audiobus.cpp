#include <iostream>

#include "../include/audiobus.hpp"
#include "../include/logger.hpp"


int AudioBus::find_device_by_driver(const std::string& target_driver) {
    if(target_driver.empty()) return -1;

    BASS_DEVICEINFO info;
    for (int a = 0; BASS_GetDeviceInfo(a, &info); a++)
    {
        if (info.driver && target_driver == info.driver)
            return a;
    }
    return -1; // not found
}

int AudioBus::init_device(const std::string& id, bool verbose) {
    int idx = find_device_by_driver(id);

    if(idx == -1) {
        if(verbose) Logger::get_instance().log_err("[AudioBus] (" + track.name + ") device " + id + " was not found");
        return -1;
    }

    if (!BASS_Init(idx, 44100, 0, nullptr, nullptr)) {
        int err = BASS_ErrorGetCode();
        if (err == BASS_ERROR_ALREADY) {
            return idx;
        }
        if(verbose) Logger::get_instance().log_err("[AudioBus] (" + track.name + ") unable to initialize device " + id
                   + " : " + std::to_string(err));
        return -1;
    }
    return idx;
}


AudioBus::~AudioBus() {
    free();
}

bool AudioBus::load(const AudioTrack& new_track, bool verbose) {
    this->track = new_track;

    int device_id = init_device(new_track.device_id, verbose);

    if(device_id == -1) return false;

    if (!BASS_SetDevice(device_id)) {
        if(verbose) Logger::get_instance().log_err("[AudioBus " + new_track.name + "] unable to set device (" + std::to_string(device_id) + ") : " + std::to_string(BASS_ErrorGetCode()));
        handle = 0;
        return false;
    }

    free();

    handle = BASS_StreamCreateFile(FALSE, new_track.file_path.c_str(), 0, 0, BASS_ASYNCFILE);

    if (!handle) {
        if(verbose) Logger::get_instance().log_err("[AudioBus " + new_track.name + "] unable to load file '" + new_track.file_path + "' : " + std::to_string(BASS_ErrorGetCode()));
    
        return false;
    }

    set_volume(new_track.volume);
    
    return true;
}

bool AudioBus::route_to_device(const std::string& new_device) {
    int device_idx = find_device_by_driver(new_device);

    if(device_idx == -1) {
        Logger::get_instance().log_err("[AudioBus " + track.name + "] device " + new_device + " not found");
        return false;
    }

    if (!handle) {
        Logger::get_instance().log_err("[AudioBus " + track.name + "] there is no audio stream to route");

        return false;
    }

    if (!BASS_ChannelSetDevice(handle, device_idx)) {
        Logger::get_instance().log_err("[AudioBus " + track.name + "] unable to route bus to device "
                   + std::to_string(device_idx) + " : " + std::to_string(BASS_ErrorGetCode()));
        
        return false;
    }
 
    track.device_id = new_device;
    return true;
}

bool AudioBus::is_playing() {
    if (!handle) return false;
    return BASS_ChannelIsActive(handle) == BASS_ACTIVE_PLAYING;
}

bool AudioBus::is_paused() {
    if (!handle) return false;
    return BASS_ChannelIsActive(handle) == BASS_ACTIVE_PAUSED;
}

bool AudioBus::is_stopped() {
    if (!handle) return false;
    return BASS_ChannelIsActive(handle) == BASS_ACTIVE_STOPPED;
}


void AudioBus::play(bool restart) {
    if (handle) BASS_ChannelPlay(handle, (restart) ? TRUE : FALSE);
}

void AudioBus::prepare_for_play() {
    if (handle) BASS_ChannelUpdate(handle, 0);
}

void AudioBus::pause() {
    if (handle) BASS_ChannelPause(handle);
}

void AudioBus::stop() {
    if (handle) BASS_ChannelStop(handle);
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

std::pair<double, double> AudioBus::get_playback_duration_info() const {
    QWORD pos = BASS_ChannelGetPosition(handle, BASS_POS_BYTE);
    QWORD len = BASS_ChannelGetLength(handle, BASS_POS_BYTE);

    double posSec = BASS_ChannelBytes2Seconds(handle, pos);
    double lenSec = BASS_ChannelBytes2Seconds(handle, len);
    return std::pair<double, double>(posSec, lenSec);
}

std::pair<float, float> AudioBus::get_stereo_audio_levels() const {
    float levels[2] = {0.0f, 0.0f};

    if (BASS_ChannelGetLevelEx(handle, levels, 0.02f, BASS_LEVEL_STEREO))
        return {levels[0], levels[1]};
    
    return {0.0f, 0.0f};
}
