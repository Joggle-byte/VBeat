#include <iostream>
#include <fstream>
#include <cstdlib>
#include <thread>
#include <chrono>

//TEMP
#include <list>

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
    std::system("clear && printf '\e[3J'");

    Logger::get_instance().log("VBeat " + std::string(VBEAT_VERSION) + " - by Emanuele Alfieri\n");
    Logger::get_instance().log("====== Startup Log ======");

    if (!load_config_file()) return 1;

    song_bank.load_all(song_bank_path);
    load_all_playlists(playlist_bank_path);

    Logger::get_instance().log("=========================\n");

    main_player.list_devices();

    main_loop();

    std::system("clear && printf '\e[3J'");

    return Logger::get_instance().write_to_file();
}

fs::path getAppSupportDir(const std::string& appName) {
    const char* home = std::getenv("HOME");
    if (!home) {
        throw std::runtime_error("Cannot resolve HOME");
    }
    fs::path path = fs::path(home) 
                                  / "Library" / "Application Support" / appName;
    return path;
}

std::string make_default_config_file(fs::path dir) {
    std::string p = "{\n\t\"song_bank_path\" : \"" + (dir / "songs").string() + std::string("\",\n\t")
        + "\"playlist_bank_path\" : \"" + (dir / "playlists").string() + std::string("\",\n\t")
        + "\"log_file_dir\" : \"" + (dir / "logs").string() + std::string("\"\n}");
    
    return p;
}

bool App::load_config_file() {
    fs::path dir = getAppSupportDir(APP_NAME);

    fs::create_directories(dir);
    fs::create_directories(dir / "songs");
    fs::create_directories(dir / "playlists");
    fs::create_directories(dir / "logs");

    if(!fs::exists(dir / "config.json")) {
        std::ofstream new_file(dir/"config.json");

        if(!new_file.is_open()) {
            std::cout << "[VBeat] unable to create config file. Exiting...";
            return false;
        }

        new_file << make_default_config_file(dir);
        new_file.close();
    }

    std::ifstream file(dir / "config.json");

    if(!file.is_open()) {
        std::cout << "[VBeat] unable to load config file. Exiting...";
        return false;
    }

    json config;

    try {
        file >> config;
        song_bank_path = config["song_bank_path"];
        playlist_bank_path = config["playlist_bank_path"];
        Logger::get_instance().set_log_file_dir(config["log_file_dir"]);
    } catch(...) {
        Logger::get_instance().log_err("[VBeat] unable to parse config file. Exiting...");
        return false;
    }

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

    if(playlists.empty())
        Logger::get_instance().log_warn("[Playlist Bank] playlist bank is empty!");
    else
        Logger::get_instance().log("[Playlist Bank] playlist bank loaded succesfully");
}

