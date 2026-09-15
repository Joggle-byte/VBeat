#include "../include/song_bank.hpp"
#include "../include/json.hpp"
#include "../include/bass.h"
#include "../include/utils.hpp"
#include "../include/logger.hpp"
#include "../include/audiobus.hpp"

#include <fstream>
#include <filesystem>
#include <iostream>


SongBank::~SongBank() {
    clear();
}

void SongBank::load_all(const std::string& bank_path) {
    std::vector<fs::path> all = get_files_by_extension(fs::path(bank_path), ".json");

    for(const auto& p : all) {
        Song* s = Song::create_from_file(p.string());

        if(!s) {
            Logger::get_instance().log(std::string(ERROR_COL) + "[Song Bank] unable to load song '" + p.string() + "'" + std::string(END));
            return;
        }
        
        validator.validate(s);
        bank[p] = s;
    }

    if(bank.empty()) Logger::get_instance().log_warn("[Song Bank] song bank is empty!");
}

void SongBank::validate_song(Song* song) {
    validator.validate(song);
}

void SongBank::validate_all() {
    for(const auto& i : bank)
        validator.validate(i.second);
}

void SongBank::clear() {
    for(const auto& i : bank)
        delete i.second;
    
    bank.clear();
}

Song* SongBank::get_song(const std::string& path) {
    auto it = bank.find(fs::path(path));
    if (it != bank.end()) return it->second;

    return nullptr;
}

std::string SongBank::get_song_path(const Song* song) const {
    auto it = std::find_if(bank.begin(), bank.end(), 
        [&song](const auto& pair) {
            return pair.second == song;
        });

    if (it != bank.end()) {
        return it->first.string();
    } else return "";
}

std::vector<Song*> SongBank::get_songs() {
    std::vector<Song*> values;
    values.reserve(bank.size());

    for (const auto& s : bank) {
        values.push_back(s.second);
    }

    return values;
}

std::pair<fs::path, Song*> SongBank::create_song(const std::string& directory) {
    Song* s = new Song();
    fs::path path = fs::path(directory) / fs::path(generate_uuid_v4() + ".json");

    bank[path] = s;

    return std::pair<fs::path, Song*>(path, s);
}


bool SongBank::song_exists(const std::string& path) const {
    auto it = bank.find(fs::path(path));
    return (it != bank.end());
}

bool SongBank::song_exists_by_name(const std::string& name) const {
    for(const auto& s : bank) {
        if(s.second->get_name() == name) return true;
    }

    return false;
}

void SongBank::list_songs() {
    std::cout << "\n========== SONGS ==========\n";

    int counter = 1;

    for(const auto& s : bank)
        std::cout << counter++ << ". " << s.second->get_name() << "\n";
}