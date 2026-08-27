#include "../include/playlist.hpp"
#include <fstream>
#include <iostream>
#include "../include/json.hpp"

using json = nlohmann::json;


Playlist::~Playlist() {
    song_paths.clear();
}

void Playlist::add_song(const std::string& song_path) {
    song_paths.push_back(song_path);
}

void Playlist::remove_song(int song_id) {
    if (is_valid_song_id(song_id)) song_paths.erase(song_paths.begin() + song_id);
}

Playlist* Playlist::create_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Playlist importer] failed to open file " << path << std::endl;
        return nullptr;
    }

    Playlist* new_playlist = new Playlist();

    json data;
    file >> data;

    new_playlist->name = data["name"];
    
    for(const auto& path : data["songs"]) {
        new_playlist->add_song(path);
    }

    return new_playlist;
}

bool Playlist::save_to_file(const std::string& path) {

}

bool Playlist::is_valid_song_id(int id) const {
    return id >= 0 && id < static_cast<int>(song_paths.size());
}
