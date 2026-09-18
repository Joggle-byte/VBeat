#pragma once

#include <string>
#include <vector>

#include "song.hpp"

namespace fs = std::filesystem;


class Playlist {
public:

    Playlist(const std::string& _name) : name(_name) {}
    Playlist() {}
    ~Playlist();

    void add_song(const std::string& song_uid);
    void remove_song(int song_id);
    void clear_songs() { song_uids.clear(); }
    void set_name(const std::string& new_name) { name = new_name; }

    const std::string& get_name() const { return name; }
    const std::vector<std::string>& get_songs() const { return song_uids; }

    static Playlist* create_from_file(const fs::path& path);
    bool save_to_file(const fs::path& path);

private:
    std::string name;
    std::vector<std::string> song_uids;

    bool is_valid_song_id(int id) const;
};