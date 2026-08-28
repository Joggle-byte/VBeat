#include "../include/app.hpp"
#include "../include/utils.hpp"
#include "../include/json.hpp"

#include <iostream>
#include <fstream>
#include <cstdlib>

using json = nlohmann::json;


App::~App() {
    song_bank.clear();
    playlists.clear();
}

int App::main() {
    std::cout << END;

    std::system("cls");

    if (!load_config_file()) return 1;

    song_bank.load_all(song_bank_path);
    load_all_playlists(playlist_bank_path);

    home();
    cli_loop();

    return 0;
}

bool App::load_config_file() {
    std::ifstream file(CONFIG_FILE_PATH);

    if(!file.is_open()) {
        std::cerr << ERROR_COL << "[VBeat] unable to load config file at path '" << CONFIG_FILE_PATH << "'. Exiting...\n" << END;
        return false;
    }

    json config;
    file >> config;

    song_bank_path = config["song_bank_path"];
    playlist_bank_path = config["playlist_bank_path"];

    return true;
}

void App::load_all_playlists(const std::string& bank_path) {
    std::vector<fs::path> all = get_files_by_extension(fs::path(bank_path), ".json");

    for(const auto& p : all) {
        Playlist* playlist = Playlist::create_from_file(p.string());

        if(!playlist) {
            std::cerr << ERROR_COL << "[Playlist Bank] unable to load playlist '" << p.string() << "'\n" << END;
            return;
        }
        
        bool flag = true;

        for(const auto& i : playlist->get_songs()) {
            if(!song_bank.song_exists(i)) {
                std::cerr << ERROR_COL << "[Playlist Bank] (" << playlist->get_name() << ") " << "song '" << i << "' doesn't exist in song bank\n" << END;
                flag = false;
            }
        }

        if(flag) playlists.push_back(playlist);
    }

    std::cout << "[Playlist Bank] playlist bank loaded succesfully\n";
}

void App::list_playlists() {
    std::cout << "\n========== PLAYLISTS ==========\n";

    for(size_t i = 0; i < playlists.size(); i++)
        std::cout << i+1 << ". " << playlists[i]->get_name() << "\n";
}


/* SCREENS */

void App::home() {
    std::cout << "\n#   # ####  #####  ###  #####" << "\n";
    std::cout << "#   # #   # #     #   #   #  " << "\n";
    std::cout << "#   # ####  ####  #####   #  " << "\n";
    std::cout << " # #  #   # #     #   #   #  " << "\n";
    std::cout << "  #   ####  ##### #   #   #  " << "\n";
    std::cout << "     by Emanuele Alfieri     " << "\n\n";

    
}

void App::cli_loop() {
    bool running = true;

    while(running) {
        std::cout << "\n [Enter command] > ";

        std::string command;
        std::getline(std::cin, command);

        if(command == "list s")
            song_bank.list_songs();
        else if(command == "list p")
            list_playlists();
        else if(command == "quit")
            running = false;
        else
            std::cout << ERROR_COL << " invalid command : " << END << command << "\n";
    }
}

void App::playlist_selection() {

}