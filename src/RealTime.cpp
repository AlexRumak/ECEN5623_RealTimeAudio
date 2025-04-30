
#include "RealTime.hpp"

#include <iostream>
#include <csignal>
#include <fstream>
#include <sstream>

#define GENERIC_ERROR 1
#define COULD_NOT_OPEN_BOOT_OPTIONS "Could not open boot options"
#define MUST_RUN_AS_ROOT "Must run as root"

//////////////////// REAL TIME CONFIGURATION  ////////////////////
void checkSudo();
void checkBootSettings();
void setPerformanceMode(int core);

void RealTimeSettings::setRealtimeSettings()
{
  checkSudo();
  checkBootSettings();
}

Sequencer *RealTimeSettings::createSequencer(uint8_t period, uint8_t priority, uint8_t affinity)
{
  if (_factory == nullptr)
  {
    throw std::runtime_error("Factory not initialized");
  }

  if (_sequencerType == SEQUENCER_SLEEP)
  {
    return _factory->createSleepSequencer(period, priority, affinity);
  }
  else if (_sequencerType == SEQUENCER_ISR)
  {
    return _factory->createISRSequencer(period, priority, affinity);
  }
  else
  {
    throw std::invalid_argument("Invalid sequencer type");
  }
}

void checkSudo()
{
  if (getuid() != 0)
  {
    throw std::runtime_error("Must run as root");
  }
}

void checkBootSettings()
{
  std::string fileName = "/boot/firmware/cmdline.txt";
  std::ifstream file(fileName);

  if (!file.is_open())
  {
    throw std::runtime_error(COULD_NOT_OPEN_BOOT_OPTIONS);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  std::cout << "Boot options: " << content << std::endl;
}