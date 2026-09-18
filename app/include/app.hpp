#pragma once

#include <ftxui/ftxui.hpp>

#include "audioplayer.hpp"
#include "song_bank.hpp"

#define CONFIG_FILE_PATH "./"

namespace ui = ftxui;


class App {
public:

    App() {}
    ~App();

    int main();

private:

    SongBank song_bank;
    std::map<std::string, Playlist*> playlists;

    AudioPlayer main_player;

    std::string song_bank_path;
    std::string playlist_bank_path;

    /* GUI */

    const int screen_width = 800;
    const int screen_height = 800;


    bool load_config_file();

    void load_all_playlists(const std::string& bank_path);

    std::pair<std::string, Playlist*> create_playlist();

    bool save_playlist_to_file(const std::string& uid);

    Playlist* get_playlist(const std::string& uid);

    std::vector<Playlist*> get_playlists();

    fs::path get_playlist_path(Playlist* playlist) const;

    fs::path get_playlist_path(const std::string& uid) const;

    const std::string get_playlist_uid(Playlist* playlist) const;

    bool playlist_exists(const std::string& name) const;

    void clear_playlists();

    std::vector<Song*> get_songs_in_playlist(Playlist* playlist);

    bool wipe_memory();

    /* APP MODES */

    void main_loop();

    void show_log();

    bool confirm_dialog(const std::string& title);

    void playlist_selection();
    void playlist_creation(Playlist* edit_playlist = nullptr);
    void playlist_editing();

    void song_creation(Song* song = nullptr);
    void song_editing();

    void song_selection();

    void song_queue_play_screen(const std::vector<Song*> queue);

    void song_play_screen(Song* song);
};