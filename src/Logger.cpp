/**
 * @file Logger.cpp
 * Logger implementation
 */

#include "Logger.hpp"

#include <syslog.h>

#include <iostream>

class StdOutLogger : public logger::Logger
{
public:
  StdOutLogger(std::string context, logger::LogLevel level): Logger(context, level)
  {
  }

  ~StdOutLogger()
  {
  }

  void log(logger::LogLevel level, std::string message) override
  {
    if (level > _level)
    {
      // don't log
      return;
    }

    if (level == logger::ERROR)
    {
      std::cerr << logLevelStr(level) << contextStr() << " " << message << std::endl;
    }
    else
    {
      std::cout << logLevelStr(level) << contextStr() << " " << message << std::endl;
    }
  }
};

class SyslogLogger : public logger::Logger
{
public:
  SyslogLogger(std::string context, logger::LogLevel level): Logger(context, level)
  {
  }

  ~SyslogLogger()
  {
  }

  void log(logger::LogLevel level, std::string message) override
  {
    if (level > _level)
    {
      // don't log
      return;
    }

    if (level == logger::ERROR)
    {
      syslog(LOG_ERR, "%s %s", contextStr().c_str(), message.c_str());
    }
    else if (level == logger::INFO)
    {
      syslog(LOG_INFO, "%s %s", contextStr().c_str(), message.c_str());
    }
    else if (level == logger::WARNING)
    {
      syslog(LOG_WARNING, "%s %s", contextStr().c_str(), message.c_str());
    }
    else
    {
      syslog(LOG_DEBUG, "%s %s", contextStr().c_str(), message.c_str());
    }
  }
};

// TODO: Implement Logger that uses a queue to send messages to a background thread at a lower priority.

logger::Logger *logger::LoggerFactory::createLogger(std::string context)
{
  if (_loggerType == STDOUT)
  {
    return new StdOutLogger(context, _loglevel);
  }
  else if (_loggerType == SYSLOG)
  {
    return new SyslogLogger(context, _loglevel);
  }
  else
  {
    throw std::logic_error("Logger type not implemented");
  }
}
