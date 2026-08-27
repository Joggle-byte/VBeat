#include "../include/song.hpp"
#include <fstream>
#include <iostream>
#include "../include/json.hpp"

using json = nlohmann::json;


Song::~Song() {
    tracks.clear();
}


int Song::add_track(AudioTrack track) {
    tracks.emplace_back();

    int track_id = static_cast<int>(tracks.size()) - 1;

    tracks[track_id] = track;

    return track_id;
}

bool Song::remove_track(int track_id) {
    if (!is_valid_track_id(track_id)) return false;

    tracks.erase(tracks.begin() + track_id);
    return true;
}

void Song::set_track_volume(int track_id, float vol) {
    if (is_valid_track_id(track_id))
        tracks[track_id].volume = vol;
}

void Song::set_track_filepath(int track_id, std::string path) {
    if (is_valid_track_id(track_id))
        tracks[track_id].file_path = path;
}

void Song::set_track_name(int track_id, std::string new_name) {
    if (is_valid_track_id(track_id))
        tracks[track_id].name = new_name;
}

void Song::set_track_device(int track_id, int device) {
    if (is_valid_track_id(track_id))
        tracks[track_id].device_id = device;
}

bool Song::is_valid_track_id(int track_id) const {
    return track_id >= 0 && track_id < static_cast<int>(tracks.size());
}   


Song* Song::create_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Song importer] failed to open file " << path << std::endl;
        return nullptr;
    }

    Song* new_song = new Song();

    json data;
    file >> data;

    new_song->name = data["name"];

    for(const auto& track : data["tracks"]) {
        new_song->add_track({
            track["name"],
            track["path"],
            track["volume"],
            static_cast<int>(track["device"])
        });
    }

    return new_song;
}

bool Song::save_to_file(const std::string& path) {

}