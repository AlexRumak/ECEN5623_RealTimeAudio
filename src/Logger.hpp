/**
 * @file Logger.hpp
 * A logger that forwards logs to a low priority context.
 */
#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <syslog.h>

namespace logger {
enum LogLevel { ERROR = 0, INFO = 1, WARNING = 2, DEBUG = 3, TRACE = 4 };

enum LoggerType { STDOUT, SYSLOG, FILE };

inline std::string logLevelStr(LogLevel level) {
  switch (level) {
  case ERROR:
    return "[ERROR]";
  case INFO:
    return "[INFO]";
  case WARNING:
    return "[WARNING]";
  case TRACE:
    return "[TRACE]";
  case DEBUG:
    return "[DEBUG]";
  default:
    return "[UNKNOWN]";
  }
}

class Logger {
public:
  Logger(std::string context, LogLevel level) {
    _context = context;
    _level = level;
  }
  virtual ~Logger() = default;
  virtual void log(LogLevel level, std::string message) = 0;

  logger::LogLevel baseLevel() { return _level; }

protected:
  virtual std::string contextStr() {
    if (cachedContext != "") {
      return cachedContext;
    }

    std::stringstream stream;
    stream << "[" << _context << "]";
    cachedContext = stream.str();

    return cachedContext;
  }

  LogLevel _level;
  std::string _context;

private:
  std::string cachedContext = "";
};

class LoggerFactory {
public:
  LoggerFactory(LoggerType type, LogLevel level) {
    _loggerType = type;
    _loglevel = level;

    if (_loggerType == FILE) {
      file.open("log.txt", std::ios::app);
      if (!file.is_open()) {
        throw std::ios_base::failure("Failed to open log file");
      }
    }
  }

  ~LoggerFactory() {
    if (file.is_open()) {
      file.close();
    }
  }

  void clear() {
    buffer.str("");
    buffer.clear();
  }

  void flush() {
    // for each line
    std::string line;
    std::istringstream iss(buffer.str());
    while (std::getline(iss, line)) {
      if (_loggerType == STDOUT) {
        std::cout << line << std::endl;
      } else if (_loggerType == SYSLOG) {
        syslog(LOG_INFO, "%s", line.c_str());
      } else if (_loggerType == FILE) {
        if (file.is_open()) {
          file << line << std::endl;
        }
      } else {
        throw std::runtime_error("Not implemented yet");
      }
    }
  }

  Logger *createLogger(std::string context);

  void writeLn(LogLevel loglevel, std::string context, std::string str) {
    buffer << logLevelStr(loglevel) << context << " " << str << "\n";
  }

private:
  std::stringstream buffer;
  LoggerType _loggerType;
  LogLevel _loglevel;
  std::ofstream file;
};
} // namespace logger
