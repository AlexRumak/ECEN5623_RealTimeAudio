/**
 * @file RealTime.hpp
 * @brief Helper class for real-time configuration
 */
#pragma once

#include "Sequencer.hpp"

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
  RealTimeSettings(SequencerType type):
    _sequencerType(type)
  {
    _factory = new SequencerFactory();
  }

  ~RealTimeSettings()
  {
    delete _factory;
  }

  /**
   * @brief Check if the system is configured for real-time operation, and set any options that can be set.
   */
  void setRealtimeSettings();

  /**
   * @brief Create a sequencer object.
   * @return A pointer to the created sequencer object.
   */
  Sequencer *createSequencer(uint8_t period, uint8_t priority, uint8_t affinity);

  static std::shared_ptr<RealTimeSettings> parseSettings(int argc, char* argv[])
  {
    if (argc < 2)
    {
      std::cerr << "Usage: real_time <sleep|isr>" << std::endl;
      exit(1);
    }

    SequencerType sequencerType;
    std::string option = argv[1];
    if (option == "sleep")
    {
      sequencerType = SEQUENCER_SLEEP; 
    }
    else if (option == "isr")
    {
      sequencerType = SEQUENCER_ISR;
    }
    else
    {
      std::cerr << "Invalid option: " << option << std::endl;
      exit(1);
    }

    std::shared_ptr<RealTimeSettings> settings = std::make_shared<RealTimeSettings>(sequencerType);

    return settings;
  }

private:
  SequencerFactory* _factory;
  SequencerType _sequencerType; 
};