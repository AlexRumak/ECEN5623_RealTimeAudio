/**
 * @file Logger.cpp
 * Logger implementation
 */

#include "Logger.hpp"

#include <iostream>

class StdOutLogger : public Logger
{
public:
  StdOutLogger(std::string context): Logger(context)
  {
  }

  ~StdOutLogger()
  {
  }

  void log(LogLevel level, std::string message) override
  {
    if (level == ERROR)
    {
      std::cerr << message << std::endl;
    }
    else
    {
      std::cout << message << std::endl;
    }
  }
};

Logger *LoggerFactory::createLogger(std::string context)
{
  if (_loggerType == STDOUT)
  {
    return new StdOutLogger(context);
  }
  else
  {
    throw std::logic_error("Logger type not implemented");
  }
}
