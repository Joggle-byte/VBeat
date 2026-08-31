#pragma once

#include <vector>
#include <string>
#include <functional>

#include <ftxui/ftxui.hpp>


class UIMenu {
public:
    UIMenu(const std::string& _title, std::vector<std::string> _options, std::function<void()> back_button_pressed);
    ~UIMenu();

    void render(std::function<bool(ftxui::Event event)> callback);

    int get_selected_id() const { return selected; }

    void select_next();

    ftxui::ScreenInteractive& get_screen() { return screen; }

private:

    int selected = 0;
    std::string title;
    std::vector<std::string> options;
    ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::TerminalOutput();

    std::function<void()> back_button_callback;

};