/**
 * @file Logger.hpp 
 * A logger that forwards logs to a low priority context.
 */

#include <string>

enum LogLevel
{
  INFO,
  WARNING,
  ERROR,
  TRACE
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
  Logger(std::string context)
  {
    _context = context;
  }
  virtual ~Logger() = default;
  virtual void log(LogLevel level, std::string message) = 0;
private:
  std::string _context;
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