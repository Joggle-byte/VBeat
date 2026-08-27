#include "../include/audiobus.hpp"
#include <iostream>


AudioBus::~AudioBus() {
    free();
}

bool AudioBus::load(const AudioTrack& new_track) {
    if (!BASS_SetDevice(new_track.device_id)) {
        std::cerr << "[AudioBus " << new_track.name << "] unable to set default device (" << new_track.device_id << ") : " << BASS_ErrorGetCode() << "\n";
        
        return false;
    }

    free();

    handle = BASS_StreamCreateFile(FALSE, new_track.file_path.c_str(), 0, 0, 0);

    if (!handle) {
        std::cerr << "[AudioBus " << new_track.name << "] unable to load file '" << new_track.file_path << "' : " << BASS_ErrorGetCode() << "\n";
    
        return false;
    }

    this->track.device_id = new_track.device_id;
    this->track.file_path = new_track.file_path;
    this->track.name = new_track.name;
    this->track.volume = new_track.volume;

    set_volume(new_track.volume);
    
    return true;
}

bool AudioBus::route_to_device(int new_device) {
    if (!handle) {
        std::cerr << "[AudioBus " << track.name << "] there is no audio stream to route\n";

        return false;
    }

    if (!BASS_ChannelSetDevice(handle, new_device)) {
        std::cerr << "[AudioBus " << track.name << "] unable to route bus to device "
                   << new_device << " : " << BASS_ErrorGetCode() << "\n";
        
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


