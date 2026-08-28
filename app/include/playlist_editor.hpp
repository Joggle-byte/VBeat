#pragma once

#include "playlist.hpp"


class PlaylistEditor {
public:

    PlaylistEditor() : playlist() {}
    PlaylistEditor(Playlist* p) : playlist(p) {}
    ~PlaylistEditor();

    void load_playlist(Playlist* p);
    bool load_playlist_from_file(const std::string& path);

    bool save_to_file(const std::string& path);

    void add_song(const std::string& song_path);
    void remove_song(int song_id);
    void set_name(const std::string& new_name) { playlist->set_name(new_name); }

    const std::string& get_name() const { return playlist->get_name(); }
    const std::vector<std::string>& get_songs() const { return playlist->get_songs(); }

private:
    Playlist* playlist;

};