#include "../include/playlist_bank.hpp"
#include "../include/logger.hpp"
#include "../include/utils.hpp"

#include <algorithm>


PlaylistBank::~PlaylistBank() {
    clear();
}

void PlaylistBank::load_all(const std::string& bank_path, const SongBank* song_bank) {
    if(!fs::exists(fs::path(bank_path)) || !fs::is_directory(fs::path(bank_path))) {
        Logger::get_instance().log_err("[Playlist Bank] invalid playlist bank path '" + bank_path + "'");
        this->bank_path = "";
        return;
    }

    this->bank_path = bank_path;

    std::vector<fs::path> all = get_files_by_extension(fs::path(bank_path), ".json");

    for(const auto& p : all) {
        Playlist* playlist = Playlist::create_from_file(p);

        if(!playlist) {
            Logger::get_instance().log_err("[Playlist Bank] unable to load playlist '" + p.string() + "'");
            return;
        }
        
        std::vector<std::string> songs = playlist->get_songs();

        for(size_t i = 0; i < songs.size(); i++) {
            if(!song_bank->song_exists(songs[i])) {
                Logger::get_instance().log_err("[Playlist Bank] (" + playlist->get_name() + ") " + "song with uid '" + songs[i] + "' doesn't exist in song bank");
                playlist->remove_song(i);
            }
        }

        bank[p.stem().string()] = playlist;
    }

    if(bank.empty())
        Logger::get_instance().log_warn("[Playlist Bank] playlist bank is empty!");
    else
        Logger::get_instance().log("[Playlist Bank] playlist bank loaded succesfully");
}

void PlaylistBank::reload_all(const SongBank* song_bank) {
    if(bank_path.empty()) return;

    clear();
    load_all(bank_path, song_bank);
}

std::pair<std::string, Playlist*> PlaylistBank::create_playlist() {
    Playlist* p = new Playlist();

    std::string uid = generate_uuid_v4();

    bank[uid] = p;

    return std::pair<std::string, Playlist*>(uid, p);
}

bool PlaylistBank::save_playlist_to_file(const std::string& uid) {
    Playlist* playlist = get_playlist(uid);

    if(!playlist) return false;

    return playlist->save_to_file(get_playlist_path(uid));
}

Playlist* PlaylistBank::get_playlist(const std::string& uid) {
    auto it = bank.find(uid);
    if (it != bank.end()) return it->second;

    return nullptr;
}

void PlaylistBank::delete_playlist(const std::string& uid) {
    auto play = get_playlist(uid);

    if(!play) return;

    delete play;

    auto play_path = get_playlist_path(uid);

    bank.erase(bank.find(uid));

    if(fs::remove(play_path))
        Logger::get_instance().log_warn("[Playlist Bank] succesfully removed playlist '" + play_path.string() + "'");
    else 
        Logger::get_instance().log_err("[Playlist Bank] impossible to remove playlist '" + play_path.string() + "'");
}

void PlaylistBank::delete_playlist(Playlist* play) {
    auto uid = get_playlist_uid(play);

    if(uid.empty()) return;

    delete play;

    auto play_path = get_playlist_path(uid);

    bank.erase(bank.find(uid));

    if(fs::remove(play_path))
        Logger::get_instance().log_warn("[Playlist Bank] succesfully removed playlist '" + play_path.string() + "'");
    else 
        Logger::get_instance().log_err("[Playlist Bank] impossible to remove playlist '" + play_path.string() + "'");
}

void PlaylistBank::clear() {
    for(const auto& i : bank)
        delete i.second;
    
    bank.clear();
}

bool PlaylistBank::playlist_exists(const std::string& name) const {
    for(const auto& p : bank) {
        if(p.second->get_name() == name) return true;
    }

    return false;
}

std::vector<Playlist*> PlaylistBank::get_playlists() {
    std::vector<Playlist*> ret;

    for(const auto& p : bank) {
        ret.push_back(p.second);
    }

    return ret;
}

fs::path PlaylistBank::get_playlist_path(Playlist* playlist) const {
    if(bank_path.empty()) return fs::path();

    auto it = std::find_if(bank.begin(), bank.end(), 
        [&playlist](const auto& pair) {
            return pair.second == playlist;
        });

    if (it != bank.end()) {
        return fs::path(bank_path) / fs::path(it->first + ".json");
    } else return fs::path();
}

fs::path PlaylistBank::get_playlist_path(const std::string& uid) const {
    if(bank_path.empty()) return fs::path();

    auto it = bank.find(uid);

    if(it != bank.end())
        return fs::path(bank_path) / fs::path(uid + ".json");
    
    return fs::path();
}

const std::string PlaylistBank::get_playlist_uid(Playlist* playlist) const {
    auto it = std::find_if(bank.begin(), bank.end(), 
        [&playlist](const auto& pair) {
            return pair.second == playlist;
        });

    if (it != bank.end()) {
        return it->first;
    } else return "";
}

std::vector<Song*> PlaylistBank::get_songs_in_playlist(Playlist* playlist, SongBank* song_bank) {
    std::vector<Song*> ret;

    for(const auto& s : playlist->get_songs()) {
        Song* song = song_bank->get_song(s);

        if(song)
            ret.push_back(song);
    }

    return ret;
}
