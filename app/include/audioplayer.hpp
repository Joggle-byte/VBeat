#pragma once

#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "audiobus.hpp"
#include "song.hpp"
#include "playlist.hpp"


class AudioPlayer {
public:

    AudioPlayer() {}
    ~AudioPlayer();

    void list_devices();
    void list_song_queue();

    void queue_song(Song* song);
    void set_queue(std::vector<Song*> new_queue);

    void clear_queue();

    bool load_playlist(Playlist* playlist);

    void play(int song_id);
    void play_current();
    void play_queue();

    void pause();
    void stop();
    void resume();

    bool is_playing();

    bool is_paused() const { return paused; }

    size_t bus_count() const { return busses.size(); }

    AudioBus* get_bus(int bus_id);

    Song* get_queued_song(int song_id) const;

    Song* get_next_song() const;
    Song* get_next_song_looped() const;

    int get_longest_bus_id();
    
    std::vector<std::pair<float, float>> get_bus_levels();

    std::pair<double, double> get_bus_playback_info(int bus_id);

private:
    int playing_song;
    std::vector<Song*> queued_songs;
    std::vector<AudioBus> busses;

    bool paused = false;
    bool should_restart = false;

    void process_playing();

    bool is_valid_bus(int id) const;
    bool is_valid_song_id(int id) const;

    void clear_busses();
    int create_bus();

    bool load_song(const Song* new_song, bool verbose = true);

    void swap_buffers();

    bool load_track(AudioBus& bus, const AudioTrack& track, bool verbose = true);
    bool route_channel(int bus_id, int new_device);
    bool init_device(int device, bool verbose = true);

    void set_volume(int bus_id, float vol);

    /* SONG PRELOADER */
    std::vector<AudioBus> bus_buffer;
    
    /*
    std::thread worker;
    std::atomic<bool> stop_flag;
    std::mutex mtx;

    void stop_preloader();
    void preload_next_song(const Song* next_song, float progress_ratio);
    void threaded_load(int longest_bus_id, const Song* next_song, float progress_ratio);
    */
};