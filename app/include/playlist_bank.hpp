#pragma once

#include "playlist.hpp"
#include "song_bank.hpp"


class PlaylistBank {
public:

    PlaylistBank() {}
    ~PlaylistBank();

    void load_all(const std::string& path, const SongBank* song_bank);

    void reload_all(const SongBank* song_bank);

    std::pair<std::string, Playlist*> create_playlist();

    bool save_playlist_to_file(const std::string& uid);

    Playlist* get_playlist(const std::string& uid);

    void delete_playlist(const std::string& uid);

    void delete_playlist(Playlist* play);

    void clear();

    bool playlist_exists(const std::string& name) const;

    std::vector<Playlist*> get_playlists();

    fs::path get_playlist_path(Playlist* playlist) const;

    fs::path get_playlist_path(const std::string& uid) const;

    const std::string get_playlist_uid(Playlist* playlist) const;

    std::vector<Song*> get_songs_in_playlist(Playlist* playlist, SongBank* song_bank);

    const std::string& get_path() { return bank_path; }

    bool empty() const { return bank.empty(); }

private:
    std::map<std::string, Playlist*> bank;
    
    std::string bank_path;
};