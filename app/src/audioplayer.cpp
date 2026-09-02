#include "../include/audioplayer.hpp"
#include "../include/logger.hpp"

#include <iostream>
#include <chrono>



AudioPlayer::~AudioPlayer() {
    busses.clear();
    //stop_preloader();
    BASS_Free();
}


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


bool AudioPlayer::load_song(const Song* new_song, bool verbose) {
    if(!new_song) return false;

    if(verbose) Logger::get_instance().log("[AudioPlayer] loading song '" + new_song->get_name() + "'...");

    std::vector<AudioTrack> tracks = new_song->get_tracks();

    bus_buffer.clear();
    bus_buffer.reserve(tracks.size());

    for(size_t i = 0; i < tracks.size(); i++)
        bus_buffer.push_back(AudioBus());

    for(size_t i = 0; i < bus_buffer.size(); i++) {
        if (!load_track(bus_buffer[i], tracks[i], verbose)) {
            if(verbose) Logger::get_instance().log_err("[AudioPlayer] unable to load song '" + new_song->get_name() + "'");
            bus_buffer.clear();
            return false;
        } else if(verbose) Logger::get_instance().log("[AudioPlayer] track '" + tracks[i].name + "' loaded succesfully");
    }

    if(verbose) Logger::get_instance().log("[AudioPlayer] song '" + new_song->get_name() + "' loaded succesfully");
    return true;
}

bool AudioPlayer::load_playlist(Playlist* playlist) {
    if(!playlist) return false;

    Logger::get_instance().log("[AudioPlayer] loading playlist '" + playlist->get_name() + "'...");

    queued_songs.clear();

    for(const auto& path : playlist->get_songs()) {
        Song* song = Song::create_from_file(path);

        if(!song) {
            Logger::get_instance().log_err("[AudioPlayer] unable to load playlist '" + playlist->get_name() + "'");
            return false;
        }
        queue_song(song);
    }

    Logger::get_instance().log("[AudioPlayer] playlist '" + playlist->get_name() + "' loaded succesfully");
    return true;
}

void AudioPlayer::swap_buffers() {
    //std::lock_guard<std::mutex> lock(mtx);
    busses = std::move(bus_buffer);
    bus_buffer.clear();
}


void AudioPlayer::list_devices() {
    BASS_DEVICEINFO info;
    Logger::get_instance().log("Index | Name                          | State");
    Logger::get_instance().log("---------------------------------------------");
    for (int i = 0; BASS_GetDeviceInfo(i, &info); i++) {
        bool enabled   = (info.flags & BASS_DEVICE_ENABLED) != 0;
        bool isDefault = (info.flags & BASS_DEVICE_DEFAULT) != 0;
 
        Logger::get_instance().log(std::to_string(i) + "      | " + info.name
                  + (enabled ? " [active]" : " [unavailable]")
                  + (isDefault ? " (default)" : ""));
    }
}

void AudioPlayer::list_song_queue() {
    std::cout << "\n=== SONG QUEUE ===\n";

    for(size_t i = 0; i < queued_songs.size(); i++)
        std::cout << i + 1 << ". " << queued_songs[i]->get_name() << "\n";
    
    std::cout << "\n";
}

bool AudioPlayer::init_device(int device, bool verbose) {
    if (!BASS_Init(device, 44100, 0, nullptr, nullptr)) {
        int err = BASS_ErrorGetCode();
        if (err == BASS_ERROR_ALREADY) {
            return true;
        }
        if(verbose) Logger::get_instance().log_err("[AudioPlayer] unable to initialize device " + std::to_string(device)
                   + " : " + std::to_string(err));
        return false;
    }
    return true;
}

int AudioPlayer::create_bus() {
    busses.emplace_back();
    int idx = static_cast<int>(busses.size()) - 1;
    
    busses[idx] = AudioBus();

    return idx;
}

void AudioPlayer::clear_busses() {
    busses.clear();
    bus_buffer.clear();
}

