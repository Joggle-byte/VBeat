#pragma once

#include <vector>
#include <unordered_map>

#include "audiobus.hpp"
#include "song.hpp"
#include "playlist.hpp"


struct Device {
    std::string name;
    std::string id;

    int session_index = -1;
};


class AudioPlayer {
public:

    AudioPlayer() {}
    ~AudioPlayer();

    void list_devices();

    std::vector<Device> get_devices();

    static std::string get_device_name_from_id(const std::string& id);

    static int get_device_index_by_id(const std::string& target_driver);

    size_t get_device_audio_out_count(const std::string& id);

    /*
    std::vector<std::string> get_device_names() const;
    std::vector<std::string> get_device_ids() const;

    std::string get_device_id_from_index(int index) const;
    std::string get_device_name_from_id(const std::string& id) const;
    */

    void queue_song(Song* song);
    void set_queue(std::vector<Song*> new_queue);

    void clear_queue();

    void play(int song_id);
    void play_current();

    void pause();
    void stop();
    void resume();

    void set_playback_pos(double seconds);

    int select_next_song();
    int select_next_song_looped();

    // Markers
    std::vector<Marker> get_current_marker_layout();
    //

    bool is_playing();

    bool is_paused() const { return paused; }

    size_t bus_count() const { return busses.size(); }

    AudioBus* get_bus(int bus_id);

    Song* get_queued_song(int song_id) const;
    Song* get_current_song() const;
    
    int get_playing_song() const { return playing_song; }
    Song* get_next_song() const;
    Song* get_next_song_looped() const;
    size_t get_queue_size() const { return queued_songs.size(); }


    int get_longest_bus_id();
    
    std::vector<std::pair<float, float>> get_bus_levels();

    std::pair<double, double> get_bus_playback_info(int bus_id);

private:
    std::unordered_map<std::string, int> active_devices; // device id -> device index

    int playing_song;
    std::vector<Song*> queued_songs;

    std::vector<AudioBus> busses;

    HSTREAM mixer = 0;

    bool paused = false;
    bool should_restart = false;

    int acquire_device(const std::string& id);
    void release_all_devices();

    bool is_valid_bus(int id) const;
    bool is_valid_song_id(int id) const;

    void clear_busses();
    int create_bus();

    bool load_song(Song* new_song, bool verbose = true);

    void set_volume(int bus_id, float vol);


};