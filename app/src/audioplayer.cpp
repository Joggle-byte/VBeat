#include "../include/audioplayer.hpp"
#include "../include/logger.hpp"

#include <iostream>
#include <chrono>
#include <algorithm>
#include <cmath>



AudioPlayer::~AudioPlayer() {
    busses.clear();
    
    if(mixer) {
        BASS_StreamFree(mixer);
    }

    release_all_devices();
}


int AudioPlayer::get_device_index_by_id(const std::string& target_driver) {
    if(target_driver.empty()) return -1;

    BASS_DEVICEINFO info;
    for (int a = 0; BASS_GetDeviceInfo(a, &info); a++)
    {
        if (info.driver && target_driver == info.driver)
            return a;
    }
    return -1; // not found
}

std::string AudioPlayer::get_device_name_from_id(const std::string& id) {
    BASS_DEVICEINFO info;
    for (int a = 0; BASS_GetDeviceInfo(a, &info); a++)
    {
        if (info.driver && id == std::string(info.driver))
            return info.name;
    }
    return "";
}

size_t AudioPlayer::get_device_audio_out_count(const std::string& id) {
    int idx = acquire_device(id);

    if(!BASS_SetDevice(idx)) {
        return 0;
    }

    BASS_INFO info;
    BASS_GetInfo(&info);

    return std::floor(info.speakers / 2);
}

int AudioPlayer::acquire_device(const std::string& id) {
    int idx = get_device_index_by_id(id);
    
    if (idx == -1) {
        Logger::get_instance().log_err("[AudioPlayer] device " + id + " not found");
        return -1;
    }

    if (active_devices.find(id) != active_devices.end()) {
        return idx;
    }

    if (!BASS_Init(idx, 44100, 0, nullptr, nullptr)) {
        int err = BASS_ErrorGetCode();
        if (err != BASS_ERROR_ALREADY) {
            Logger::get_instance().log_err("[AudioPlayer] cannot initialize audio device " + id);
            return -1;
        }
    }

    active_devices[id] = idx;
    
    return idx;
}

void AudioPlayer::release_all_devices() {
    for (const auto& [id, idx] : active_devices) {
        BASS_SetDevice(idx);
        BASS_Free();
    }
    active_devices.clear();
}

std::vector<Device> AudioPlayer::get_devices() {
    std::vector<Device> ret;
    BASS_DEVICEINFO info;
    
    for (int i = 0; BASS_GetDeviceInfo(i, &info); i++) {
        if(!info.driver) continue;

        ret.push_back({
            info.name,
            info.driver,
            i
        });
    }

    return ret;
}

void AudioPlayer::list_devices() {
    BASS_DEVICEINFO info;
    Logger::get_instance().log("AUDIO DEVICES LIST:");
    Logger::get_instance().log("---------------------------------------------");
    for (int i = 0; BASS_GetDeviceInfo(i, &info); i++) {
        bool enabled   = (info.flags & BASS_DEVICE_ENABLED) != 0;
        bool isDefault = (info.flags & BASS_DEVICE_DEFAULT) != 0;
        
        if(!info.driver) continue;

        Logger::get_instance().log(std::to_string(i) + "      " + std::string(info.driver) + "      " + info.name + "      "
                  + (enabled ? " [active]" : " [unavailable]")
                  + (isDefault ? " (default)" : ""));
    }
    Logger::get_instance().log("\n");
}


// =========================== SONG QUEUE HANDLING ===========================

void AudioPlayer::queue_song(Song* song) {
    queued_songs.push_back(song);
}

void AudioPlayer::clear_queue() {
    playing_song = -1;
    queued_songs.clear();
    clear_busses();
}

void AudioPlayer::set_queue(std::vector<Song*> new_queue) {
    queued_songs = new_queue;
    playing_song = 0;

    clear_busses();
}

Song* AudioPlayer::get_queued_song(int song_id) const {
    if(!is_valid_song_id(song_id)) return nullptr;

    return queued_songs[song_id];
}

Song* AudioPlayer::get_current_song() const {
    if(playing_song >= 0 && playing_song < static_cast<int>(get_queue_size())) return queued_songs[playing_song];
    return nullptr;
}

// ===========================================================================


// ================================= MARKERS =================================

std::vector<Marker> AudioPlayer::get_current_marker_layout() {
    Song* current_song = get_current_song();

    if(!current_song) return {};
    if(busses.empty()) return {};
    
    auto markers = current_song->get_markers();

    if(markers.empty()) return {};

    auto song_duration_info = get_bus_playback_info(get_longest_bus_id());

    std::vector<Marker> ret;

    ret.reserve(markers.size());

    for(const auto& i : markers) {
        if(i.second.get_timestamp() > 0.0f && i.second.get_timestamp() <= song_duration_info.second)
            ret.push_back(i.second);
    }

    std::sort(ret.begin(), ret.end(), [](const Marker& a, const Marker& b) {
        return a.get_timestamp() < b.get_timestamp();
    });

    return ret;
}

