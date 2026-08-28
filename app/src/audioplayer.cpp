#include "../include/audioplayer.hpp"
#include <iostream>
#include <chrono>
#include <thread>


AudioPlayer::~AudioPlayer() {
    busses.clear();
    BASS_Free();
}


void AudioPlayer::queue_song(Song* song) {
    queued_songs.push_back(song);
}

void AudioPlayer::clear_queue() {
    playing_song = -1;
    queued_songs.clear();
}

Song* AudioPlayer::get_queued_song(int song_id) const {
    if(!is_valid_song_id(song_id)) return nullptr;

    return queued_songs[song_id];
}


bool AudioPlayer::load_song(Song* new_song) {
    if(!new_song) return false;

    std::cout << "[AudioPlayer] loading song '" << new_song->get_name() << "'...\n";

    //clear_busses();

    std::vector<AudioTrack> tracks = new_song->get_tracks();

    clear_busses();

    for(size_t i = 0; i < tracks.size(); i++)
        create_bus();

    for(size_t i = 0; i < bus_count(); i++) {
        if (!load_track(i, tracks[i])) {
            std::cerr << "[AudioPlayer] unable to load song '" << new_song->get_name() << "'\n";
            return false;
        } else std::cout << "[AudioPlayer] track '" << tracks[i].name << "' loaded succesfully\n";
    }

    std::cout << "[AudioPlayer] song '" << new_song->get_name() << "' loaded succesfully\n";
    return true;
}

bool AudioPlayer::load_playlist(Playlist* playlist) {
    if(!playlist) return false;

    std::cout << "[AudioPlayer] loading playlist '" << playlist->get_name() << "'...\n";

    queued_songs.clear();

    for(const auto& path : playlist->get_songs()) {
        Song* song = Song::create_from_file(path);

        if(!song) {
            std::cerr << "[AudioPlayer] unable to load playlist '" << playlist->get_name() << "'\n";
            return false;
        }
        queue_song(song);
    }

    std::cout << "[AudioPlayer] playlist '" << playlist->get_name() << "' loaded succesfully\n";
    return true;
}


void AudioPlayer::list_devices() {
    BASS_DEVICEINFO info;
    std::cout << "Index | Name                          | State\n";
    std::cout << "---------------------------------------------\n";
    for (int i = 0; BASS_GetDeviceInfo(i, &info); i++) {
        bool enabled   = (info.flags & BASS_DEVICE_ENABLED) != 0;
        bool isDefault = (info.flags & BASS_DEVICE_DEFAULT) != 0;
 
        std::cout << i << "      | " << info.name
                  << (enabled ? " [active]" : " [unavailable]")
                  << (isDefault ? " (default)" : "")
                  << "\n";
    }
}

void AudioPlayer::list_song_queue() {
    std::cout << "\n=== SONG QUEUE ===\n";

    for(size_t i = 0; i < queued_songs.size(); i++)
        std::cout << i + 1 << ". " << queued_songs[i]->get_name() << "\n";
    
    std::cout << "\n";
}

bool AudioPlayer::init_device(int device) {
    if (!BASS_Init(device, 44100, 0, nullptr, nullptr)) {
        int err = BASS_ErrorGetCode();
        if (err == BASS_ERROR_ALREADY) {
            return true;
        }
        std::cerr << "[AudioPlayer] unable to initialize device " << device
                   << " : " << err << "\n";
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
}

bool AudioPlayer::load_track(int bus_id, const AudioTrack& track) {
    if (!is_valid_bus(bus_id)) return false;

    if (!init_device(track.device_id)) return false;

    return busses[bus_id].load(track);
}

bool AudioPlayer::route_channel(int bus_id, int new_device) {
    if (!is_valid_bus(bus_id)) return false;

    return busses[bus_id].route_to_device(new_device);
}

void AudioPlayer::play(int song_id, bool restart) {
    if (!is_valid_song_id(song_id)) {
        std::cerr << "[AudioPlayer] queued song with id " << song_id << " doesn't exist\n";
        return;
    }

    playing_song = song_id;

    Song* current_song = queued_songs[playing_song];

    if(!load_song(current_song)) return;

    for(auto& bus : busses) {
        bus.prepare_for_play();
    }

    for(auto& bus : busses) {
        bus.play(restart);
    }

    std::cout << "\n[AudioPlayer] now playing '" << current_song->get_name() << "'\n";

    process_playing();
}

void AudioPlayer::play_queue() {
    if(queued_songs.empty()) {
        std::cerr << "[AudioPlayer] there are no songs in the queue\n";
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

    playing_song = -1;
}

bool AudioPlayer::is_playing() {
    bool ret = false;
    for(size_t i = 0; i < bus_count(); i++) {
        ret |= busses[i].is_playing();
    }
    return ret || paused;
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


void AudioPlayer::process_playing() {
    while(is_playing()) {
        std::string command;
        std::getline(std::cin, command);
        
        if(command == "s") stop();
        else if(command == "p") pause();
        else if(command == "r") resume();
    }
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));
}