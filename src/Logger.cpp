/**
 * @file Logger.cpp
 * Logger implementation
 */

#include "Logger.hpp"

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

// TODO: Implement Logger that uses a queue to send messages to a background thread at a lower priority.

logger::Logger *logger::LoggerFactory::createLogger(std::string context)
{
  if (_loggerType == STDOUT)
  {
    return new StdOutLogger(context, _loglevel);
  }
  else
  {
    throw std::logic_error("Logger type not implemented");
  }
}
