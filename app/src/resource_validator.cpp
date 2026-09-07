#include "../include/resource_validator.hpp"
#include "../include/logger.hpp"
#include "../include/bass.h"
#include "../include/audiobus.hpp"

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

        if(!is_device_available(AudioBus::find_device_by_driver(t.device_id))) {
            if(!fs::exists(fs::path(t.file_path))) {
                res->set_track_state(i, TrackState::FILE_AND_DEVICE_MISSING);
            } else res->set_track_state(i, TrackState::DEVICE_MISSING);

            continue;
        }

        if(!fs::exists(fs::path(t.file_path))) {
                res->set_track_state(i, TrackState::FILE_MISSING);
        } else res->set_track_state(i, TrackState::OK);
    }

    bool corrupted_flag = true;

    for(size_t i = 0; i < res->get_tracks_count(); i++) {
        const AudioTrack& t = res->get_track(i);

        switch(res->get_track_state(i)) {
            case TrackState::OK:
                res->set_state(SongState::OK);
                corrupted_flag = false;
                break;
            case TrackState::DEVICE_MISSING:
                res->set_state(SongState::DEGRADED);
                Logger::get_instance().log_warn("[SongValidator] (in song: " + res->get_name() + ") Track '" + t.name + "' is routed to an unavailable device: " + t.device_id);
                break;
            case TrackState::FILE_MISSING:
                res->set_state(SongState::DEGRADED);
                Logger::get_instance().log_warn("[SongValidator] (in song: " + res->get_name() + ") Track '" + t.name + "' points to an unavailable audio file: " + t.file_path);
                break;
            case TrackState::FILE_AND_DEVICE_MISSING:
                Logger::get_instance().log_warn("[SongValidator] (in song: " + res->get_name() + ") Track '" + t.name + "' is routed to an unavailable device: " + t.device_id);
                Logger::get_instance().log_warn("[SongValidator] (in song: " + res->get_name() + ") Track '" + t.name + "' points to an unavailable audio file: " + t.file_path);
                res->set_state(SongState::DEGRADED);
                break;
        }
    }

    if(corrupted_flag) {
        res->set_state(SongState::CORRUPTED);
        Logger::get_instance().log_err("[SongValidator] song '" + res->get_name() + "' is corrupted");
    }
}