void App::clear_playlists() {
    for(const auto i : playlists)
        delete i;
    
    playlists.clear();
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
    UIMenu menu("VBeat Menu", {"Select Playlist", "Select Song", "+ Create Song", "✎ Edit Song", "🗎 View Log", "🗘 Reload Songs & Playlists"}, [&] {
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
                    song_creation();
                    break;
                case 3:
                    song_editing();
                    break;
                case 4:
                    show_log();
                    break;
                case 5:
                    Logger::get_instance().log(">>> RELOADING SONGS & PLAYLISTS...");
                    clear_playlists();
                    song_bank.clear();
                    song_bank.load_all(song_bank_path);
                    load_all_playlists(playlist_bank_path);
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
    const int viewport_height = 20; // altezza visibile dell'area

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
    if(playlists.empty()) return;

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
    if(queue.empty()) return;
    
    main_player.set_queue(queue);

    std::vector<std::string> options;

    for(const auto s : queue) {
        std::string name = s->get_name();

        if(s->is_degraded())
           name.insert(0, 1, '%');
        else if(s->is_corrupted())
           name.insert(0, 1, '$'); 

        options.push_back(name);
    }

    UIMenu menu("Songs", options, [&] { 
        menu.get_screen().ExitLoopClosure()();
    });

    if(main_player.get_queued_song(menu.get_selected_id())->is_corrupted()) {
        menu.set_selected(main_player.select_next_song_looped());
    }

    menu.render([&] (ui::Event event) {
        if(event == ui::Event::Return) {
            main_player.play(menu.get_selected_id());
            song_play_screen(main_player.get_current_song());
            menu.set_selected(main_player.select_next_song_looped());
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


    std::string prossimo_brano = "---";

    if(next_song) {
        prossimo_brano = next_song->get_name();

        if(next_song->get_state() == SongState::DEGRADED)
            prossimo_brano += " (DEGRADED)";
        else if(next_song->get_state() == SongState::CORRUPTED)
            prossimo_brano += " (CORRUPTED)";
    }

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

    auto btn_play  = ui::Button("▶",  on_play,  opzioni_bottone);
    auto btn_pause = ui::Button("⏸", on_pause, opzioni_bottone);
    auto btn_stop  = ui::Button("◼",  on_stop,  opzioni_bottone);

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


struct UITrack {
    AudioTrack data;
    int device_index = 0;
};

void App::song_creation(Song* edit_song) {
    auto screen = ui::ScreenInteractive::TerminalOutput();
 
    std::string nome_canzone;

    if(edit_song) nome_canzone = edit_song->get_name();
 
    std::vector<std::string> audio_devices_names = main_player.get_device_names();
    std::vector<std::string> audio_devices_ids = main_player.get_device_ids();
 
    std::list<UITrack> tracce;
 
    auto container_tracce = ui::Container::Vertical({});
 
    auto crea_ui_traccia = [&](std::list<UITrack>::iterator it) -> ui::Component {
        UITrack& stato = *it;

        auto input_nome = ui::Input(&stato.data.name, "Track name...");

        ui::SliderOption<float> opzioni_volume;
        opzioni_volume.value = &stato.data.volume;
        opzioni_volume.min = 0.0f;
        opzioni_volume.max = 2.0f;
        opzioni_volume.increment = 0.05f;

        auto slider_volume = ui::Slider<float>(opzioni_volume);

        auto input_percorso = ui::Input(&stato.data.file_path, "File path...");
 
        
        stato.device_index = AudioBus::find_device_by_driver(stato.data.device_id);

        auto dropdown_device = ui::Dropdown(&audio_devices_names, &stato.device_index);
 
        //DELETE BUTTON
        auto self_weak_box = std::make_shared<std::weak_ptr<ui::ComponentBase>>();
 
        auto btn_elimina = ui::Button("🗑 Delete", [self_weak_box, it, &tracce] {
            if (auto self = self_weak_box->lock())
                self->Detach();
            tracce.erase(it);
        });
 
        
        auto gruppo = ui::Container::Vertical({
            input_nome,
            input_percorso,
            slider_volume,
            dropdown_device,
            btn_elimina
        });

        ui::Component componente = ui::Renderer(gruppo, [&stato, input_nome, slider_volume,
                                                   input_percorso, dropdown_device,
                                                   btn_elimina] {
            return ui::vbox({
                       ui::hbox(ui::text("Name:    ") | ui::dim, input_nome->Render()),
                       ui::hbox(ui::text("Volume:  ") | ui::dim, slider_volume->Render() | ui::flex,
                            ui::text(" " + std::to_string(stato.data.volume))),
                       ui::hbox(ui::text("File:    ") | ui::dim, input_percorso->Render()),
                       ui::hbox(ui::text("Device:  ") | ui::dim, dropdown_device->Render()),
                       ui::separator(),
                       btn_elimina->Render(),
                   }) |
                   ui::border;
        });
 
        *self_weak_box = componente;
        return componente;
    };
 
    auto on_aggiungi_click = [&] {
        tracce.emplace_back();
        auto it = std::prev(tracce.end());
        container_tracce->Add(crea_ui_traccia(it));
    };
 
    auto btn_aggiungi = ui::Button("+ Add Track", on_aggiungi_click);
 
    auto input_nome_canzone = ui::Input(&nome_canzone, "Song name...");
 
 
    auto on_salva_click = [&] {
        if(tracce.empty() || nome_canzone.empty()) return;
        
        std::string song_path;
        Song* new_song;
        
        if(edit_song) {
            new_song = edit_song;
            song_path = song_bank.get_song_path(edit_song);

            edit_song->clear_tracks();
        } else {
            song_path = song_bank_path + "/" + nome_canzone + ".json";
            new_song = song_bank.create_song(song_path);
        }
        
        new_song->set_name(nome_canzone);

        for(auto& t : tracce) {
            t.data.device_id = audio_devices_ids[t.device_index];
            new_song->add_track(t.data);
        }
            
        song_bank.validate_song(new_song);

        if(!new_song->save_to_file(song_path))
            Logger::get_instance().log_err("[SongEditor] unable to save song '" + nome_canzone + "'. All changes will be discarded");
        else
            Logger::get_instance().log(((edit_song) ? "[SongEditor]>>> Edited new song '" : ">>> Created new song '") + nome_canzone + "'");

        screen.ExitLoopClosure()();
    };
 
    auto on_annulla_click = [&] {
        screen.ExitLoopClosure()();
    };
 
    ui::ButtonOption stile_salva;
    stile_salva.transform = [](const ui::EntryState& state) {
        ui::Element e = ui::text(state.label) | ui::center | ui::size(ui::WIDTH, ui::EQUAL, 18);
        if (state.focused)
            e = e | ui::bgcolor(ui::Color::Green) | ui::color(ui::Color::White);
        return e | ui::border;
    };
 
    ui::ButtonOption stile_annulla;
    stile_annulla.transform = [](const ui::EntryState& state) {
        ui::Element e = ui::text(state.label) | ui::center | ui::size(ui::WIDTH, ui::EQUAL, 18);
        if (state.focused)
            e = e | ui::bgcolor(ui::Color::Red) | ui::color(ui::Color::White);
        return e | ui::border;
    };
 
    auto btn_salva = ui::Button("Save and Back", on_salva_click, stile_salva);
    auto btn_annulla = ui::Button("Cancel", on_annulla_click, stile_annulla);
 
    auto bottoni_finali = ui::Container::Horizontal({
        btn_salva,
        btn_annulla,
    });
 
    auto layout_principale = ui::Container::Vertical({
        input_nome_canzone,
        btn_aggiungi,
        container_tracce,
        bottoni_finali,
    });
    
    auto get_index_from_device = [&] (const std::string& dev) -> int {
        for(int i = 0; i < static_cast<int>(audio_devices_ids.size()); i++) {
            if(audio_devices_ids[i] == dev) return i;
        }
        return -1;
    };

    if(edit_song) {
        for(auto& t : edit_song->get_tracks()) {
            on_aggiungi_click();
            UITrack& new_track = tracce.back();
            new_track.data = t;

            int idx = get_index_from_device(t.device_id);
            new_track.device_index = (idx >= 0) ? idx : 0;
        }
    }

    auto renderer = ui::Renderer(layout_principale, [&] {
        return ui::vbox({
                   ui::text("Create New Song") | ui::bold | ui::center,
                   ui::separator(),
 
                   ui::hbox(ui::text("Song Name: ") | ui::dim, input_nome_canzone->Render()),
 
                   ui::separator(),
 
                   ui::hbox({
                       ui::text("Tracks") | ui::bold,
                       ui::filler(),
                       btn_aggiungi->Render(),
                   }),
                   ui::separator(),
 
                   container_tracce->Render() | ui::vscroll_indicator | ui::frame |
                       ui::size(ui::HEIGHT, ui::LESS_THAN, 20),
 
                   ui::separator(),
                   bottoni_finali->Render() | ui::center,
               }) |
               ui::size(ui::WIDTH, ui::GREATER_THAN, 60) |
               ui::border;
    });
 
    screen.Loop(renderer);
}


void App::song_editing() {
    std::vector<Song*> songs = song_bank.get_songs();

    if(songs.empty()) return;

    std::vector<std::string> options;

    auto load_songs = [&] {
        options.clear();

        for(const auto s : songs) {
            std::string name = s->get_name();

            if(s->is_degraded())
                name.insert(0, 1, '%');
            else if(s->is_corrupted())
                name.insert(0, 1, '$'); 

            options.push_back(name);
        }
    };
    
    load_songs();

    UIMenu menu("Choose a song to edit", options, [&] {
        menu.get_screen().ExitLoopClosure()();
    });

    menu.render([&] (ui::Event event) {
        if(event == ui::Event::Return) {
            song_creation(songs[menu.get_selected_id()]);
            load_songs();
            menu.set_options(options);

            return true;
        }
        return false;
    });
}