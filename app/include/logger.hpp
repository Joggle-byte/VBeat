#pragma once

#include <string>
#include <vector>


class Logger {
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger& get_instance() {
        static Logger instance;
        return instance;
    }

    void log(const std::string& line);
    void log_err(const std::string& line);
    void log_warn(const std::string& line);
    void clear();

    int write_to_file();

    std::vector<std::string> get_log() const { return buffer; }

    void set_log_file_dir(const std::string& dir) { log_file_dir = dir; };

private:
    Logger() {}
    ~Logger() = default;

    std::vector<std::string> buffer;
    std::string log_file_dir;

};




