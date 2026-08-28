#pragma once

#include "audiobus.hpp"
#include "song.hpp"
#include "playlist.hpp"
#include <vector>


class AudioPlayer {
public:

    AudioPlayer() {}
    ~AudioPlayer();

    void list_devices();
    void list_song_queue();

    void queue_song(Song* song);

    void clear_queue();

    bool load_playlist(Playlist* playlist);

    void play(int song_id, bool restart = false);
    void play_queue();

    void pause();
    void stop();

    bool is_playing();

    size_t bus_count() const { return busses.size(); }

    Song* get_queued_song(int song_id) const;

private:
    Song* playing_song;
    std::vector<Song*> queued_songs;
    std::vector<AudioBus> busses;

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