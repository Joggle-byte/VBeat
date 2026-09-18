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
    clear_playlists();
}

int App::main() {
    std::system("cls");

    Logger::get_instance().log("VBeat " + std::string(VBEAT_VERSION) + " - by Emanuele Alfieri\n");
    Logger::get_instance().log("====== Startup Log ======");

    if (!load_config_file()) return 1;

    song_bank.load_all(song_bank_path);
    load_all_playlists(playlist_bank_path);

    Logger::get_instance().log("=========================\n");

    main_player.list_devices();

    main_loop();
    std::system("cls");

    return Logger::get_instance().write_to_file();
}

bool App::load_config_file() {
    std::ifstream file(std::string(CONFIG_FILE_PATH) + "config.json");

    if (!file.is_open()) {
        std::cout << "[VBeat] unable to load config file. Exiting...";
        return false;
    }

    json config;

    try {
        file >> config;
        song_bank_path = config.at("song_bank_path").get<std::string>();
        playlist_bank_path = config.at("playlist_bank_path").get<std::string>();
        Logger::get_instance().set_log_file_dir(config.at("log_file_dir").get<std::string>());
    } catch (const std::exception& e) {
        Logger::get_instance().log_err(
            std::string("[VBeat] unable to parse config file: ") + e.what()
        );
        return false;
    }

    return true;
}

void App::load_all_playlists(const std::string& bank_path) {
    std::vector<fs::path> all = get_files_by_extension(fs::path(bank_path), ".json");

    for(const auto& p : all) {
        Playlist* playlist = Playlist::create_from_file(p);

        if(!playlist) {
            Logger::get_instance().log_err("[Playlist Bank] unable to load playlist '" + p.string() + "'");
            return;
        }
        
        std::vector<std::string> songs = playlist->get_songs();

        for(size_t i = 0; i < songs.size(); i++) {
            if(!song_bank.song_exists(songs[i])) {
                Logger::get_instance().log_err("[Playlist Bank] (" + playlist->get_name() + ") " + "song with uid '" + songs[i] + "' doesn't exist in song bank");
                playlist->remove_song(i);
            }
        }

        playlists[p.stem().string()] = playlist;
    }

    if(playlists.empty())
        Logger::get_instance().log_warn("[Playlist Bank] playlist bank is empty!");
    else
        Logger::get_instance().log("[Playlist Bank] playlist bank loaded succesfully");
}

std::pair<std::string, Playlist*> App::create_playlist() {
    Playlist* p = new Playlist();

    std::string uid = generate_uuid_v4();

    playlists[uid] = p;

    return std::pair<std::string, Playlist*>(uid, p);
}

bool App::save_playlist_to_file(const std::string& uid) {
    Playlist* playlist = get_playlist(uid);

    if(!playlist) return false;

    return playlist->save_to_file(get_playlist_path(uid));
}

Playlist* App::get_playlist(const std::string& uid) {
    auto it = playlists.find(uid);
    if (it != playlists.end()) return it->second;

    return nullptr;
}

void App::clear_playlists() {
    for(const auto& i : playlists)
        delete i.second;
    
    playlists.clear();
}

bool App::playlist_exists(const std::string& name) const {
    for(const auto& p : playlists) {
        if(p.second->get_name() == name) return true;
    }

    return false;
}

std::vector<Playlist*> App::get_playlists() {
    std::vector<Playlist*> ret;

    for(const auto& p : playlists) {
        ret.push_back(p.second);
    }

    return ret;
}

fs::path App::get_playlist_path(Playlist* playlist) const {
    auto it = std::find_if(playlists.begin(), playlists.end(), 
        [&playlist](const auto& pair) {
            return pair.second == playlist;
        });

    if (it != playlists.end()) {
        return fs::path(playlist_bank_path) / fs::path(it->first + ".json");
    } else return fs::path();
}

