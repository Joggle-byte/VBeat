#pragma once

#include <string>
#include <vector>
#include "audiotrack.hpp"


enum class SongState {
    OK,
    DEGRADED,
    CORRUPTED
};


class Song {
public:
    Song() {}
    Song(const std::string& _name) : name(_name) {}
    ~Song();

    int add_track(AudioTrack track);
    bool remove_track(int track_id);

    void set_track_volume(int track_id, float vol);
    void set_track_filepath(int track_id, const std::string& path);
    void set_track_name(int track_id, const std::string& new_name);
    void set_track_device(int track_id, const std::string& device);
    void set_track_state(int track_id, TrackState state);

    void set_name(const std::string& new_name) { name = new_name; }
    void set_state(SongState new_state) { state = new_state; }

    const AudioTrack& get_track(int track_id);
    const std::vector<AudioTrack>& get_tracks() { return tracks; }
    const std::string& get_name() const;
    SongState get_state() const { return state; }
    TrackState get_track_state(int track_id) const;

    size_t get_tracks_count() const { return tracks.size(); }

    static Song* create_from_file(const std::string& path);
    bool save_to_file(const std::string& path);


private:
    std::string name;
    std::vector<AudioTrack> tracks;
    SongState state;

    bool is_valid_track_id(int track_id) const;
};