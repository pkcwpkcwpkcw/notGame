#pragma once

#include <string>
#include <fstream>
#include <mutex>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR_LEVEL = 3,  // Renamed to avoid Windows macro conflict
    CRITICAL = 4
};

class Logger {
public:
    static void Initialize(const std::string& logFile = "notgate.log");
    static void Shutdown();
    
    static void SetMinLevel(LogLevel level);
    static void SetConsoleOutput(bool enabled);
    static void SetFileOutput(bool enabled);
    
    static void Log(LogLevel level, const std::string& message);
    static void Debug(const std::string& message);
    static void Info(const std::string& message);
    static void Warning(const std::string& message);
    static void Error(const std::string& message);
    static void Critical(const std::string& message);
    
private:
    static std::string GetTimestamp();
    static std::string GetLevelString(LogLevel level);
    static void WriteLog(LogLevel level, const std::string& message);
    
private:
    static std::ofstream s_logFile;
    static LogLevel s_minLevel;
    static bool s_consoleOutput;
    static bool s_fileOutput;
    static bool s_initialized;
    static std::mutex s_mutex;
};