fs::path App::get_playlist_path(const std::string& uid) const {
    auto it = playlists.find(uid);

    if(it != playlists.end())
        return fs::path(playlist_bank_path) / fs::path(uid + ".json");
    
    return fs::path();
}

const std::string App::get_playlist_uid(Playlist* playlist) const {
    auto it = std::find_if(playlists.begin(), playlists.end(), 
        [&playlist](const auto& pair) {
            return pair.second == playlist;
        });

    if (it != playlists.end()) {
        return it->first;
    } else return "";
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

bool App::wipe_memory() {
    Logger::get_instance().log_warn("[VBeat]>>> Wiping song bank and playlist bank");

    std::vector<fs::path> all_songs = get_files_by_extension(fs::path(song_bank_path), ".json");
    std::vector<fs::path> all_playlists = get_files_by_extension(fs::path(playlist_bank_path), ".json");

    if(all_songs.empty()) {
        Logger::get_instance().log_err("[VBeat] song bank is empty. Aborting memory wipe");
        return false;
    }

    if(all_playlists.empty()) {
        Logger::get_instance().log_err("[VBeat] playlist bank is empty. Aborting memory wipe");
        return false;
    }

    for(const auto& p : all_songs) {
        if(fs::remove(p)) {
            Logger::get_instance().log_warn("[VBeat] succesfully removed song '" + p.string() + "'");
        } else {
            Logger::get_instance().log_err("[VBeat] impossible to remove song '" + p.string() + "'");
        }
    }

    for(const auto& p : all_playlists) {
        if(fs::remove(p)) {
            Logger::get_instance().log_warn("[VBeat] succesfully removed playlist '" + p.string() + "'");
        } else {
            Logger::get_instance().log_err("[VBeat] impossible to remove playlist '" + p.string() + "'");
        }
    }

    Logger::get_instance().log_warn("[VBeat]>>> Memory wipe completed");
    return true;
}


/* SCREENS */


void App::main_loop() {
    UIMenu menu("VBeat Menu", {"Select Playlist", "Select Song", "+ Create Song", "+ Create Playlist", "✎ Edit Song", "✎ Edit Playlist", "🗎 View Log", "🗘 Reload Songs & Playlists", "$⚠ Wipe Memory"}, [&] {
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
                    playlist_creation();
                    break;
                case 4:
                    song_editing();
                    break;
                case 5:
                    playlist_editing();
                    break;
                case 6:
                    show_log();
                    break;
                case 7:
                    Logger::get_instance().log(">>> RELOADING SONGS & PLAYLISTS...");
                    clear_playlists();
                    song_bank.clear();
                    song_bank.load_all(song_bank_path);
                    load_all_playlists(playlist_bank_path);
                    break;
                case 8:
                    if(confirm_dialog("Are you sure you want to wipe\nall saved songs and playlists?"))
                        wipe_memory();
                    break;
            }
            return true;
        }
        return false;
    });
}

bool App::confirm_dialog(const std::string& title) {
    UIMenu menu(title, {"No, Go Back", "$Yes, Go Ahead"}, [&] {
        menu.get_screen().ExitLoopClosure()();
    });

    bool ret = false;

    menu.render([&] (ui::Event event) {
        if(event == ui::Event::Return) {
            switch(menu.get_selected_id()) {
                case 0:
                    menu.get_screen().ExitLoopClosure()();
                    ret = false;
                    break;
                case 1:
                    menu.get_screen().ExitLoopClosure()();
                    ret = true;
                    break;
            }
            return true;
        }
        return false;
    });

    return ret;
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

    std::vector<Playlist*> playlist_arr = get_playlists();

    for(const auto p : playlist_arr)
        options.push_back(p->get_name());
 
    UIMenu menu("Playlists", options, [&] {
        menu.get_screen().ExitLoopClosure()();
    });

    menu.render([&] (ui::Event event) {
        if(event == ui::Event::Return) {
            song_queue_play_screen(get_songs_in_playlist(playlist_arr[menu.get_selected_id()]));
            return true;
        }
        return false;
    });
}