// ===========================================================================


/*
std::vector<std::string> AudioPlayer::get_device_names() const {
    std::vector<std::string> ret;
    BASS_DEVICEINFO info;
    
    for (int i = 0; BASS_GetDeviceInfo(i, &info); i++) {
        if(!info.driver) continue;

        ret.push_back(std::string(info.name));
    }

    return ret;
} 

std::vector<std::string> AudioPlayer::get_device_ids() const {
    std::vector<std::string> ret;
    BASS_DEVICEINFO info;
    
    for (int i = 0; BASS_GetDeviceInfo(i, &info); i++) {
        if(info.driver) ret.push_back(std::string(info.driver));
    }

    return ret;
} 

std::string AudioPlayer::get_device_id_from_index(int index) const {
    BASS_DEVICEINFO info;

    if(!BASS_GetDeviceInfo(index, &info)) return "";
    if(!info.driver) return "";

    return std::string(info.driver);
}

std::string AudioPlayer::get_device_name_from_id(const std::string& id) const {
    BASS_DEVICEINFO info;
    
    for (int i = 0; BASS_GetDeviceInfo(i, &info); i++) {
        if(info.driver) {
            if(std::string(info.driver) == id) return info.name;
        }
    }

    return "";
}
*/


// ============================== SONG LOADING ==============================

bool AudioPlayer::load_song(Song* new_song, bool verbose) {
    if(!new_song) return false;

    if(verbose) Logger::get_instance().log("[AudioPlayer] loading song '" + new_song->get_name() + "'...");


    // Device init
    int device_idx = acquire_device(new_song->get_device_id());

    if(device_idx == -1) return false;

    if (!BASS_SetDevice(device_idx)) {
        if(verbose) Logger::get_instance().log_err("[AudioPlayer] unable to set device (" + std::to_string(device_idx) + ") in song " + new_song->get_name() + " : " + std::to_string(BASS_ErrorGetCode()));
        return false;
    }

    // Mixer init
    if(mixer) {
        BASS_StreamFree(mixer);
    }

    BASS_INFO info;
    BASS_GetInfo(&info);
    mixer = BASS_Mixer_StreamCreate(info.freq, info.speakers, BASS_MIXER_NONSTOP);

    if(!mixer) {
        if(verbose) Logger::get_instance().log_err("[AudioPlayer] unable to initialize track mixer for song " + new_song->get_name() + " : " + std::to_string(BASS_ErrorGetCode()));
        
        mixer = 0;
        return false;
    }

    
    // Tracks init
    std::vector<AudioTrack> tracks = new_song->get_tracks();

    clear_busses();
    busses.reserve(tracks.size());

    for(size_t i = 0; i < tracks.size(); i++)
        busses.push_back(AudioBus());

    int loaded_tracks = 0;
    
    for(size_t i = 0; i < busses.size(); i++) {
        if (!busses[i].load(tracks[i], verbose)) {
            if(verbose) Logger::get_instance().log_err("[AudioPlayer] unable to load track '" + tracks[i].name + "'");
            continue;

        } else {

            if(!BASS_Mixer_StreamAddChannel(mixer, busses[i].get_handle(), BASS_MIXER_CHAN_MATRIX | BASS_MIXER_CHAN_BUFFER)) {
                Logger::get_instance().log_err("[AudioPlayer] unable to initialize track bus '" + tracks[i].name + "'");
                continue;
            }

            if(!busses[i].route_bus()) {
                Logger::get_instance().log_err("[AudioPlayer] unable to route track bus '" + tracks[i].name + "' to channel " + std::to_string(tracks[i].channel_index));
                continue;
            }

            if(verbose) Logger::get_instance().log("[AudioPlayer] track '" + tracks[i].name + "' loaded succesfully");

            loaded_tracks++;
        }
    }

    return loaded_tracks > 0;
}

int AudioPlayer::create_bus() {
    busses.emplace_back();
    int idx = static_cast<int>(busses.size()) - 1;
    
    busses[idx] = AudioBus();

    return idx;
}

void AudioPlayer::clear_busses() {
    busses.clear();
}

// ===========================================================================


// ============================== SONG PLAYBACK ==============================

