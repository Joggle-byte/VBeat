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

    void log(const std::string& line) { buffer.push_back(line); }
    void log_err(const std::string& line) { buffer.push_back("$" + line); }
    void log_warn(const std::string& line) { buffer.push_back("%" + line); }
    void clear() { buffer.clear(); }

    std::vector<std::string> get_log() const { return buffer; }

private:
    Logger() {}
    ~Logger() = default;

    std::vector<std::string> buffer;

};




