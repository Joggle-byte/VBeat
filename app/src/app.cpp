#include <iostream>
#include <fstream>
#include <cstdlib>
#include <thread>
#include <chrono>

#include "../include/app.hpp"
#include "../include/utils.hpp"
#include "../include/json.hpp"
#include "../include/ui_menu.hpp"
#include "../include/logger.hpp"


using json = nlohmann::json;


App::~App() {
    song_bank.clear();
    playlists.clear();
}

int App::main() {
    std::system("cls");

    Logger::get_instance().log("VBeat " + std::string(VBEAT_VERSION) + " - by Emanuele Alfieri\n");
    Logger::get_instance().log("====== Startup Log ======");

    if (!load_config_file()) return 1;

    song_bank.load_all(song_bank_path);
    load_all_playlists(playlist_bank_path);

    Logger::get_instance().log("=========================\n");

    main_loop();

    std::system("cls");

    return 0;
}

bool App::load_config_file() {
    std::ifstream file(CONFIG_FILE_PATH);

    if(!file.is_open()) {
        Logger::get_instance().log_err("[VBeat] unable to load config file at path '" + std::string(CONFIG_FILE_PATH) + "'. Exiting...");
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
            Logger::get_instance().log_err("[Playlist Bank] unable to load playlist '" + p.string() + "'");
            return;
        }
        
        std::vector<std::string> songs = playlist->get_songs();

        for(size_t i = 0; i < songs.size(); i++) {
            if(!song_bank.song_exists(songs[i])) {
                Logger::get_instance().log_err("[Playlist Bank] (" + playlist->get_name() + ") " + "song with id " + std::to_string(i) + " doesn't exist in song bank");
                playlist->remove_song(i);
            }
        }

        playlists.push_back(playlist);
    }

    Logger::get_instance().log("[Playlist Bank] playlist bank loaded succesfully");
}

void App::list_playlists() {
    std::cout << "\n========== PLAYLISTS ==========\n";

    for(size_t i = 0; i < playlists.size(); i++) {
        std::cout << i+1 << ". " << playlists[i]->get_name() << "\n";
        
        std::vector<std::string> songs = playlists[i]->get_songs();
        for(size_t i = 0; i < songs.size(); i++)
            std::cout << "\t" << i + 1 << ". " << song_bank.get_song(songs[i])->get_name() << "\n";
    }
}

std::vector<Song*> App::get_songs_in_playlist(Playlist* playlist) {
    std::vector<Song*> ret;

    for(const auto& s : playlist->get_songs()) {
        Song* song = song_bank.get_song(s);

        if(song)
            ret.push_back(song);
    }

    return ret;
}


/* SCREENS */


void App::main_loop() {
    UIMenu menu("VBeat Menu", {"Select Playlist", "Select Song", "View Log"}, [&] {
        menu.get_screen().ExitLoopClosure()();
    });

    menu.render([&] (ui::Event event) {
        if(event == ui::Event::Return) {
            switch(menu.get_selected_id()) {
                case 0:
                    playlist_selection();
                    break;
                case 1:
                    song_queue_play_screen(song_bank.get_songs());
                    break;
                case 2:
                    show_log();
                    break;
            }
            return true;
        }
        return false;
    });
}

void App::show_log() {
    std::vector<std::string> lines = Logger::get_instance().get_log();

    int scroll_offset = 0;
    const int viewport_height = 16; // altezza visibile dell'area

    auto screen = ui::ScreenInteractive::TerminalOutput();

    auto log_view = ui::Renderer([&] {
        ui::Elements rendered_lines;
        int max_offset = std::max(0, (int)lines.size() - viewport_height);
        scroll_offset = std::clamp(scroll_offset, 0, max_offset);

        for (int i = scroll_offset;
            i < std::min((int)lines.size(), scroll_offset + viewport_height);
            i++) {
            
            auto col = ui::Color::White;
            std::string text_str = lines[i];

            if(text_str[0] == '$') {
                col = ui::Color::Red;
                text_str = text_str.substr(1);
            }
            else if(text_str[0] == '%') {
                col = ui::Color::Yellow;
                text_str = text_str.substr(1);
            }
            
            auto t = ui::text(text_str);
            rendered_lines.push_back(t | ui::color(col));
        }

        return ui::vbox(rendered_lines) |
            ui::size(ui::HEIGHT, ui::EQUAL, viewport_height) |
            ui::border;
    });

    ui::ButtonOption stile_back;
    stile_back.transform = [](const ui::EntryState& state) {
        ui::Element e = ui::text(state.label) | ui::center | ui::size(ui::WIDTH, ui::EQUAL, 20);
 
        if (state.focused)
            e = e | ui::bgcolor(ui::Color::CornflowerBlue) | ui::color(ui::Color::White);
 
        return e | ui::border;
    };

    auto back_button = ui::Button("< Back", screen.ExitLoopClosure(), stile_back);

    auto layout = ui::Container::Vertical({
        log_view,
        back_button,
    });

    auto component = ui::Renderer(layout, [&] {
        return ui::vbox({
            log_view->Render(),
            ui::separator(),
            back_button->Render(),
        });
    });

    component = ui::CatchEvent(component, [&](ui::Event event) {
        if (event == ui::Event::ArrowDown || event.mouse().button == ui::Mouse::WheelDown) {
            scroll_offset++;
            return true;
        }
        if (event == ui::Event::ArrowUp || event.mouse().button == ui::Mouse::WheelUp) {
            scroll_offset--;
            return true;
        }
        return false;
    });

    screen.Loop(component);
}

