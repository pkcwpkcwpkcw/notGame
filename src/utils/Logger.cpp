#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

std::ofstream Logger::s_logFile;
LogLevel Logger::s_minLevel = LogLevel::DEBUG;
bool Logger::s_consoleOutput = true;
bool Logger::s_fileOutput = true;
bool Logger::s_initialized = false;
std::mutex Logger::s_mutex;

void Logger::Initialize(const std::string& logFile) {
    // Try-catch to handle any initialization errors
    try {
        std::lock_guard<std::mutex> lock(s_mutex);
        
        if (s_initialized) {
            return;
        }
        
        if (s_fileOutput && !logFile.empty()) {
            s_logFile.open(logFile, std::ios::out | std::ios::app);
            if (!s_logFile.is_open()) {
                std::cerr << "Failed to open log file: " << logFile << std::endl;
                s_fileOutput = false;
            }
        }
    
#ifdef _WIN32
        // Enable ANSI color codes on Windows console
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole != INVALID_HANDLE_VALUE) {
            DWORD mode;
            if (GetConsoleMode(hConsole, &mode)) {
                mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hConsole, mode);
            }
        }
#endif
        
        s_initialized = true;
        // Don't call Info here to avoid recursion
    } catch (const std::exception& e) {
        std::cerr << "Logger initialization exception: " << e.what() << std::endl;
        abort();
    } catch (...) {
        std::cerr << "Logger initialization unknown exception" << std::endl;
        abort();
    }
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(s_mutex);
    
    if (!s_initialized) {
        return;
    }
    
    Info("Logger shutting down");
    
    if (s_logFile.is_open()) {
        s_logFile.close();
    }
    
    s_initialized = false;
}

void Logger::SetMinLevel(LogLevel level) {
    s_minLevel = level;
}

void Logger::SetConsoleOutput(bool enabled) {
    s_consoleOutput = enabled;
}

void Logger::SetFileOutput(bool enabled) {
    s_fileOutput = enabled;
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (level < s_minLevel) {
        return;
    }
    
    WriteLog(level, message);
}

void Logger::Debug(const std::string& message) {
    Log(LogLevel::DEBUG, message);
}

void Logger::Info(const std::string& message) {
    Log(LogLevel::INFO, message);
}

void Logger::Warning(const std::string& message) {
    Log(LogLevel::WARNING, message);
}

void Logger::Error(const std::string& message) {
    Log(LogLevel::ERROR_LEVEL, message);
}

void Logger::Critical(const std::string& message) {
    Log(LogLevel::CRITICAL, message);
}

std::string Logger::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    
#ifdef _WIN32
    // Windows uses localtime_s for security
    struct tm timeinfo;
    localtime_s(&timeinfo, &time_t);
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
#else
    // POSIX systems use localtime
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
#endif
    
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string Logger::GetLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO ";
        case LogLevel::WARNING:  return "WARN ";
        case LogLevel::ERROR_LEVEL:    return "ERROR";
        case LogLevel::CRITICAL: return "CRIT ";
        default:                 return "UNKN ";
    }
}

void Logger::WriteLog(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(s_mutex);
    
    if (!s_initialized && (s_consoleOutput || s_fileOutput)) {
        Initialize();
    }
    
    std::string timestamp = GetTimestamp();
    std::string levelStr = GetLevelString(level);
    std::string logLine = "[" + timestamp + "] [" + levelStr + "] " + message;
    
    // Console output with colors
    if (s_consoleOutput) {
        std::ostream& output = (level >= LogLevel::ERROR_LEVEL) ? std::cerr : std::cout;
        
        // ANSI color codes
        const char* colorCode = "";
        const char* resetCode = "\033[0m";
        
        switch (level) {
            case LogLevel::DEBUG:    colorCode = "\033[90m"; break;  // Gray
            case LogLevel::INFO:     colorCode = "\033[37m"; break;  // White
            case LogLevel::WARNING:  colorCode = "\033[93m"; break;  // Yellow
            case LogLevel::ERROR_LEVEL:    colorCode = "\033[91m"; break;  // Red
            case LogLevel::CRITICAL: colorCode = "\033[95m"; break;  // Magenta
        }
        
        output << colorCode << logLine << resetCode << std::endl;
    }
    
    // File output (no colors)
    if (s_fileOutput && s_logFile.is_open()) {
        s_logFile << logLine << std::endl;
        s_logFile.flush();
    }
}