void AudioPlayer::play(int song_id) {
    if (!is_valid_song_id(song_id)) {
        Logger::get_instance().log_err("[AudioPlayer] queued song with id " + std::to_string(song_id) + " doesn't exist");
        return;
    }

    playing_song = song_id;

    Song* current_song = queued_songs[playing_song];

    if(!load_song(current_song)) return;

    BASS_ChannelPlay(mixer, FALSE);

    Logger::get_instance().log("[AudioPlayer] now playing '" + current_song->get_name() + "'\n");
}

void AudioPlayer::play_current() {
    play(playing_song);
}


void AudioPlayer::pause() {
    paused = true;
    BASS_ChannelPause(mixer);
}

void AudioPlayer::resume() {
    paused = false;
    BASS_ChannelPlay(mixer, FALSE);
}

void AudioPlayer::stop() {
    BASS_ChannelStop(mixer);
}

void AudioPlayer::set_playback_pos(double seconds) {
    if (!mixer) return;

    bool was_playing = (BASS_ChannelIsActive(mixer) == BASS_ACTIVE_PLAYING);

    if (was_playing) {
        BASS_ChannelPause(mixer);
    }

    for (auto& bus : busses) {
        bus.set_playback_pos(seconds);
    }

    if (was_playing) {
        BASS_ChannelPlay(mixer, FALSE);
    }
}

// ===========================================================================


int AudioPlayer::select_next_song() {
    int current_song = playing_song;
    int queue_size = static_cast<int>(get_queue_size());
    int max_depth = queue_size;

    do {
        if(current_song == queue_size - 1) {
            playing_song = current_song;
            return playing_song;
        };

        current_song++;
        max_depth--;

    } while(queued_songs[current_song]->is_corrupted() && max_depth > 0);

    playing_song = current_song;
    return playing_song;
}

int AudioPlayer::select_next_song_looped() {
    int current_song = playing_song;
    int queue_size = static_cast<int>(get_queue_size());
    int max_depth = queue_size;

    do {
        if(current_song == queue_size - 1) {
            current_song = 0;
        } else current_song++;
        max_depth--;

    } while(queued_songs[current_song]->is_corrupted() && max_depth > 0);

    playing_song = current_song;
    return playing_song;
}

Song* AudioPlayer::get_next_song() const {
    int current_song = playing_song;
    int queue_size = static_cast<int>(get_queue_size());
    int max_depth = queue_size;

    do {
        if(current_song == queue_size - 1) return nullptr;

        current_song++;
        max_depth--;

    } while(queued_songs[current_song]->is_corrupted() && max_depth > 0);

    return queued_songs[current_song];
}

Song* AudioPlayer::get_next_song_looped() const {
    int current_song = playing_song;
    int queue_size = static_cast<int>(get_queue_size());
    int max_depth = queue_size;

    do {
        if(current_song == queue_size - 1) {
            current_song = 0;
        } else current_song++;
        max_depth--;

    } while(queued_songs[current_song]->is_corrupted() && max_depth > 0);

    return queued_songs[current_song];
}


bool AudioPlayer::is_playing() {
    bool ret = false;
    for(size_t i = 0; i < bus_count(); i++) {
        ret |= busses[i].is_playing();
    }
    return ret;
}

void AudioPlayer::set_volume(int bus_id, float vol) {
    if (is_valid_bus(bus_id)) busses[bus_id].set_volume(vol);
}

bool AudioPlayer::is_valid_bus(int id) const {
    return id >= 0 && id < static_cast<int>(busses.size());
}

bool AudioPlayer::is_valid_song_id(int id) const {
    return id >= 0 && id < static_cast<int>(queued_songs.size());
}

std::pair<double, double> AudioPlayer::get_bus_playback_info(int bus_id) {
    if(is_valid_bus(bus_id)) return busses[bus_id].get_playback_duration_info();
    return {0.0f, 0.0f};
}

int AudioPlayer::get_longest_bus_id() {
    if(busses.empty()) return -1;
    if(busses.size() == 1) return 0;

    double greater = busses[0].get_playback_duration_info().second;
    int id = 0;

    for(size_t i = 1; i < busses.size(); i++) {
        double curr_len = busses[i].get_playback_duration_info().second;
        if(curr_len > greater) {
            greater = curr_len;
            id = i;
        };
    }

    return id;
}

AudioBus* AudioPlayer::get_bus(int bus_id) {
    if(is_valid_bus(bus_id)) return &busses[bus_id];
    return nullptr;
}

std::vector<std::pair<float, float>> AudioPlayer::get_bus_levels() {
    std::vector<std::pair<float, float>> ret;
    
    ret.reserve(busses.size());

    for(const auto& b : busses)
        ret.push_back(b.get_stereo_audio_levels());
    
    return ret;
}