void App::playlist_selection() {
    std::vector<std::string> options;
    options.reserve(playlists.size());

    for(const auto p : playlists)
        options.push_back(p->get_name());
 
    UIMenu menu("Playlists", options, [&] {
        menu.get_screen().ExitLoopClosure()();
    });

    menu.render([&] (ui::Event event) {
        if(event == ui::Event::Return) {
            song_queue_play_screen(get_songs_in_playlist(playlists[menu.get_selected_id()]));
            return true;
        }
        return false;
    });
}

void App::song_queue_play_screen(const std::vector<Song*> queue) {
    main_player.set_queue(queue);

    std::vector<std::string> options;
    options.reserve(queue.size());

    for(const auto s : queue)
        options.push_back(s->get_name());

    UIMenu menu("Songs", options, [&] { 
        menu.get_screen().ExitLoopClosure()();
    });

    menu.render([&] (ui::Event event) {
        if(event == ui::Event::Return) {
            std::system("cls");
            main_player.play(menu.get_selected_id());
            std::system("cls");
            song_play_screen(main_player.get_queued_song(menu.get_selected_id()));
            std::system("cls");
            menu.select_next();
            return true;
        }
        return false;
    });
}

void App::song_play_screen(Song* song) {
    auto screen = ui::ScreenInteractive::TerminalOutput();

    std::string titolo_brano = song->get_name();
    std::pair<double, double> progresso(0.0f, 0.0f);
    int longest_bus = main_player.get_longest_bus_id();
    
    std::string stato = "Playing";     // "Playing" / "Paused" / "Stopped"

    Song* next_song = main_player.get_next_song();
    std::string prossimo_brano = (next_song) ? next_song->get_name() : "---";

    auto on_play = [&] {
        stato = "Playing";
        main_player.resume();
        ui::animation::RequestAnimationFrame();
    };
    auto on_pause = [&] {
        stato = "Paused";
        main_player.pause();
    };
    auto on_stop = [&] {
        main_player.stop();
        screen.ExitLoopClosure()();
    };

    auto stile_bottone = [](const ui::EntryState& state) {
        ui::Element e = ui::text(state.label) | ui::center | ui::size(ui::WIDTH, ui::EQUAL, 10);

        if (state.focused)
            e = e | ui::bgcolor(ui::Color::CornflowerBlue) | ui::color(ui::Color::White);

        return e | ui::border;
    };

    ui::ButtonOption opzioni_bottone;
    opzioni_bottone.transform = stile_bottone;

    auto btn_play  = ui::Button("Play",  on_play,  opzioni_bottone);
    auto btn_pause = ui::Button("Pause", on_pause, opzioni_bottone);
    auto btn_stop  = ui::Button("Stop",  on_stop,  opzioni_bottone);

    auto controlli = ui::Container::Horizontal({
        btn_play,
        btn_pause,
        btn_stop,
    });

    auto renderer = ui::Renderer(controlli, [&] {
        ui::Elements bus_rows;

        if (main_player.is_playing()) {
            progresso = main_player.get_bus_playback_info(longest_bus);
                
            auto bus_levels = main_player.get_bus_levels();

            for (size_t i = 0; i < main_player.bus_count(); i++) {
                AudioTrack track = main_player.get_bus(i)->get_track();
                bus_rows.push_back(
                    ui::hbox({
                        ui::text(std::to_string(i + 1) + ". " + track.name) | ui::size(ui::WIDTH, ui::EQUAL, 12),
                        ui::separatorEmpty(),
                        ui::vbox({
                            ui::gauge(bus_levels[i].first) | ui::color(ui::Color::Cyan),
                            ui::separatorEmpty(),
                            ui::gauge(bus_levels[i].second) | ui::color(ui::Color::Cyan),
                            ui::separatorEmpty()
                        }) | ui::flex,
                        ui::filler() | ui::flex
                    })
                );
            }

            ui::animation::RequestAnimationFrame();
        } else if(!main_player.is_paused()) {
            screen.ExitLoopClosure()();
        }

        return ui::vbox({
                   ui::text(titolo_brano) | ui::bold | ui::center,
                   ui::separatorEmpty(),

                   ui::gauge(progresso.first / progresso.second) | ui::color(ui::Color::Green),
                   ui::text(format_time_to_minutes(progresso.first) + " | " + format_time_to_minutes(progresso.second))
                       | ui::center | ui::dim,

                   ui::separator(),
                   ui::vbox(std::move(bus_rows)),
                   ui::separator(),

                   ui::hbox({
                       ui::filler(),
                       btn_play->Render(),
                       ui::text(" "),
                       btn_pause->Render(),
                       ui::text(" "),
                       btn_stop->Render(),
                       ui::filler(),
                   }),

                   ui::separator(),

                   ui::text("State: " + stato) | ui::center,

                   ui::text("Next: " + prossimo_brano) | ui::center | ui::dim,
               }) |
               ui::size(ui::WIDTH, ui::GREATER_THAN, 50) |
               ui::border;
    });

    screen.Loop(renderer);
}

