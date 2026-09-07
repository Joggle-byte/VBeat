#include <ctime>

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