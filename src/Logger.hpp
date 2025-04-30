/**
 * @file Logger.hpp 
 * A logger that forwards logs to a low priority context.
 */
#pragma once

#include <string>
#include <sstream>

enum LogLevel
{
  ERROR = 0,
  INFO = 1,
  WARNING = 2,
  TRACE = 3
};

enum LoggerType
{
  STDOUT,
  SYSLOG,
  BACKGROUND
};

class Logger
{
public:
  Logger(std::string context, LogLevel level)
  {
    _context = context;
    _level = level;
  }
  virtual ~Logger() = default;
  virtual void log(LogLevel level, std::string message) = 0;
protected:

  virtual std::string contextStr()
  {
    if (cachedContext != "")
    {
      return cachedContext;
    }

    std::stringstream stream;
    stream << "[" << _context << "]";
    cachedContext = stream.str();

    return cachedContext;
  }
  LogLevel _level;
  std::string _context;

  virtual std::string logLevelStr(LogLevel level)
  {
    switch (level)
    {
      case ERROR:
        return "[ERROR]";
      case INFO:
        return "[INFO]";
      case WARNING:
        return "[WARNING]";
      case TRACE:
        return "[TRACE]";
      default:
        return "[UNKNOWN]";
    }
  }

private:
  std::string cachedContext = "";
};

class LoggerFactory
{
public:
  LoggerFactory(LoggerType type, LogLevel level)
  {
    _loggerType = type;
    _loglevel = level;
  }

  Logger *createLogger(std::string context);

private:
  LoggerType _loggerType;
  LogLevel _loglevel;
};