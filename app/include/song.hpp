#pragma once

#include <string>
#include <vector>
#include <map>
#include <filesystem>

#include "audiotrack.hpp"

namespace fs = std::filesystem;


enum class SongState {
    OK,
    DEGRADED,
    CORRUPTED
};


class Marker {
public:
    Marker() {}
    Marker(double timestamp, const std::string& _name) :
        timestamp_seconds(timestamp),
        name(_name) {}
    
    const std::string get_name() const { return name; }
    double get_timestamp() const { return timestamp_seconds; }

private:
    double timestamp_seconds;
    std::string name;
};


class Song {
public:
    Song() {}
    Song(const std::string& _name) : name(_name) {}
    ~Song();

    int add_track(AudioTrack track);
    bool remove_track(int track_id);
    void clear_tracks() { tracks.clear(); }

    void set_track_volume(int track_id, float vol);
    void set_track_filepath(int track_id, const std::string& path);
    void set_track_name(int track_id, const std::string& new_name);
    void set_track_device(int track_id, const std::string& device);
    void set_track_state(int track_id, TrackState state);

    void set_name(const std::string& new_name) { name = new_name; }
    void set_state(SongState new_state) { state = new_state; }

    void add_marker(const Marker marker) { markers[marker.get_timestamp()] = marker; }

    const AudioTrack& get_track(int track_id);
    const std::vector<AudioTrack>& get_tracks() { return tracks; }
    const std::string& get_name() const;
    SongState get_state() const { return state; }
    TrackState get_track_state(int track_id) const;

    const std::map<double, Marker>& get_markers() const { return markers; }

    bool is_corrupted() const { return state == SongState::CORRUPTED; }
    bool is_degraded() const { return state == SongState::DEGRADED; }

    size_t get_tracks_count() const { return tracks.size(); }

    static Song* create_from_file(const fs::path& path);
    bool save_to_file(const fs::path& path);


private:
    std::string name;
    std::vector<AudioTrack> tracks;
    SongState state;

    std::map<double, Marker> markers;

    bool is_valid_track_id(int track_id) const;
};