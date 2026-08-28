#include "../include/playlist_editor.hpp"


PlaylistEditor::~PlaylistEditor() {}

void PlaylistEditor::load_playlist(Playlist* p) {
    playlist = p;
}

bool PlaylistEditor::load_playlist_from_file(const std::string& path) {
    Playlist* p = Playlist::create_from_file(path);
    
    if(!p) return false;

    playlist = p;
    return true;
}

bool PlaylistEditor::save_to_file(const std::string& path) {
    return playlist->save_to_file(path);
}

void PlaylistEditor::add_song(const std::string& song_path) {
    playlist->add_song(song_path);
}

void PlaylistEditor::remove_song(int song_id) {
    playlist->remove_song(song_id);
}