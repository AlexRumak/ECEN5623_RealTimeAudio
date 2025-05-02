/**
 * @file Logger.cpp
 * Logger implementation
 */

#include "Logger.hpp"

#include <syslog.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>

class FactoryLogger : public logger::Logger
{
public:
  FactoryLogger(logger::LoggerFactory *factory, std::string context, logger::LogLevel level): Logger(context, level)
  {
    _factory = factory;
  }

  ~FactoryLogger()
  {
  }

  void log(logger::LogLevel level, std::string message) override
  {
    if (level > _level)
    {
      // don't log
      return;
    }

    std::stringstream str;
    if (level == logger::ERROR)
    {
      str << logger::logLevelStr(level) << contextStr() << " " << message << std::endl;
    }
    else
    {
      str << logger::logLevelStr(level) << contextStr() << " " << message << std::endl;
    }

    _factory->writeLn(level, contextStr(), str.str());
  }

private:
  logger::LoggerFactory *_factory;
};

logger::Logger *logger::LoggerFactory::createLogger(std::string context)
{
  return new FactoryLogger(this, context, _loglevel);
}