bool AudioPlayer::load_track(AudioBus& bus, const AudioTrack& track, bool verbose) {
    if (!init_device(track.device_id, verbose)) return false;

    return bus.load(track, verbose);
}

bool AudioPlayer::route_channel(int bus_id, int new_device) {
    if (!is_valid_bus(bus_id)) return false;

    return busses[bus_id].route_to_device(new_device);
}

void AudioPlayer::play(int song_id) {
    if (!is_valid_song_id(song_id)) {
        Logger::get_instance().log_err("[AudioPlayer] queued song with id " + std::to_string(song_id) + " doesn't exist");
        return;
    }

    playing_song = song_id;

    Song* current_song = queued_songs[playing_song];

    //{
        //std::lock_guard<std::mutex> lock(mtx);
        //if(bus_buffer.empty())
            if(!load_song(current_song)) return;
    //}

    swap_buffers();

    for(auto& bus : busses) {
        bus.prepare_for_play();
    }

    for(auto& bus : busses) {
        bus.play(should_restart);
    }

    Logger::get_instance().log("[AudioPlayer] now playing '" + current_song->get_name() + "'\n");

    //Song* next = get_next_song_looped();

    //if(next)
    //    preload_next_song(next, 0.5f);
}

void AudioPlayer::play_current() {
    play(playing_song);
}


// Deprecated
void AudioPlayer::play_queue() {
    if(queued_songs.empty()) {
        Logger::get_instance().log_warn("[AudioPlayer] there are no songs in the queue");
        return;
    }

    for(size_t i = 0; i < queued_songs.size(); i++) {
        play(static_cast<int>(i));

        if (i == queued_songs.size() - 1) return;

        char cmd;
        std::cout << "\nNext song : " << queued_songs[i + 1]->get_name() << " (y/n) > ";
        std::cin >> cmd;
        std::cin.ignore();

        if(cmd != 'y') return;
    }
}

void AudioPlayer::pause() {
    paused = true;
    for(size_t i = 0; i < bus_count(); i++) {
        busses[i].pause();
    }
}

void AudioPlayer::resume() {
    paused = false;
    for(auto& bus : busses) {
        bus.prepare_for_play();
    }

    for(auto& bus : busses) {
        bus.play(false);
    }
}

void AudioPlayer::stop() {
    for(size_t i = 0; i < bus_count(); i++) {
        busses[i].stop();
    }

    //stop_preloader();
    playing_song = -1;
}

Song* AudioPlayer::get_next_song() const {
    if (playing_song < static_cast<int>(queued_songs.size()) - 1)
        return queued_songs[playing_song + 1];
    return nullptr;
}

Song* AudioPlayer::get_next_song_looped() const {
    if (playing_song < static_cast<int>(queued_songs.size()) - 1)
        return queued_songs[playing_song + 1];
    return queued_songs[0];
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
    //std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::pair<float, float>> ret;
    
    ret.reserve(busses.size());

    for(const auto& b : busses)
        ret.push_back(b.get_stereo_audio_levels());
    
    return ret;
}


/* SONG PRELOADER */

/*
void AudioPlayer::stop_preloader() {
    stop_flag = true;

    {
        std::lock_guard<std::mutex> lock(mtx);
        bus_buffer.clear();
    }

    if(worker.joinable())
        worker.join();
}

void AudioPlayer::preload_next_song(const Song* next_song, float progress_ratio) {
    if(busses.empty()) return;

    stop_preloader();
    stop_flag = false;
    bus_buffer.reserve(next_song->get_tracks_count());
    worker = std::thread(&AudioPlayer::threaded_load, this, get_longest_bus_id(), next_song, progress_ratio);
}

void AudioPlayer::threaded_load(int longest_bus_id, const Song* next_song, float progress_ratio) {
    while(!stop_flag) {
        std::pair<double, double> duration_info = get_bus_playback_info(longest_bus_id);

        if((duration_info.first / duration_info.second) >= progress_ratio) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                load_song(next_song, false);
            }
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}
*/