void App::song_queue_play_screen(const std::vector<Song*> queue) {
    if(queue.empty()) return;
    
    main_player.set_queue(queue);

    std::vector<std::string> options;

    auto load_songs = [&] {
        options.clear();

        for(const auto s : queue) {
            song_bank.validate_song(s);

            std::string name = s->get_name();

            if(s->is_degraded())
            name.insert(0, 1, '%');
            else if(s->is_corrupted())
            name.insert(0, 1, '$'); 

            options.push_back(name);
        }
    };

    load_songs();

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
            
            load_songs();
            menu.set_options(options);
            menu.set_selected(main_player.select_next_song_looped());
            return true;
        }
        return false;
    }, true);
}


void App::song_play_screen(Song* song) {
    auto screen = ui::ScreenInteractive::TerminalOutput();

    std::string titolo_brano = song->get_name();

    std::pair<double, double> progresso(0.0f, 0.0f);
    int longest_bus = main_player.get_longest_bus_id();

    std::string stato = "Playing";
    Song* next_song = main_player.get_next_song();

    std::string prossimo_brano = "---";

    if (next_song) {
        prossimo_brano = next_song->get_name();

        if (next_song->get_state() == SongState::DEGRADED)
            prossimo_brano += " (DEGRADED)";
        else if (next_song->get_state() == SongState::CORRUPTED)
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

    auto pulsanti_riproduzione = ui::Container::Horizontal({
        btn_play,
        btn_pause,
        btn_stop,
    });

    ui::SliderOption<double> opzioni_progresso;
    opzioni_progresso.value = &progresso.first;
    opzioni_progresso.min = 0.0;
    opzioni_progresso.max = &progresso.second;
    opzioni_progresso.increment = 1.0;

    auto slider_progresso = ui::Slider<double>(opzioni_progresso);

    bool is_seeking = false;
    ui::Box slider_box;

    auto seek_catcher = ui::CatchEvent(slider_progresso, [&](ui::Event event) {
        if (!event.is_mouse())
            return false;

        auto& mouse = event.mouse();

        if (mouse.button == ui::Mouse::Left) {
            if (mouse.motion == ui::Mouse::Pressed &&
                slider_box.Contain(mouse.x, mouse.y)) {
                is_seeking = true;
            } else if (mouse.motion == ui::Mouse::Released && is_seeking) {
                is_seeking = false;
                main_player.set_playback_pos(progresso.first);
            }
        }

        return false;
    });

    auto controlli = ui::Container::Vertical({
        seek_catcher,
        pulsanti_riproduzione,
    });

    auto renderer = ui::Renderer(controlli, [&] {
        ui::Elements bus_rows;

        if (main_player.is_playing()) {
            if (!is_seeking) {
                progresso = main_player.get_bus_playback_info(longest_bus);
            }

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
        } else if (!main_player.is_paused()) {
            screen.ExitLoopClosure()();
        }

        return ui::vbox({
                   ui::text(titolo_brano) | ui::bold | ui::center,
                   ui::separatorEmpty(),

                   seek_catcher->Render() | ui::reflect(slider_box),

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
        
        if(song_bank.song_exists_by_name(nome_canzone)) {
            if(edit_song) {
                if(edit_song->get_name() != nome_canzone) {
                    Logger::get_instance().log_err("[SongEditor] unable to edit song '" + edit_song->get_name() + "' with new name '" + nome_canzone + "' since it already exists");
                    return;
                }
            } else {
                Logger::get_instance().log_err("[SongEditor] unable to create song '" + nome_canzone + "' since it already exists");
                return;
            }
        }

        std::string song_uid;
        Song* new_song;
        
        if(edit_song) {
            new_song = edit_song;
            song_uid = song_bank.get_song_uid(edit_song);

            edit_song->clear_tracks();
        } else {
            std::pair<std::string, Song*> song_and_uid = song_bank.create_song();

            new_song = song_and_uid.second;
            song_uid = song_and_uid.first;
        }
        
        new_song->set_name(nome_canzone);

        for(auto& t : tracce) {
            t.data.device_id = audio_devices_ids[t.device_index];
            new_song->add_track(t.data);
        }
            
        if(!song_bank.save_song_to_file(song_uid))
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
            song_bank.validate_song(s);

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
    }, true);
}


struct PlaylistEntryUI {
    Song* canzone;
    int canzone_index = 0;
    ui::Component componente;
};

void App::playlist_creation(Playlist* edit_playlist) {
    auto screen = ui::ScreenInteractive::TerminalOutput();
 
    std::string nome_playlist;
 
    std::vector<Song*> songs = song_bank.get_songs();
    std::vector<std::string> song_names;

    for(const auto i : songs)
        song_names.push_back(i->get_name());
 

    std::list<PlaylistEntryUI> canzoni_playlist;
 
    auto trova_indice_canzone = [&](const Song* canzone_id) -> int {
        auto it = std::find(songs.begin(), songs.end(), canzone_id);
        if (it != songs.end())
            return static_cast<int>(std::distance(songs.begin(), it));
        return 0;
    };
 
    auto container_canzoni = ui::Container::Vertical({});

    auto ricostruisci_ordine = [&] {
        for (auto& entry : canzoni_playlist)
            container_canzoni->Add(entry.componente);
    };
 
    std::function<ui::Component(std::list<PlaylistEntryUI>::iterator)> crea_ui_canzone;
    crea_ui_canzone = [&](std::list<PlaylistEntryUI>::iterator it) -> ui::Component {
        PlaylistEntryUI& entry = *it;
        entry.canzone_index = trova_indice_canzone(entry.canzone);
 
        auto dropdown_canzone = ui::Dropdown(&song_names, &entry.canzone_index);
 
        auto self_weak_box = std::make_shared<std::weak_ptr<ui::ComponentBase>>();
 
        auto btn_elimina = ui::Button("🗑 Delete", [self_weak_box, it, &canzoni_playlist] {
            if (auto self = self_weak_box->lock())
                self->Detach();
            canzoni_playlist.erase(it);
        });
 
        auto btn_su = ui::Button("↑", [it, &canzoni_playlist, &ricostruisci_ordine] {
            if (it != canzoni_playlist.begin()) {
                auto prev_it = std::prev(it);
                canzoni_playlist.splice(prev_it, canzoni_playlist, it);
                ricostruisci_ordine();
            }
        });
 
        auto btn_giu = ui::Button("↓", [it, &canzoni_playlist, &ricostruisci_ordine] {
            auto next_it = std::next(it);
            if (next_it != canzoni_playlist.end()) {
                canzoni_playlist.splice(std::next(next_it), canzoni_playlist, it);
                ricostruisci_ordine();
            }
        });
 
        auto pulsanti = ui::Container::Horizontal({
            btn_elimina,
            btn_su,
            btn_giu,
        });
 
        auto gruppo = ui::Container::Vertical({
            dropdown_canzone,
            pulsanti,
        });
 
        ui::Component componente = ui::Renderer(gruppo, [dropdown_canzone, btn_elimina,
                                                   btn_su, btn_giu] {
            return ui::vbox({
                       ui::hbox(ui::text("Song: ") | ui::dim, dropdown_canzone->Render()),
                       ui::separator(),
                       ui::hbox({
                           btn_elimina->Render(),
                           ui::filler(),
                           btn_su->Render(),
                           ui::text(" "),
                           btn_giu->Render(),
                       }),
                   }) |
                   ui::border;
        });
 
        entry.componente = componente;
        *self_weak_box = componente;
        return componente;
    };
 
    auto on_aggiungi_click = [&] {
        canzoni_playlist.emplace_back();
        auto it = std::prev(canzoni_playlist.end());
        container_canzoni->Add(crea_ui_canzone(it));
    };
 
    auto btn_aggiungi = ui::Button("+ Add Song", on_aggiungi_click);
 
    auto input_nome_playlist = ui::Input(&nome_playlist, "Playlist name...");
 
    auto on_salva_click = [&] {
        if(canzoni_playlist.empty() || nome_playlist.empty()) return;

        if(playlist_exists(nome_playlist)) {
            if(edit_playlist) {
                if(edit_playlist->get_name() != nome_playlist) {
                    Logger::get_instance().log_err("[PlaylistEditor] unable to edit playlist '" + edit_playlist->get_name() + "' with new name '" + nome_playlist + "' since it already exists");
                    return;
                }
            } else {
                Logger::get_instance().log_err("[PlaylistEditor] unable to create new playlist '" + nome_playlist + "' since it already exists");
                return;
            }
        }
        
        Playlist* new_playlist;
        std::string playlist_uid;

        if(edit_playlist) {
            new_playlist = edit_playlist;
            playlist_uid = get_playlist_uid(edit_playlist);

            edit_playlist->clear_songs();
        } else {
            std::pair<std::string, Playlist*> new_playlist_and_uid = create_playlist();

            new_playlist = new_playlist_and_uid.second;
            playlist_uid = new_playlist_and_uid.first;
        }

        new_playlist->set_name(nome_playlist);

        for (auto& entry : canzoni_playlist) {
            entry.canzone = songs[entry.canzone_index];
            new_playlist->add_song(song_bank.get_song_uid(entry.canzone));
        }

        if(!save_playlist_to_file(playlist_uid))
            Logger::get_instance().log_err("[PlaylistEditor]>>> Unable to create playlist '" + nome_playlist + "'. All changes will be discarded");
        else
            Logger::get_instance().log("[PlaylistEditor]>>> " + std::string((edit_playlist) ? "Edited" : "Created") + " new playlist '" + nome_playlist + "'");

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
        input_nome_playlist,
        btn_aggiungi,
        container_canzoni,
        bottoni_finali,
    });
 
    if(edit_playlist) {
        nome_playlist = edit_playlist->get_name();

        for(const auto& i : edit_playlist->get_songs()) {
            on_aggiungi_click();
            PlaylistEntryUI& entry = canzoni_playlist.back();
            entry.canzone = song_bank.get_song(i);
            entry.canzone_index = trova_indice_canzone(entry.canzone);
        }
    }


    auto renderer = ui::Renderer(layout_principale, [&] {
        return ui::vbox({
                   ui::text("Create new Playlist") | ui::bold | ui::center,
                   ui::separator(),
 
                   ui::hbox(ui::text("Playlist Name: ") | ui::dim, input_nome_playlist->Render()),
 
                   ui::separator(),
 
                   ui::hbox({
                       ui::text("Songs") | ui::bold,
                       ui::filler(),
                       btn_aggiungi->Render(),
                   }),
                   ui::separator(),
 
                   container_canzoni->Render() | ui::vscroll_indicator | ui::frame |
                       ui::size(ui::HEIGHT, ui::LESS_THAN, 20),
 
                   ui::separator(),
                   bottoni_finali->Render() | ui::center,
               }) |
               ui::size(ui::WIDTH, ui::GREATER_THAN, 60) |
               ui::border;
    });
 
    screen.Loop(renderer);
}

void App::playlist_editing() {
    std::vector<Playlist*> playlists = get_playlists();

    if(playlists.empty()) return;

    std::vector<std::string> options;

    auto load_playlists = [&] {
        options.clear();

        for(const auto s : playlists) {
            options.push_back(s->get_name());
        }
    };
    
    load_playlists();

    UIMenu menu("Choose a playlist to edit", options, [&] {
        menu.get_screen().ExitLoopClosure()();
    });

    menu.render([&] (ui::Event event) {
        if(event == ui::Event::Return) {
            playlist_creation(playlists[menu.get_selected_id()]);
            load_playlists();
            menu.set_options(options);

            return true;
        }
        return false;
    });
}
