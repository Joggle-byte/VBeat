#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>

#include "../include/logger.hpp"


void Logger::log(const std::string& line) {
    std::time_t now = std::time(nullptr);
    std::tm* local_time = std::localtime(&now);

    buffer.push_back("(" + std::to_string(local_time->tm_hour) + ":" + std::to_string(local_time->tm_min) + ") " + line); 
}

void Logger::log_err(const std::string& line) {
    std::time_t now = std::time(nullptr);
    std::tm* local_time = std::localtime(&now);

    buffer.push_back("$(" + std::to_string(local_time->tm_hour) + ":" + std::to_string(local_time->tm_min) + ") " + line); 
}

void Logger::log_warn(const std::string& line) {
    std::time_t now = std::time(nullptr);
    std::tm* local_time = std::localtime(&now);

    buffer.push_back("%(" + std::to_string(local_time->tm_hour) + ":" + std::to_string(local_time->tm_min) + ") " + line); 
}

void Logger::clear() { buffer.clear(); }

int Logger::write_to_file() {
    auto now = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm_buf{};
    
    #if defined(_WIN32)
        localtime_s(&tm_buf, &time_now);
    #else
        localtime_r(&time_now, &tm_buf);
    #endif

    auto format_time = [&tm_buf](const char* fmt) {
        std::ostringstream ss;
        ss << std::put_time(&tm_buf, fmt);
        return ss.str();
    };

    std::string combined_time = format_time("%Y-%m-%d__%H-%M-%S");

    std::string filename = log_file_dir + "/log_" + combined_time + ".txt";
    std::ofstream file(filename);

    if (!file.is_open()) {
        log_err("[Logger] unable to write log to file");
        return 1;
    }

    for (const auto& line : buffer) {
        file << line << "\n";
    }

    file.close();
    return 0;
}
