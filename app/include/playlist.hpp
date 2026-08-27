#pragma once

#include "song.hpp"
#include <string>
#include <vector>


class Playlist {
public:

    Playlist(const std::string& _name) : name(_name) {}
    Playlist() {}
    ~Playlist();

    void add_song(const std::string& song_path);
    void remove_song(int song_id);
    void set_name(const std::string& new_name) { name = new_name; }

    const std::string& get_name() const { return name; }
    const std::vector<std::string>& get_songs() const { return song_paths; }

    static Playlist* create_from_file(const std::string& path);
    bool save_to_file(const std::string& path);

private:
    std::string name;
    std::vector<std::string> song_paths;

    bool is_valid_song_id(int id) const;
};