/**
 * @file Logger.cpp
 * Logger implementation
 */

#include "Logger.hpp"

#include <iostream>

class StdOutLogger : public Logger
{
public:
  StdOutLogger(std::string context, LogLevel level): Logger(context, level)
  {
  }

  ~StdOutLogger()
  {
  }

  void log(LogLevel level, std::string message) override
  {
    if (level < _level)
    {
      // don't log
      return;
    }

    if (level == ERROR)
    {
      std::cerr << logLevelStr(level) << contextStr() << " " << message << std::endl;
    }
    else
    {
      std::cout << logLevelStr(level) << contextStr() << " " << message << std::endl;
    }
  }
};

Logger *LoggerFactory::createLogger(std::string context)
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
