#pragma once

#include "song.hpp"
#include <map>
#include <filesystem>


namespace fs = std::filesystem;

class SongBank {
public:

    SongBank() {}
    ~SongBank();

    void load_all(const std::string& bank_path);

    void clear();

    Song* create_song(const std::string& path);

    void append_new(Song* s);

    Song* get_song(const std::string& path);

    std::vector<Song*> get_songs();

    bool song_exists(const std::string& path);

    void list_songs();

private:
    std::map<fs::path, Song*> bank;

    bool validate_song(Song* s);
};