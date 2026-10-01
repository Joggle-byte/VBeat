#include "../include/resource_validator.hpp"
#include "../include/logger.hpp"
#include "../include/bass.h"
#include "../include/audioplayer.hpp"

#include <filesystem>

namespace fs = std::filesystem;


bool ResourceValidator::is_device_available(int deviceIndex) {
    if(deviceIndex < 0) return false;

    BASS_DEVICEINFO info;
    if (!BASS_GetDeviceInfo(static_cast<DWORD>(deviceIndex), &info)) {
        return false;
    }

    return (info.flags & BASS_DEVICE_ENABLED) != 0;
}

void ResourceValidator::validate(Song* res) {
    for(size_t i = 0; i < res->get_tracks_count(); i++) {
        const AudioTrack& t = res->get_track(i);

        if(!fs::exists(fs::path(t.file_path))) {
            res->set_track_state(i, TrackState::FILE_MISSING);
        } else res->set_track_state(i, TrackState::OK);
    }

    bool corrupted_flag = true;
    bool degraded_flag = false;

    for(size_t i = 0; i < res->get_tracks_count(); i++) {
        const AudioTrack& t = res->get_track(i);

        switch(res->get_track_state(i)) {
            case TrackState::OK:
                corrupted_flag = false;
                break;
            case TrackState::FILE_MISSING:
                degraded_flag = true;
                Logger::get_instance().log_warn("[SongValidator] (in song: " + res->get_name() + ") Track '" + t.name + "' points to an unavailable audio file: " + t.file_path);
                break;
        }
    }

    if(!is_device_available(AudioPlayer::get_device_index_by_id(res->get_device_id()))) {
        Logger::get_instance().log_warn("[SongValidator] (in song: " + res->get_name() + ") the song is routed to an unavailable device: " + res->get_device_id());
        
        corrupted_flag = true;
    }

    if(corrupted_flag) {
        res->set_state(SongState::CORRUPTED);
        Logger::get_instance().log_err("[SongValidator] song '" + res->get_name() + "' is corrupted");
        return;
    }

    if(degraded_flag) {
        res->set_state(SongState::DEGRADED);
        Logger::get_instance().log_err("[SongValidator] song '" + res->get_name() + "' is degraded");
    } else res->set_state(SongState::OK);
}

