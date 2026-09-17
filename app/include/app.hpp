#pragma once

#include <ftxui/ftxui.hpp>

#include "audioplayer.hpp"
#include "song_bank.hpp"

namespace ui = ftxui;


class App {
public:

    App() {}
    ~App();

    int main();

private:

    SongBank song_bank;
    std::map<fs::path, Playlist*> playlists;

    AudioPlayer main_player;

    std::string song_bank_path;
    std::string playlist_bank_path;

    /* GUI */

    const int screen_width = 800;
    const int screen_height = 800;


    bool load_config_file();

    void load_all_playlists(const std::string& bank_path);

    std::pair<fs::path, Playlist*> create_playlist();

    std::vector<Playlist*> get_playlists();

    std::string get_playlist_path(Playlist* playlist) const;

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