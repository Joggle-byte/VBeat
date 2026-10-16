#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <memory>
#include <utility>
#include <algorithm>

#include <list>

#include "../include/app.hpp"
#include "../include/utils.hpp"
#include "../include/json.hpp"
#include "../include/ui_menu.hpp"
#include "../include/logger.hpp"


using json = nlohmann::json;


App::~App() {
    song_bank.clear();
    play_bank.clear();
}

int App::main() {
    std::system("cls");

    Logger::get_instance().log("VBeat " + std::string(VBEAT_VERSION) + " - by Emanuele Alfieri\n");
    Logger::get_instance().log("============= Startup Log =============");

    auto config = load_config_file();

    if(config.failed) return 1;

    song_bank.load_all(config.song_bank_path);
    play_bank.load_all(config.play_bank_path, &song_bank);

    Logger::get_instance().log("=====================================\n");

    main_player.list_devices();

    main_loop();
    std::system("cls");

    return Logger::get_instance().write_to_file();
}

ConfigFile App::load_config_file() {
    std::ifstream file(std::string(CONFIG_FILE_PATH) + "config.json");

    ConfigFile ret;

    if (!file.is_open()) {
        std::cout << "[VBeat] unable to load config file. Exiting...";
        ret.failed = true;
        return ret;
    }

    json config;

    try {
        file >> config;

        ret.song_bank_path = config.at("song_bank_path").get<std::string>();
        ret.play_bank_path = config.at("playlist_bank_path").get<std::string>();

        Logger::get_instance().set_log_file_dir(config.at("log_file_dir").get<std::string>());
    } catch (const std::exception& e) {
        Logger::get_instance().log_err(
            std::string("[VBeat] unable to parse config file: ") + e.what()
        );
        ret.failed = true;
        return ret;
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
    UIMenu menu("VBeat Menu", {"Select Playlist", "Select Song", "+ Create Song", 
        "+ Create Playlist", "✎ Edit Song", "✎ Edit Playlist", "✎ Edit Song Markers", "🗎 View Log", 
        "🗘 Reload Songs & Playlists", "$⚠ Delete Song", "$⚠ Delete Playlist", "$⚠ Wipe Memory"}, [&] {
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
                    marker_editing_selection();
                    break;
                case 7:
                    show_log();
                    break;
                case 8:
                    Logger::get_instance().log(">>> RELOADING SONGS & PLAYLISTS...");
                    song_bank.reload_all();
                    play_bank.reload_all(&song_bank);
                    break;
                case 9:
                    song_delete();
                    break;
                case 10:
                    playlist_delete();
                    break;
                case 11:
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
    const int viewport_height = 30; // visible log height

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
        ui::Element e = ui::text(state.label) | ui::center | ui::size(ui::WIDTH, ui::EQUAL, 30);
 
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
    if(play_bank.empty()) return;

    std::vector<std::string> options;

    std::vector<Playlist*> playlist_arr = play_bank.get_playlists();

    for(const auto p : playlist_arr)
        options.push_back(p->get_name());
 
    UIMenu menu("Playlists", options, [&] {
        menu.get_screen().ExitLoopClosure()();
    });

    menu.render([&] (ui::Event event) {
        if(event == ui::Event::Return) {
            song_queue_play_screen(play_bank.get_songs_in_playlist(playlist_arr[menu.get_selected_id()], &song_bank));
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
            
            main_player.stop();

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

    bool is_first_frame = true;

    std::string titolo_brano = song->get_name();

    std::string device_name = AudioPlayer::get_device_name_from_id(song->get_device_id());

    if(device_name.empty())
        device_name = "⚠ DEVICE UNAVAILABLE";

    std::pair<double, double> progresso(0.0f, 0.0f);
    int longest_bus = main_player.get_longest_bus_id();

    std::string stato = "Playing";
    Song* next_song = main_player.get_next_song();

    std::string prossimo_brano = "---";

    std::vector<Marker> marker_layout = main_player.get_current_marker_layout(); // sorted
    std::string marker_line;
    int current_marker_idx = -1;


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

    auto on_prev_marker = [&] {
        if(marker_layout.empty()) return;

        if(current_marker_idx < 0) {
            main_player.set_playback_pos(marker_layout.back().get_timestamp() - 1.0f);
            return;
        };

        if(current_marker_idx > 0) {
            current_marker_idx--;
            main_player.set_playback_pos(marker_layout[current_marker_idx].get_timestamp() - 1.0f);
        } else {
            main_player.set_playback_pos(0.0f);
        }
    };
    auto on_next_marker = [&] {
        if(marker_layout.empty()) return;
        
        if(progresso.first > marker_layout.back().get_timestamp()) {
            main_player.set_playback_pos(progresso.second);
            return;
        }

        if(current_marker_idx < 0) return;

        main_player.set_playback_pos(marker_layout[current_marker_idx].get_timestamp());
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

    auto btn_prev_marker = ui::Button("<<<", on_prev_marker, opzioni_bottone);
    auto btn_next_marker = ui::Button(">>>", on_next_marker, opzioni_bottone);

    auto pulsanti_riproduzione = ui::Container::Horizontal({
        btn_play,
        btn_pause,
        btn_stop,
    });

    auto pulsanti_marker = ui::Container::Horizontal({
        btn_prev_marker,
        btn_next_marker,
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


    auto get_channel_name_from_index = [] (int idx) -> std::string {
        int first = idx * 2 + 1;
        return std::to_string(first) + "-" + std::to_string(first + 1);
    };


    int bus_scroll_offset = 0;
    ui::Box bus_list_box;

    auto lista_bus_component = ui::Renderer([&] {
        ui::Elements bus_rows;

        if (main_player.is_playing()) {
            auto bus_levels = main_player.get_bus_levels();

            for (size_t i = 0; i < main_player.bus_count(); i++) {
                const AudioTrack& track = main_player.get_bus(i)->get_track();
                std::string bus_channel;

                bus_channel = get_channel_name_from_index(track.channel_index);

                bus_rows.push_back(
                    ui::hbox({
                        ui::text(std::to_string(i + 1) + ". " + track.name) | ui::size(ui::WIDTH, ui::EQUAL, 20),
                        ui::text(" → " + bus_channel) | ui::size(ui::WIDTH, ui::EQUAL, 10) | ui::dim,
                        ui::separatorEmpty(),
                        ui::vbox({
                            ui::gauge(bus_levels[i].first) | ui::color(ui::Color::Cyan),
                            ui::separatorEmpty(),
                            ui::gauge(bus_levels[i].second) | ui::color(ui::Color::Cyan),
                            ui::separatorEmpty(),
                        }) | ui::flex,
                        ui::filler() | ui::flex,

                        ui::separator(),
                    })
                );
            }
        }

        return ui::vbox(std::move(bus_rows))
            | ui::focusPosition(0, bus_scroll_offset)  // scorri per mostrare questa riga
            | ui::vscroll_indicator                     // la "sidebar" con la barra
            | ui::frame                                 // clippa + abilita lo scroll
            | ui::size(ui::HEIGHT, ui::LESS_THAN, 18);   // altezza massima visibile
    });

    auto lista_bus_scrollabile = ui::CatchEvent(lista_bus_component, [&](ui::Event event) {
        if (!event.is_mouse())
            return false;

        auto& mouse = event.mouse();
        if (!bus_list_box.Contain(mouse.x, mouse.y))
            return false;

        if (mouse.button == ui::Mouse::WheelDown) {
            bus_scroll_offset = std::min(bus_scroll_offset + 3,
                                        std::max(0, (int)main_player.bus_count() * 6));
            return true;
        }
        if (mouse.button == ui::Mouse::WheelUp) {
            bus_scroll_offset = std::max(bus_scroll_offset - 3, 0);
            return true;
        }
        return false;
    });


    auto controlli = ui::Container::Vertical({
        seek_catcher,
        lista_bus_scrollabile,
        pulsanti_riproduzione,
        pulsanti_marker,
    });

    auto build_marker_line = [&] {
        if(marker_layout.empty()) return;

        int width = slider_box.x_max - slider_box.x_min + 1;

        std::string buffer(width, ' ');

        std::vector<int> marker_idx;
        std::vector<std::string> marker_names;

        marker_idx.reserve(marker_layout.size() + 1);

        marker_idx.push_back(-1);

        for(const auto& i : marker_layout) {
            int idx = static_cast<int>(std::floor((i.get_timestamp() / progresso.second) * width));
            buffer[idx] = '|';

            marker_idx.push_back(idx);
            marker_names.push_back(i.get_name());
        }

        for(int i = 1; i < static_cast<int>(marker_idx.size()); i++) {
            int start = marker_idx[i - 1] + 1;
            int end = std::max(0, marker_idx[i] - 1);
            int len = end - start + 1;
            int half = static_cast<int>(std::floor(len / 2));
            std::string& name = marker_names[i - 1];

            if(len <= static_cast<int>(name.size())) {
                for(int j = start; j <= end; j++) {
                    buffer[j] = name[j - start];
                }
            } else {
                int name_half = static_cast<int>(std::floor(name.size() / 2));
                
                int local_start = half - name_half;
                int local_end = half - name_half + name.size() - 1;

                for(int j = local_start; j <= local_end; j++) {
                    buffer[start + j] = name[j - local_start];
                }
            }

        }

        marker_line = buffer;
    };

    auto get_current_marker = [&] (double current_time) -> int {
        if(marker_layout.empty()) return -1;

        for(int i = 0; i < static_cast<int>(marker_layout.size()); i++) {
            if(current_time < marker_layout[i].get_timestamp()) {
                return i;
            }
        }

        return -1;
    };

    auto renderer = ui::Renderer(controlli, [&] {
        if (!is_seeking) {
            progresso = main_player.get_bus_playback_info(longest_bus);
        }

        if (main_player.is_playing()) {
            ui::animation::RequestAnimationFrame();
        } else if (!main_player.is_paused()) {
            screen.ExitLoopClosure()();
        }

        if(!is_first_frame) build_marker_line();

        current_marker_idx = get_current_marker(progresso.first);
        std::string current_section_name = (current_marker_idx >= 0) ? marker_layout[current_marker_idx].get_name() : "---";

        is_first_frame = false;

        return ui::vbox({
                ui::text(titolo_brano) | ui::bold | ui::center,
                ui::separatorEmpty(),

                ui::text(marker_line) | ui::bold | ui::bgcolor(ui::Color::GreenLight) | ui::color(ui::Color::Black),
                seek_catcher->Render() | ui::reflect(slider_box),

                ui::separatorEmpty(),

                ui::text(format_time_to_minutes(progresso.first) + " | " + format_time_to_minutes(progresso.second))
                    | ui::center,

                ui::separatorEmpty(),
                ui::text("Section: " + current_section_name) | ui::bold | ui::center | ui::color(ui::Color::GreenLight),

                ui::separatorEmpty(),
                ui::text("→ " + device_name) | ui::center | ui::dim,

                ui::separator(),

                lista_bus_scrollabile->Render() | ui::reflect(bus_list_box),

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

                ui::separatorEmpty(),

                ui::hbox({
                    ui::filler(),
                    btn_prev_marker->Render(),
                    ui::text(" "),
                    btn_next_marker->Render(),
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
    int output_index = 0;   // indice della coppia di canali (0 -> "1-2", ecc.)
};

void App::song_creation(Song* edit_song) {
    auto screen = ui::ScreenInteractive::TerminalOutput();

    std::string nome_canzone;
    if (edit_song) nome_canzone = edit_song->get_name();

    std::vector<Device> audio_devices = main_player.get_devices();
    std::vector<std::string> audio_devices_names;

    for (const auto& i : audio_devices)
        audio_devices_names.push_back(i.name);

    auto get_index_from_device = [&](const std::string& dev) -> int {
        for (size_t idx = 0; idx < audio_devices.size(); idx++) {
            if (audio_devices[idx].id == dev) return (int)idx;
        }
        return 0;
    };

    int device_index = 0;
    if (edit_song) device_index = get_index_from_device(edit_song->get_device_id());

    auto dropdown_device_canzone = ui::Dropdown(&audio_devices_names, &device_index);

    std::vector<std::string> uscita_labels;

    auto ricalcola_uscite = [&] {
        uscita_labels.clear();

        if (device_index < 0 || device_index >= (int)audio_devices.size())
            return;

        int uscite_stereo = main_player.get_device_audio_out_count(audio_devices[device_index].id);

        if (uscite_stereo <= 0) {
            uscita_labels.push_back("N/A");
            return;
        }

        for (int i = 0; i < uscite_stereo; i++) {
            int canale_sinistro = i * 2 + 1;
            int canale_destro = i * 2 + 2;
            uscita_labels.push_back(std::to_string(canale_sinistro) + "-" + std::to_string(canale_destro));
        }
    };

    int ultimo_device_index = device_index;

    ricalcola_uscite();

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

        // Dropdown dell'USCITA (non più del device): referenzia la lista
        // condivisa `uscita_labels`, che viene rigenerata ogni frame.
        auto dropdown_uscita = ui::Dropdown(&uscita_labels, &stato.output_index);

        // --- DELETE BUTTON (invariato) -----------------------------------
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
            dropdown_uscita,
            btn_elimina
        });

        ui::Component componente = ui::Renderer(gruppo, [&stato, input_nome, slider_volume,
                                                   input_percorso, dropdown_uscita,
                                                   btn_elimina] {
            return ui::vbox({
                       ui::hbox(ui::text("Name:    ") | ui::dim, input_nome->Render()),
                       ui::hbox(ui::text("Volume:  ") | ui::dim, slider_volume->Render() | ui::flex,
                            ui::text(" " + std::to_string(stato.data.volume))),
                       ui::hbox(ui::text("File:    ") | ui::dim, input_percorso->Render()),
                       ui::hbox(ui::text("Output:  ") | ui::dim, dropdown_uscita->Render()),
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
        if (tracce.empty() || nome_canzone.empty()) return;

        if (song_bank.song_exists_by_name(nome_canzone)) {
            if (edit_song) {
                if (edit_song->get_name() != nome_canzone) {
                    Logger::get_instance().log_err("[Song Editor] unable to edit song '" + edit_song->get_name() + "' with new name '" + nome_canzone + "' since it already exists");
                    return;
                }
            } else {
                Logger::get_instance().log_err("[Song Editor] unable to create song '" + nome_canzone + "' since it already exists");
                return;
            }
        }

        std::string song_uid;
        Song* new_song;

        if (edit_song) {
            new_song = edit_song;
            song_uid = song_bank.get_song_uid(edit_song);
            edit_song->clear_tracks();
        } else {
            std::pair<std::string, Song*> song_and_uid = song_bank.create_song();
            new_song = song_and_uid.second;
            song_uid = song_and_uid.first;
        }

        new_song->set_name(nome_canzone);

        if (device_index >= 0 && device_index < (int)audio_devices.size())
            new_song->set_device_id(audio_devices[device_index].id);

        for (auto& t : tracce) {
            t.data.channel_index = t.output_index;
            new_song->add_track(t.data);
        }

        if (!song_bank.save_song_to_file(song_uid))
            Logger::get_instance().log_err("[Song Editor] unable to save song '" + nome_canzone + "'. All changes will be discarded");
        else
            Logger::get_instance().log(((edit_song) ? "[Song Editor]>>> Edited new song '" : ">>> Created new song '") + nome_canzone + "'");

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
        dropdown_device_canzone,   // <-- nuovo: dropdown device a livello canzone
        btn_aggiungi,
        container_tracce,
        bottoni_finali,
    });

    if (edit_song) {
        for (auto& t : edit_song->get_tracks()) {
            on_aggiungi_click();
            UITrack& new_track = tracce.back();
            new_track.data = t;
            new_track.output_index = t.channel_index;
        }
    }

    auto renderer = ui::Renderer(layout_principale, [&] {
        if (device_index != ultimo_device_index) {
            ricalcola_uscite();
            ultimo_device_index = device_index;

            for (auto& t : tracce) {
                if (t.output_index >= (int)uscita_labels.size())
                    t.output_index = 0;
            }
        }

        return ui::vbox({
                   ui::text("Create New Song") | ui::bold | ui::center,
                   ui::separator(),

                   ui::hbox(ui::text("Song Name: ") | ui::dim, input_nome_canzone->Render()),
                   ui::hbox(ui::text("Device:    ") | ui::dim, dropdown_device_canzone->Render()),

                   ui::separator(),

                   ui::hbox({
                       ui::text("Tracks") | ui::bold,
                       ui::filler(),
                       btn_aggiungi->Render(),
                   }),
                   ui::separator(),

                   container_tracce->Render() | ui::vscroll_indicator | ui::frame |
                       ui::size(ui::HEIGHT, ui::LESS_THAN, 25),

                   ui::separator(),
                   bottoni_finali->Render() | ui::center,
               }) |
               ui::size(ui::WIDTH, ui::GREATER_THAN, 60) |
               ui::border;
    });

    screen.Loop(renderer);
}

void App::song_editing() {
    if(song_bank.empty()) return;

    std::vector<Song*> songs;

    std::vector<std::string> options;

    auto load_songs = [&] {
        songs = song_bank.get_songs();
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

void App::song_delete() {
    if(song_bank.empty()) return;

    std::vector<Song*> songs;

    std::vector<std::string> options;

    auto load_songs = [&] {
        songs = song_bank.get_songs();
        options.clear();

        for(const auto s : songs) {
            options.push_back(s->get_name());
        }
    };
    
    load_songs();

    UIMenu menu("Select the song to delete", options, [&] {
        menu.get_screen().ExitLoopClosure()();
    });

    menu.render([&] (ui::Event event) {
        if(event == ui::Event::Return) {
            
            Song* song = songs[menu.get_selected_id()];

            if(!songs.empty()) {
                song = songs[menu.get_selected_id()];
            } else return true;

            if(confirm_dialog("Are you sure you want to delete\nthe song '" + song->get_name() + "' ?")) {
                song_bank.delete_song(song);
                load_songs();

                if(options.empty()) {
                    menu.get_screen().ExitLoopClosure()();
                    return true;
                }

                menu.set_selected(0);
                menu.set_options(options);
            }

            return true;
        }
        return false;
    }, true);
}


struct UIMarker {
    std::string marker_name;
    std::string marker_timestamp;   // formato "MM:SS"
};

void App::marker_editing(Song* song) {
    if(!song) return;

    // Canzone corrotta/degradata: NON va caricata né riprodotta. L'editor
    // mostra e permette di modificare SOLO la lista dei marker (niente barra di
    // avanzamento, barra dei marker, play/pausa).
    const bool markers_only = song->is_corrupted() || song->is_degraded();

    std::string avviso_markers_only;
    if (markers_only)
        avviso_markers_only = std::string("⚠ ") + (song->is_corrupted() ? "CORRUPTED" : "DEGRADED")
                            + " song: playback disabled, markers only";

    auto screen = ui::ScreenInteractive::TerminalOutput();

    std::string titolo_brano = song->get_name();

    // --- Conversione "MM:SS" <-> secondi ------------------------------------

    // "MM:SS" -> secondi. Ritorna -1.0f se la stringa non è valida.
    auto timestamp_to_seconds = [](const std::string& ts) -> float {
        auto colon = ts.find(':');
        if (colon == std::string::npos) return -1.0f;

        std::string mm = ts.substr(0, colon);
        std::string ss = ts.substr(colon + 1);

        auto all_digits = [](const std::string& s) {
            return !s.empty() && std::all_of(s.begin(), s.end(),
                [](unsigned char c) { return std::isdigit(c) != 0; });
        };

        if (!all_digits(mm) || !all_digits(ss) || mm.size() > 4 || ss.size() > 2)
            return -1.0f;

        int minutes = std::stoi(mm);
        int seconds = std::stoi(ss);
        if (seconds >= 60) return -1.0f;

        return static_cast<float>(minutes * 60 + seconds);
    };

    // secondi -> "MM:SS" (troncato al secondo)
    auto seconds_to_timestamp = [](double secs) -> std::string {
        int total = std::max(0, static_cast<int>(std::floor(secs)));
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02d:%02d", total / 60, total % 60);
        return buf;
    };

    // --- Stato playback ------------------------------------------------------
    // In modalità markers_only il player non viene mai interrogato:
    // progresso resta (0, 0) e longest_bus non viene usato.

    std::pair<double, double> progresso(0.0, 0.0);   // (posizione, durata)
    int longest_bus = markers_only ? -1 : main_player.get_longest_bus_id();
    bool is_first_frame = true;

    // --- Stato dei marker ----------------------------------------------------

    std::list<UIMarker> marker_list;
    int prossimo_numero_marker = 1;

    std::string errore;                 // mostrato in rosso sotto i bottoni
    std::string marker_line;            // la barra dei marker già composta
    bool markers_dirty = true;          // vero se la barra va ricostruita
    int last_bar_width = -1;
    double last_bar_duration = -1.0;

    auto segna_modificato = [&] {
        markers_dirty = true;
        errore.clear();
    };

    // Chiusura dell'editor. In markers_only nessuna riproduzione è stata
    // avviata da qui, quindi non si tocca il player.
    auto chiudi_editor = [&] {
        if (!markers_only)
            main_player.stop();
        screen.ExitLoopClosure()();
    };

    // --- Slider di progresso (seek) -----------------------------------------

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

    // --- Bottoni di riproduzione ---------------------------------------------

    ui::ButtonOption opzioni_bottone;
    opzioni_bottone.transform = [](const ui::EntryState& state) {
        ui::Element e = ui::text(state.label) | ui::center | ui::size(ui::WIDTH, ui::EQUAL, 10);
        if (state.focused)
            e = e | ui::bgcolor(ui::Color::CornflowerBlue) | ui::color(ui::Color::White);
        return e | ui::border;
    };

    auto btn_play = ui::Button("▶", [&] {
        main_player.resume();
        ui::animation::RequestAnimationFrame();
    }, opzioni_bottone);

    auto btn_pause = ui::Button("⏸", [&] {
        main_player.pause();
    }, opzioni_bottone);

    auto pulsanti_riproduzione = ui::Container::Horizontal({
        btn_play,
        btn_pause,
    });

    // --- Lista dinamica dei marker -------------------------------------------

    ui::ButtonOption opzioni_elimina;
    opzioni_elimina.transform = [](const ui::EntryState& state) {
        ui::Element e = ui::text(" " + state.label + " ");
        if (state.focused)
            e = e | ui::bgcolor(ui::Color::Red) | ui::color(ui::Color::White);
        return e;
    };

    auto container_marker = ui::Container::Vertical({});

    auto crea_ui_marker = [&](std::list<UIMarker>::iterator it) -> ui::Component {
        UIMarker& stato = *it;

        ui::InputOption opzioni_input;
        opzioni_input.on_change = segna_modificato;

        auto input_nome = ui::Input(&stato.marker_name, "Marker name...", opzioni_input);
        auto input_timestamp = ui::Input(&stato.marker_timestamp, "MM:SS", opzioni_input);

        auto self_weak_box = std::make_shared<std::weak_ptr<ui::ComponentBase>>();

        // L'eliminazione viene DIFFERITA con screen.Post(): se distruggessimo il
        // componente direttamente qui, il bottone verrebbe liberato mentre il
        // suo stesso callback è ancora in esecuzione. Post() la esegue appena
        // l'evento corrente è stato gestito per intero.
        auto btn_elimina = ui::Button("🗑 Delete",
            [&screen, &prossimo_numero_marker, &marker_list, &segna_modificato, self_weak_box, it] {
                if(--prossimo_numero_marker < 1)
                    prossimo_numero_marker = 1;
                    

                screen.Post([&marker_list, &segna_modificato, self_weak_box, it] {
                    // Se il componente non esiste più (doppio click), non fare nulla:
                    // eseguire erase(it) due volte sarebbe undefined behavior.
                    if (auto self = self_weak_box->lock()) {
                        self->Detach();
                        marker_list.erase(it);
                        segna_modificato();
                    }
                });
            }, opzioni_elimina);

        auto gruppo = ui::Container::Horizontal({
            input_nome,
            input_timestamp,
            btn_elimina,
        });

        ui::Component componente = ui::Renderer(gruppo,
            [&stato, &progresso, timestamp_to_seconds, input_nome, input_timestamp, btn_elimina] {
                ui::Element ts_el = input_timestamp->Render() | ui::size(ui::WIDTH, ui::EQUAL, 7);

                // timestamp non valido -> in rosso
                float timestamp_seconds = timestamp_to_seconds(stato.marker_timestamp);

                // La durata è nota solo con la canzone caricata: in markers_only
                // progresso.second resta 0 e NON va usata come limite, altrimenti
                // ogni timestamp risulterebbe "oltre la fine".
                bool oltre_durata = progresso.second > 0.0 && timestamp_seconds > progresso.second;

                if (timestamp_seconds < 0.0f || oltre_durata)
                    ts_el = ts_el | ui::color(ui::Color::Red);

                return ui::hbox({
                           ui::text("Name: ") | ui::dim,
                           input_nome->Render() | ui::flex,
                           ui::text("  Time: ") | ui::dim,
                           ts_el,
                           ui::text("  "),
                           btn_elimina->Render(),
                       }) |
                       ui::border;
            });

        *self_weak_box = componente;
        return componente;
    };

    auto aggiungi_marker = [&](const std::string& nome, const std::string& timestamp) {
        marker_list.push_back({nome, timestamp});
        auto it = std::prev(marker_list.end());
        container_marker->Add(crea_ui_marker(it));
        segna_modificato();
    };

    // Il bottone inserisce un marker alla posizione corrente della canzone
    // (in markers_only la posizione è sempre 00:00: si modifica a mano).
    auto on_aggiungi_click = [&] {
        aggiungi_marker("Marker " + std::to_string(prossimo_numero_marker++),
                        seconds_to_timestamp(progresso.first));
    };

    auto btn_aggiungi = ui::Button("+ Add Marker", on_aggiungi_click);

    // TODO (modifica di marker esistenti): precarica qui i marker della canzone
    // con una chiamata per ognuno, es.
    for(const auto& i : song->get_markers()) {
        auto m = i.second;
        aggiungi_marker(m.get_name(), seconds_to_timestamp(m.get_timestamp()));
    }

    // --- Salvataggio / annullamento ------------------------------------------

    auto on_salva_click = [&] {
        std::vector<std::pair<std::string, float>> markers_da_salvare;   // (nome, secondi)

        for (const auto& m : marker_list) {
            float ts = timestamp_to_seconds(m.marker_timestamp);

            if (m.marker_name.empty() || ts < 0.0f) {
                errore = "Invalid marker: '" + m.marker_name + "' (" + m.marker_timestamp + ")";
                return;
            }
            if (progresso.second > 0.0 && ts > progresso.second) {
                errore = "Marker '" + m.marker_name + "' is beyond the end of the song";
                return;
            }

            markers_da_salvare.emplace_back(m.marker_name, ts);
        }

        std::stable_sort(markers_da_salvare.begin(), markers_da_salvare.end(),
                         [](const auto& a, const auto& b) { return a.second < b.second; });

        // TODO: collega qui il tuo backend. `markers_da_salvare` contiene
        // (nome, timestamp in secondi) già validato e ordinato per timestamp.
        song->clear_markers();

        for(const auto& i : markers_da_salvare) {
            song->add_marker(Marker(i.second, i.first));
        }

        auto song_uid = song_bank.get_song_uid(song);

        if(song_uid.empty()) {
            Logger::get_instance().log_err("[Marker Editor] unable to save song '" + song->get_name() + "' since it does not exist in song bank. All changes will be discarded");

            chiudi_editor();
            return;
        }

        if (!song_bank.save_song_to_file(song_uid))
            Logger::get_instance().log_err("[Marker Editor] unable to save song '" + song->get_name() + "'. All changes will be discarded");
        else
            Logger::get_instance().log("[Marker Editor]>>> Edited markers in song '" + song->get_name() + "'");

        chiudi_editor();
    };

    auto on_annulla_click = [&] {
        chiudi_editor();
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

    auto btn_salva = ui::Button("Save & Back", on_salva_click, stile_salva);
    auto btn_annulla = ui::Button("Cancel", on_annulla_click, stile_annulla);

    auto bottoni_finali = ui::Container::Horizontal({
        btn_salva,
        btn_annulla,
    });

    // --- Albero dei componenti -----------------------------------------------
    // In markers_only slider e play/pausa NON entrano nell'albero: così non
    // possono ricevere focus né eventi (un Invio su un "Play" invisibile
    // chiamerebbe resume() su una canzone mai caricata).

    auto layout_principale = ui::Container::Vertical({});

    if (!markers_only) {
        layout_principale->Add(seek_catcher);
        layout_principale->Add(pulsanti_riproduzione);
    }

    layout_principale->Add(btn_aggiungi);
    layout_principale->Add(container_marker);
    layout_principale->Add(bottoni_finali);

    // --- Barra dei marker -----------------------------------------------------
    // Ricostruita SOLO quando serve (vedi renderer): marker modificati,
    // larghezza dello slider cambiata o durata cambiata.

    auto build_marker_line = [&] {
        int width = slider_box.x_max - slider_box.x_min + 1;
        double duration = progresso.second;

        marker_line.clear();
        if (width <= 1 || duration <= 0.0) return;

        // Marker validi (timestamp parsabile e dentro la durata), ordinati.
        std::vector<std::pair<float, std::string>> validi;
        for (const auto& m : marker_list) {
            float ts = timestamp_to_seconds(m.marker_timestamp);
            if (ts >= 0.0f && ts <= duration)
                validi.emplace_back(ts, m.marker_name);
        }
        std::stable_sort(validi.begin(), validi.end(),
                         [](const auto& a, const auto& b) { return a.first < b.first; });

        std::string buffer(width, ' ');
        std::vector<int> pos;
        pos.reserve(validi.size());

        for (const auto& v : validi) {
            int idx = std::min(width - 1, static_cast<int>(std::floor(v.first / duration * width)));
            buffer[idx] = '|';
            pos.push_back(idx);
        }

        // Il nome di ogni marker occupa il segmento che lo PRECEDE, come in
        // song_play_screen().
        for (size_t i = 0; i < validi.size(); i++) {
            int start = (i == 0) ? 0 : pos[i - 1] + 1;
            int end = pos[i] - 1;
            int len = end - start + 1;
            if (len <= 0) continue;

            const std::string& name = validi[i].second;
            int name_len = static_cast<int>(name.size());

            if (len <= name_len) {
                for (int j = 0; j < len; j++)
                    buffer[start + j] = name[j];
            } else {
                int offset = start + (len - name_len) / 2;
                for (int j = 0; j < name_len; j++)
                    buffer[offset + j] = name[j];
            }
        }

        marker_line = buffer;
    };

    // --- Renderer -------------------------------------------------------------

    auto renderer = ui::Renderer(layout_principale, [&] {
        ui::Elements contenuto;

        contenuto.push_back(ui::text("Marker Editor - " + titolo_brano) | ui::bold | ui::center);
        contenuto.push_back(ui::separatorEmpty());

        if (markers_only) {
            contenuto.push_back(ui::text(avviso_markers_only) | ui::center | ui::color(ui::Color::Yellow));
            contenuto.push_back(ui::separator());
        } else {
            // --- Parte "player": solo con la canzone caricata -----------------
            if (!is_seeking)
                progresso = main_player.get_bus_playback_info(longest_bus);

            if (main_player.is_playing())
                ui::animation::RequestAnimationFrame();

            if (is_first_frame) {
                is_first_frame = false;
                ui::animation::RequestAnimationFrame();
            }

            int width = slider_box.x_max - slider_box.x_min + 1;
            if (markers_dirty || width != last_bar_width || progresso.second != last_bar_duration) {
                build_marker_line();
                last_bar_width = width;
                last_bar_duration = progresso.second;
                markers_dirty = false;
            }

            std::string stato = main_player.is_paused() ? "Paused"
                              : (main_player.is_playing() ? "Playing" : "Stopped");

            contenuto.insert(contenuto.end(), {
                ui::text(marker_line) | ui::bold | ui::bgcolor(ui::Color::GreenLight) | ui::color(ui::Color::Black),
                seek_catcher->Render() | ui::reflect(slider_box),

                ui::separatorEmpty(),

                ui::text(seconds_to_timestamp(progresso.first) + " | " + seconds_to_timestamp(progresso.second))
                    | ui::center,

                ui::separatorEmpty(),

                ui::hbox({
                    ui::filler(),
                    btn_play->Render(),
                    ui::text(" "),
                    btn_pause->Render(),
                    ui::filler(),
                }),

                ui::text("State: " + stato) | ui::center | ui::dim,

                ui::separator(),
            });
        }

        // --- Sezione marker: comune a entrambe le modalità --------------------
        contenuto.insert(contenuto.end(), {
            ui::hbox({
                ui::text("Markers") | ui::bold,
                ui::filler(),
                btn_aggiungi->Render(),
            }),
            ui::separator(),

            container_marker->Render() | ui::vscroll_indicator | ui::frame |
                ui::size(ui::HEIGHT, ui::LESS_THAN, 20),

            ui::separator(),
            bottoni_finali->Render() | ui::center,
            ui::text(errore) | ui::color(ui::Color::Red) | ui::center,
        });

        return ui::vbox(std::move(contenuto)) |
               ui::size(ui::WIDTH, ui::GREATER_THAN, 60) |
               ui::border;
    });

    screen.Loop(renderer);
}

void App::marker_editing_selection() {
    if(song_bank.empty()) return;

    main_player.clear_queue();

    std::vector<Song*> songs = song_bank.get_songs();

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

    UIMenu menu("Choose a song for Marker configuration", options, [&] {
        menu.get_screen().ExitLoopClosure()();
    });

    menu.render([&] (ui::Event event) {
        if(event == ui::Event::Return) {
            main_player.set_queue({songs[menu.get_selected_id()]});

            Song* s = main_player.get_queued_song(0);

            if(!s->is_corrupted() && !s->is_degraded()) {
                main_player.play(0);
            }

            marker_editing(s);

            main_player.clear_queue();

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

        if(play_bank.playlist_exists(nome_playlist)) {
            if(edit_playlist) {
                if(edit_playlist->get_name() != nome_playlist) {
                    Logger::get_instance().log_err("[Playlist Editor] unable to edit playlist '" + edit_playlist->get_name() + "' with new name '" + nome_playlist + "' since it already exists");
                    return;
                }
            } else {
                Logger::get_instance().log_err("[Playlist Editor] unable to create new playlist '" + nome_playlist + "' since it already exists");
                return;
            }
        }
        
        Playlist* new_playlist;
        std::string playlist_uid;

        if(edit_playlist) {
            new_playlist = edit_playlist;
            playlist_uid = play_bank.get_playlist_uid(edit_playlist);

            edit_playlist->clear_songs();
        } else {
            std::pair<std::string, Playlist*> new_playlist_and_uid = play_bank.create_playlist();

            new_playlist = new_playlist_and_uid.second;
            playlist_uid = new_playlist_and_uid.first;
        }

        new_playlist->set_name(nome_playlist);

        for (auto& entry : canzoni_playlist) {
            entry.canzone = songs[entry.canzone_index];
            new_playlist->add_song(song_bank.get_song_uid(entry.canzone));
        }

        if(!play_bank.save_playlist_to_file(playlist_uid))
            Logger::get_instance().log_err("[Playlist Editor]>>> Unable to create playlist '" + nome_playlist + "'. All changes will be discarded");
        else
            Logger::get_instance().log("[Playlist Editor]>>> " + std::string((edit_playlist) ? "Edited" : "Created") + " new playlist '" + nome_playlist + "'");

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
                       ui::size(ui::HEIGHT, ui::LESS_THAN, 25),
 
                   ui::separator(),
                   bottoni_finali->Render() | ui::center,
               }) |
               ui::size(ui::WIDTH, ui::GREATER_THAN, 60) |
               ui::border;
    });
 
    screen.Loop(renderer);
}

void App::playlist_editing() {
    if(play_bank.empty()) return;

    std::vector<Playlist*> playlists;

    std::vector<std::string> options;

    auto load_playlists = [&] {
        playlists = play_bank.get_playlists();
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

void App::playlist_delete() {
    if(play_bank.empty()) return;

    std::vector<Playlist*> playlists;

    std::vector<std::string> options;

    auto load_playlists = [&] {
        options.clear();

        playlists = play_bank.get_playlists();

        for(const auto s : playlists) {
            options.push_back(s->get_name());
        }
    };
    
    load_playlists();

    UIMenu menu("Select the playlist to delete", options, [&] {
        menu.get_screen().ExitLoopClosure()();
    });

    menu.render([&] (ui::Event event) {
        if(event == ui::Event::Return) {
            
            Playlist* play;

            if(!playlists.empty()) {
                play = playlists[menu.get_selected_id()];
            } else return true;

            if(confirm_dialog("Are you sure you want to delete\nthe playlist '" + play->get_name() + "' ?")) {
                play_bank.delete_playlist(play);
                load_playlists();

                if(options.empty()) {
                    menu.get_screen().ExitLoopClosure()();
                    return true;
                }

                menu.set_selected(0);
                menu.set_options(options);
            }

            return true;
        }
        return false;
    });
}
