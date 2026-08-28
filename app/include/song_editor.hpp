#pragma once

#include "song.hpp"
#include <string>


class SongEditor {
public:

    SongEditor() : song() {}
    SongEditor(Song* s) : song(s) {}
    ~SongEditor();

    void load_song(Song* s);
    bool load_song_from_file(const std::string& path);

    bool save_to_file(const std::string& path);

    int add_track(AudioTrack track);
    bool remove_track(int track_id);

    void set_track_volume(int track_id, float vol);
    void set_track_filepath(int track_id, const std::string& path);
    void set_track_name(int track_id, const std::string& new_name);
    void set_track_device(int track_id, int device);

    /* INTERFACE PRINT UTILS */

    void print_track_list();


    void set_name(const std::string& new_name) { song->set_name(new_name); }

    const std::vector<AudioTrack>& get_tracks() const { return song->get_tracks(); }
    const std::string& get_name() const { return song->get_name(); }

private:
    Song* song = nullptr;

};