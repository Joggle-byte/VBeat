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
        Song* s = Song::create_from_file(p);

        if(!s) {
            Logger::get_instance().log(std::string(ERROR_COL) + "[Song Bank] unable to load song '" + p.string() + "'" + std::string(END));
            return;
        }
        
        validator.validate(s);
        bank[p.stem().string()] = s;
    }

    this->bank_path = bank_path;

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

Song* SongBank::get_song(const std::string& uid) {
    auto it = bank.find(uid);
    if (it != bank.end()) return it->second;

    return nullptr;
}

const std::string SongBank::get_song_uid(const Song* song) const {
    auto it = std::find_if(bank.begin(), bank.end(), 
        [&song](const auto& pair) {
            return pair.second == song;
        });

    if (it != bank.end()) {
        return it->first;
    } else return "";
}

fs::path SongBank::get_song_path(const Song* song) const {
    if(bank_path.empty()) return fs::path();

    return fs::path(bank_path) / fs::path(get_song_uid(song) + ".json");
}

fs::path SongBank::get_song_path(const std::string& uid) const {
    auto it = bank.find(uid);

    if(it != bank.end())
        return fs::path(bank_path) / fs::path(uid + ".json");
    
    return fs::path();
}

std::vector<Song*> SongBank::get_songs() {
    std::vector<Song*> values;
    values.reserve(bank.size());

    for (const auto& s : bank) {
        values.push_back(s.second);
    }

    return values;
}

std::pair<std::string, Song*> SongBank::create_song() {
    Song* s = new Song();
    std::string uid = generate_uuid_v4();

    bank[uid] = s;

    return std::pair<std::string, Song*>(uid, s);
}

bool SongBank::save_song_to_file(const std::string& uid) {
    Song* song = get_song(uid);

    if(!song)
        return false;

    return song->save_to_file(get_song_path(uid));
}

bool SongBank::song_exists(const std::string& uid) const {
    auto it = bank.find(uid);
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