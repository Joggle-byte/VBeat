#pragma once

#include "audioplayer.hpp"
#include "song_editor.hpp"
#include "song_bank.hpp"

#define CONFIG_FILE_PATH "C:/Users/emaal/cpp_games/VBeat/app/config.json"


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

    bool load_config_file();

    void load_all_playlists(const std::string& bank_path);

    void list_playlists();

    /* APP MODES */

    void home();

    void cli_loop();

    void playlist_selection();
    void playlist_creation();

};