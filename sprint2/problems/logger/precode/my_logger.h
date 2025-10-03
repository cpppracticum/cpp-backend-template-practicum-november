#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>
#include <iostream>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    auto GetTime() const {
        std::lock_guard lock(mutex_);
        if (manual_ts_) {
            return *manual_ts_;
        }
        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&t_c), "%F %T");
        return oss.str();
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&t_c), "%Y_%m_%d");
        return oss.str();
    }

    void UpdateLogFile() {
        std::string current_date = GetFileTimeStamp();
        
        if (current_date != stored_date_ || !log_file_.is_open()) {
            if (log_file_.is_open()) {
                log_file_.close();
            }
            
            std::string filename = "/var/log/sample_log_" + current_date + ".log";
            log_file_.open(filename, std::ios::app);
            
            if (!log_file_.is_open()) {
                std::cerr << "Failed to open log file: " << filename << std::endl;
            } else {
                stored_date_ = current_date;
            }
        }
    }

    Logger() {
        UpdateLogFile();
    }
    
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard lock(mutex_);
        
        UpdateLogFile();
        
        if (log_file_.is_open()) {
            log_file_ << GetTimeStamp() << ": ";
            ((log_file_ << args), ...);
            log_file_ << std::endl;
        }
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard lock(mutex_);
        manual_ts_ = ts;
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    mutable std::mutex mutex_;
    std::string stored_date_;
    std::ofstream log_file_;
};
