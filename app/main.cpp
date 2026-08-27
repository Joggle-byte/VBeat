#include "include/audioplayer.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>


AudioPlayer* main_player;


int main() {
    main_player = new AudioPlayer();

    main_player->list_devices();

    main_player->load_playlist(Playlist::create_from_file("C:/Users/emaal/cpp_games/LivePlay/playlists/playlist_1.json"));

    main_player->list_song_queue();

    main_player->play_queue();

    main_player->stop();

    return 0;
}