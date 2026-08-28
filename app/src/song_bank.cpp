#include "../include/song_bank.hpp"

#include "../include/json.hpp"
#include "../include/bass.h"
#include "../include/utils.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>


bool is_device_available(int deviceIndex) {
    if(deviceIndex < 0) return false;

    BASS_DEVICEINFO info;
    if (!BASS_GetDeviceInfo(static_cast<DWORD>(deviceIndex), &info)) {
        return false;
    }

    return (info.flags & BASS_DEVICE_ENABLED) != 0;
}


SongBank::~SongBank() {
    clear();
}

void SongBank::load_all(const std::string& bank_path) {
    std::vector<fs::path> all = get_files_by_extension(fs::path(bank_path), ".json");

    for(const auto& p : all) {
        Song* s = Song::create_from_file(p.string());

        if(!s)
            std::cerr << ERROR_COL << "[Song Bank] unable to load song '" << p.string() << "'\n" << END;
        else if(validate_song(s)) bank[p] = s;
    }

    std::cout << "[Song Bank] song bank loaded succesfully\n";
}

bool SongBank::validate_song(Song* s) {
    std::vector<AudioTrack> corrputed_tracks;
    std::vector<int> unavailable_devices;

    bool flag = true;

    for(const auto& t : s->get_tracks()) {
        if(!fs::exists(fs::path(t.file_path))) {
            corrputed_tracks.push_back(t);
            flag = false;
        }
        
        if(!is_device_available(t.device_id)) {
            unavailable_devices.push_back(t.device_id);
            flag = false;
        }
    }

    if(!corrputed_tracks.empty()) {
        std::cerr << ERROR_COL << "[Song Bank] cannot resolve tracks of song '" << s->get_name() << "' :\n";
        for(const auto& p : corrputed_tracks)
            std::cerr << "\t> Track '" << p.name << "' (" << p.file_path << ")\n";
        std::cout << END;
    }

    if(!unavailable_devices.empty()) {
        std::cerr << WARNING << "[Song Bank] Unavailable audio devices found in song '" << s->get_name() << "' :\n";
        for(const auto i : unavailable_devices)
            std::cerr << "\t> Device " << i << "\n";
        std::cout << END;
    }
    
    return flag;
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

bool SongBank::song_exists(const std::string& path) {
    auto it = bank.find(fs::path(path));
    return (it != bank.end());
}

void SongBank::list_songs() {
    std::cout << "\n========== SONGS ==========\n";

    int counter = 1;

    for(const auto& s : bank)
        std::cout << counter++ << ". " << s.second->get_name() << "\n";
}