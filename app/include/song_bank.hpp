#pragma once

#include "song.hpp"
#include "resource_validator.hpp"
#include <map>
#include <filesystem>


namespace fs = std::filesystem;

class SongBank {
public:

    SongBank() {}
    ~SongBank();

    void load_all(const std::string& bank_path);

    void clear();

    std::pair<std::string, Song*> create_song();

    bool save_song_to_file(const std::string& uid);

    void append_new(Song* s);

    Song* get_song(const std::string& uid);

    const std::string get_song_uid(const Song* song) const;

    fs::path get_song_path(const Song* song) const;
    fs::path get_song_path(const std::string& uid) const;

    std::vector<Song*> get_songs();

    bool song_exists(const std::string& uid) const;

    bool song_exists_by_name(const std::string& name) const;

    void list_songs();

    void validate_song(Song* song);
    void validate_all();

private:
    std::map<std::string, Song*> bank;
    ResourceValidator validator;

    std::string bank_path;
};