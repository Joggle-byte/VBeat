#include "../include/song_editor.hpp"
#include <iostream>


SongEditor::~SongEditor() {}

void SongEditor::load_song(Song* s) {
    song = s;
}

bool SongEditor::load_song_from_file(const std::string& path) {
    Song* s = Song::create_from_file(path);

    if(!s) return false;

    song = s;
    return true;
}

bool SongEditor::save_to_file(const std::string& path) {
    return song->save_to_file(path);
}

int SongEditor::add_track(AudioTrack track) {
    return song->add_track(track);
}

bool SongEditor::remove_track(int track_id) {
    return song->remove_track(track_id);
}

void SongEditor::set_track_volume(int track_id, float vol) {
    song->set_track_volume(track_id, vol);
}

void SongEditor::set_track_filepath(int track_id, const std::string& path) {
    song->set_track_filepath(track_id, path);
}

void SongEditor::set_track_name(int track_id, const std::string& new_name) {
    song->set_track_name(track_id, new_name);
}

void SongEditor::set_track_device(int track_id, int device) {
    song->set_track_device(track_id, device);
}


void SongEditor::print_track_list() {
    std::cout << "=== " << get_name() << " Tracks ===\n";
    std::cout << "ID\tNAME\tPATH\tDEVICE\tVOL\n\n";

    std::vector<AudioTrack> tracks = get_tracks();

    for(size_t i = 0; i < tracks.size(); i++) {
        AudioTrack t = tracks[i];
        std::cout << i << "\t" << t.name << "\t" << t.file_path << "\t" << t.device_id << "\t" << t.volume << "\n";
    }
}