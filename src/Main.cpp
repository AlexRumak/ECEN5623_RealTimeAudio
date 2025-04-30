/**
 * @file Main.cpp
 * @brief Main file for the real-time audio sequencer. 
 */

#include "AudioBuffer.hpp"
#include "Microphone.hpp"
#include "Sequencer.hpp"
#include "Fib.hpp"
#include "RealTime.hpp"

#include <csignal>
#include <iostream>

#define SEQUENCER_CORE 2
#define SERVICES_CORE 3

#define TENMS 10
#define TWENTYMS 20
#define SEQ 115900

FibonacciLoadGenerator fib10(SEQ, TENMS);
FibonacciLoadGenerator fib20(SEQ, TWENTYMS);

class MicrophoneService : public Service
{
public:
  MicrophoneService(std::string id, uint16_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<logger::LoggerFactory> loggerFactory, std::shared_ptr<AudioBuffer> audioBuffer, std::shared_ptr<Microphone> microphone)
    : Service("microphone[" + id + "]", period, priority, affinity, loggerFactory)
  {
    _audioBuffer = audioBuffer;
    _microphone = microphone;
  }

  ~MicrophoneService()
  {
  }

protected:
  void _serviceFunction() override
  {
    
  }

private:
  std::shared_ptr<AudioBuffer> _audioBuffer;
  std::shared_ptr<Microphone> _microphone;
};

// TODO: Replace with LED service
class BeeperService : public Service
{
public:
  BeeperService(std::string id, uint16_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<logger::LoggerFactory> loggerFactory, std::shared_ptr<AudioBuffer> audioBuffer)
    : Service("beeper[" + id + "]", period, priority, affinity, loggerFactory)
  {
    _audioBuffer = audioBuffer;
  }

  ~BeeperService(){

  }

protected:
  void _serviceFunction() override
  {
  }

private:
  std::shared_ptr<AudioBuffer> _audioBuffer;
};

class LogsToFileService : public Service
{
public:
  LogsToFileService(std::string id, uint16_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<logger::LoggerFactory> loggerFactory)
    : Service("logstofile[" + id + "]", period, priority, affinity, loggerFactory)
  {
    
  }

  ~LogsToFileService(){

  }

protected:
  void _serviceFunction() override
  {
  }
};

std::shared_ptr<std::atomic<bool>> keepRunning; 

void interruptHandler(int sig)
{
  if (sig == SIGINT)
    keepRunning->store(false);
}

void runSequencer(std::shared_ptr<RealTimeSettings> realTimeSettings)
{
  std::shared_ptr<logger::LoggerFactory> loggerFactory = realTimeSettings->getLoggerFactory();

  int maxPriority = sched_get_priority_max(SCHED_FIFO);
  int minPriority = sched_get_priority_min(SCHED_FIFO);

  Sequencer* sequencer = realTimeSettings->createSequencer(10, maxPriority, SEQUENCER_CORE);

  std::shared_ptr<AudioBuffer> audioBuffer = std::make_shared<AudioBuffer>();
  std::shared_ptr<Microphone> microphone = std::make_shared<Microphone>();

  // starts service threads instantly, but will not run anything
  // TODO: Create pattern that creates services while adding them to the sequencer, as this prevents dangling threads.
  auto serviceOne = std::make_unique<MicrophoneService>("1", 20, maxPriority, SERVICES_CORE, loggerFactory, audioBuffer, microphone); 
  auto serviceTwo = std::make_unique<BeeperService>("2", 20, maxPriority - 1, SERVICES_CORE, loggerFactory, audioBuffer);
  auto serviceThree = std::make_unique<LogsToFileService>("3", 200, minPriority, SERVICES_CORE, loggerFactory);

  sequencer->addService(std::move(serviceOne));
  sequencer->addService(std::move(serviceTwo));
  sequencer->addService(std::move(serviceThree));

  sequencer->startServices(keepRunning);
  sequencer->stopServices();

  delete sequencer;
}

int main(int argc, char **argv)
{
  SettingsParser *parser = new SettingsParser(argc, argv);

  auto realTimeSettings = parser->parseSettings();

  delete parser;

  realTimeSettings->setRealtimeSettings();

  // Interrupt handler
  keepRunning = std::make_shared<std::atomic<bool>>(true);
  signal(SIGINT, interruptHandler);

  runSequencer(realTimeSettings);
}