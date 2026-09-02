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
    if(selected < options.size() - 1)
        selected++;
    else selected = 0;
}

void UIMenu::render(std::function<bool(ftxui::Event event)> callback) {
    ui::MenuOption opzioni = ui::MenuOption::Vertical();
    opzioni.entries = &options;
    opzioni.selected = &selected;
 
    opzioni.entries_option.transform = [](const ui::EntryState& state) {
        ui::Element e = ui::text(state.label) | ui::size(ui::WIDTH, ui::EQUAL, 50);
 
        if (state.focused)
            e = e | ui::bgcolor(ui::Color::CornflowerBlue) | ui::color(ui::Color::White);
 
        if (state.active)
            e = e | ui::bold | ui::color(ui::Color::GreenLight);
 
        return e | ui::border;
    };
 
    auto menu = ui::Menu(opzioni);

    ui::ButtonOption stile_back;
    stile_back.transform = [](const ui::EntryState& state) {
        ui::Element e = ui::text(state.label) | ui::center | ui::size(ui::WIDTH, ui::EQUAL, 20);
 
        if (state.focused)
            e = e | ui::bgcolor(ui::Color::CornflowerBlue) | ui::color(ui::Color::White);
 
        return e | ui::border;
    };
 
    auto back_button = ui::Button("< Back", back_button_callback, stile_back);
 
    auto container = ui::Container::Vertical({
        menu,
        back_button,
    });

 
    auto renderer = ui::Renderer(container, [&] {
        return ui::vbox({
                   ui::text(title) | ui::bold | ui::center,
                   ui::separatorEmpty(),
                   menu->Render(),
                   ui::separatorEmpty(),
                   ui::separatorEmpty(),
                   back_button->Render(),
                   ui::separator(),
                   ui::text("Selected: " + options[selected]) | ui::dim,
               }) |
               ui::border;
    });
 
    auto app = ui::CatchEvent(renderer, callback);
 
    screen.Loop(app);
}