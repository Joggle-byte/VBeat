#include "../include/ui_menu.hpp"

namespace ui = ftxui;


UIMenu::UIMenu(const std::string& _title, std::vector<std::string> _options, std::function<void()> back_button_pressed) {
    title = _title;
    options = _options;
    selected = 0;
    back_button_callback = back_button_pressed;
}

UIMenu::~UIMenu() {
    options.clear();
}

void UIMenu::select_next() {
    if(selected < static_cast<int>(options.size()) - 1)
        selected++;
    else selected = 0;
}

void UIMenu::render(std::function<bool(ftxui::Event event)> callback) {
    ui::MenuOption opzioni = ui::MenuOption::Vertical();
    opzioni.entries = &options;
    opzioni.selected = &selected;
 
    opzioni.entries_option.transform = [](const ui::EntryState& state) {
        ui::Color bg_color;
        ui::Color color = ui::Color::White;
        bool bold = false;
 
        std::string text = state.label;

        if (text[0] == '$') {
            bg_color = ui::Color::Red;
            color = ui::Color::White;
            text = text.substr(1) + " (CORRUPTED)";
        } else if (text[0] == '%') {
            bg_color = ui::Color::Yellow;
            color = ui::Color::White;
            text = text.substr(1) + " (DEGRADED)";
        }

        if (state.focused) {
            bg_color = ui::Color::CornflowerBlue;
            color = ui::Color::White;
            bold = true;
        }
 
        if (state.active) {
            color = ui::Color::GreenLight;
            bold = true;
        }
 
        ui::Element e = ui::text(text) | ui::size(ui::WIDTH, ui::EQUAL, 50) | ui::color(color) | ui::bgcolor(bg_color);
        if(bold) e |= ui::bold;

        return e | ui::border;
    };
 
    auto menu = ui::Menu(opzioni);

    ui::ButtonOption stile_back;
    stile_back.transform = [](const ui::EntryState& state) {
        ui::Element e = ui::text(state.label) | ui::center | ui::size(ui::WIDTH, ui::EQUAL, 20);

        if(state.focused)
            e = e | ui::bgcolor(ui::Color::CornflowerBlue) | ui::color(ui::Color::White);

        return e | ui::border;
    };
 
    auto back_button = ui::Button("< Back", back_button_callback, stile_back);
 
    auto container = ui::Container::Vertical({
        menu,
        back_button,
    });

 
    auto renderer = ui::Renderer(container, [&] {
        std::string sel = options[selected];
        if(sel[0] == '$' || sel[0] == '%') sel = sel.substr(1);

        return ui::vbox({
                   ui::text(title) | ui::bold | ui::center,
                   ui::separatorEmpty(),
                   menu->Render(),
                   ui::separatorEmpty(),
                   ui::separatorEmpty(),
                   back_button->Render(),
                   ui::separator(),
                   ui::text("Selected: " + sel) | ui::dim,
               }) |
               ui::border;
    });
 
    auto app = ui::CatchEvent(renderer, callback);
 
    screen.Loop(app);
}