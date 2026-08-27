#pragma once

#include <string>
#include <vector>
#include "audiotrack.hpp"


class Song {
public:
    Song() {}
    Song(const std::string& _name) : name(_name) {}
    ~Song();

    int add_track(AudioTrack track);
    bool remove_track(int track_id);

    void set_track_volume(int track_id, float vol);
    void set_track_filepath(int track_id, std::string path);
    void set_track_name(int track_id, std::string new_name);
    void set_track_device(int track_id, int device);

    void set_name(const std::string& new_name) { name = new_name; }

    const std::vector<AudioTrack>& get_tracks() const { return tracks; }
    const std::string& get_name() const { return name; }

    static Song* create_from_file(const std::string& path);
    bool save_to_file(const std::string& path);

private:
    std::string name;
    std::vector<AudioTrack> tracks;

    bool is_valid_track_id(int track_id) const;
};