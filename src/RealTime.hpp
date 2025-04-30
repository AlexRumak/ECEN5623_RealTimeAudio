/**
 * @file RealTime.hpp
 * @brief Helper class for real-time configuration
 */
#pragma once

#include "Sequencer.hpp"
#include "Logger.hpp"

#include <memory>

enum SequencerType
{
  SEQUENCER_SLEEP,
  SEQUENCER_ISR
};

// Todo: Replace with static methods if class is unnecessary.
class RealTimeSettings
{
public:
  RealTimeSettings(SequencerType type, std::shared_ptr<LoggerFactory> factory):
    _sequencerType(type)
  {
    _factory = new SequencerFactory();
    _logger = factory->createLogger("RealTimeSettings");
    _loggerFactory = factory;
  }

  ~RealTimeSettings()
  {
    delete _factory;
    delete _logger;
  }

  /**
   * @brief Check if the system is configured for real-time operation, and set any options that can be set.
   */
  virtual void setRealtimeSettings() = 0;

  /**
   * @brief Create a sequencer object.
   * @return A pointer to the created sequencer object.
   */
  virtual Sequencer *createSequencer(uint8_t period, uint8_t priority, uint8_t affinity) = 0;

protected:
  SequencerType _sequencerType;
  SequencerFactory* _factory;
  std::shared_ptr<LoggerFactory> _loggerFactory;

private:
  Logger* _logger;
};

class SettingsParser
{
public:
  SettingsParser(int argc, char **argv)
  {
    _argc = argc;
    _argv = argv;
  }

  std::shared_ptr<RealTimeSettings> parseSettings();
private:
  int _argc;
  char **_argv;
};