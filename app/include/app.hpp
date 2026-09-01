#pragma once

#include <ftxui/ftxui.hpp>

#include "audioplayer.hpp"
#include "song_editor.hpp"
#include "song_bank.hpp"

#define CONFIG_FILE_PATH "/Users/emaalf/Documents/repos/VBeat/app/config.json"

namespace ui = ftxui;


class App {
public:

    App() {}
    ~App();

    int main();

private:

    SongBank song_bank;
    std::vector<Playlist*> playlists;

    AudioPlayer main_player;
    SongEditor song_editor;

    std::string song_bank_path;
    std::string playlist_bank_path;

    /* GUI */

    const int screen_width = 800;
    const int screen_height = 800;


    bool load_config_file();

    void load_all_playlists(const std::string& bank_path);

    void list_playlists();

    std::vector<Song*> get_songs_in_playlist(Playlist* playlist);

    /* APP MODES */

    void main_loop();

    void playlist_selection();
    void song_selection();

    void song_queue_play_screen(const std::vector<Song*> queue);

    void song_play_screen(Song* song);

    void playlist_creation();

};