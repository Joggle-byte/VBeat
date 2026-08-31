#pragma once

#include "audiobus.hpp"
#include "song.hpp"
#include "playlist.hpp"
#include <vector>
#include <functional>


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

    bool load_song(Song* new_song);

    bool load_track(int bus_id, const AudioTrack& track);
    bool route_channel(int bus_id, int new_device);
    bool init_device(int device);

    void set_volume(int bus_id, float vol);
};