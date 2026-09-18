#include <fstream>
#include <iostream>
#include <stdexcept>

#include "../include/playlist.hpp"
#include "../include/json.hpp"

using json = nlohmann::json;


Playlist::~Playlist() {
    song_uids.clear();
}

void Playlist::add_song(const std::string& song_uid) {
    song_uids.push_back(song_uid);
}

void Playlist::remove_song(int song_id) {
    if (is_valid_song_id(song_id)) song_uids.erase(song_uids.begin() + song_id);
}

Playlist* Playlist::create_from_file(const fs::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Playlist importer] failed to open file " << path.string() << std::endl;
        return nullptr;
    }

    Playlist* new_playlist = new Playlist();

    json data;

    try {
        file >> data;
        new_playlist->name = data["name"];
        
        for(const auto& song_uid : data["songs"]) {
            new_playlist->add_song(song_uid);
        }
    } catch(...) {
        return nullptr;
    }

    return new_playlist;
}

bool Playlist::save_to_file(const fs::path& path) {
    std::ofstream file(path);

    if(!file.is_open()) {
        std::cerr << "[Playlist exporter] failed to open file " << path.string() << std::endl;
        return false;
    }

    json j;

    j["name"] = name;
    j["songs"] = json::array();

    for(const auto& s : song_uids)
        j["songs"].push_back(s);
    
    file << std::setw(4) << j << std::endl;
    file.close();

    return true;
}

bool Playlist::is_valid_song_id(int id) const {
    return id >= 0 && id < static_cast<int>(song_uids.size());
}
