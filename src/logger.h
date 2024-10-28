#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <mutex>
#include <ctime>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR
};

class Logger {
private:
    std::ofstream logFile;
    LogLevel logLevel;
    std::mutex mtx;

    // Constructor and destructor declared here, definitions below
    Logger(const std::string& filename, LogLevel level = LogLevel::INFO) : logLevel(level) {
        logFile.open(filename, std::ios::app);
        if (!logFile.is_open()) {
            throw std::runtime_error("Unable to open log file.");
        }
    }

    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    std::string getCurrentTime() {
        time_t now = time(0);
        struct tm timeInfo;
        char buf[80];
        localtime_s(&timeInfo, &now);
        strftime(buf, sizeof(buf), "%Y-%m-%d %X", &timeInfo);
        return std::string(buf);
    }

    std::string levelToString(LogLevel level) {
        switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
        }
    }

public:
    static Logger& getInstance(const std::string& filename = "log.txt", LogLevel level = LogLevel::INFO) {
        static Logger instance(filename, level);
        return instance;
    }

    void setLogLevel(LogLevel level) {
        logLevel = level;
    }

    template<typename... Args>
    void log(LogLevel level, Args... args) {
        std::lock_guard<std::mutex> guard(mtx);

        if (level >= logLevel) {
            std::ostringstream oss;
            oss << getCurrentTime() << " [" << levelToString(level) << "] ";
            writeToStream(oss, std::forward<Args>(args)...);
            std::string logMessage = oss.str();

            // Output to console
            std::cout << logMessage << std::endl;

            // Output to file
            if (logFile.is_open()) {
                logFile << logMessage << std::endl;
            }
        }
    }

    template<typename... Args>
    void info(Args... args) {
        log(LogLevel::INFO, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warning(Args... args) {
        log(LogLevel::WARNING, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(Args... args) {
        log(LogLevel::ERROR, std::forward<Args>(args)...);
    }

private:
    template<typename T>
    void writeToStream(std::ostringstream& oss, T&& arg) {
        oss << std::forward<T>(arg);
    }

    template<typename T, typename... Args>
    void writeToStream(std::ostringstream& oss, T&& firstArg, Args&&... remainingArgs) {
        oss << std::forward<T>(firstArg);
        writeToStream(oss, std::forward<Args>(remainingArgs)...);
    }
};

// Define macros for simplified logging
#define LOG_INFO(...) Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARNING(...) Logger::getInstance().warning(__VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().error(__VA_ARGS__)

#endif  // LOGGER_H
