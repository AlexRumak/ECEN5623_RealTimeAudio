/**
 * @file Main.cpp
 * @brief Main file for the real-time audio sequencer. 
 */

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
  MicrophoneService(std::string id, uint8_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<LoggerFactory> loggerFactory)
    : Service("microphone[" + id + "]", period, priority, affinity, loggerFactory)
  {
    
  }

  ~MicrophoneService()
  {

  }

protected:
  void _serviceFunction() override
  {
  }
};

class BeeperService : public Service
{
public:
  BeeperService(std::string id, uint8_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<LoggerFactory> loggerFactory)
    : Service("beeper[" + id + "]", period, priority, affinity, loggerFactory)
  {
    
  }

  ~BeeperService(){

  }

protected:
  void _serviceFunction() override
  {
  }
};

class LogsToFileService : public Service
{
public:
  LogsToFileService(std::string id, uint8_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<LoggerFactory> loggerFactory)
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
  std::shared_ptr<LoggerFactory> loggerFactory = realTimeSettings->getLoggerFactory();

  int maxPriority = sched_get_priority_max(SCHED_FIFO);
  int minPriority = sched_get_priority_min(SCHED_FIFO);

  Sequencer* sequencer = realTimeSettings->createSequencer(10, maxPriority, SEQUENCER_CORE);

  // starts service threads instantly, but will not run anything
  // TODO: Create pattern that creates services while adding them to the sequencer, as this prevents dangling threads.
  auto serviceOne = std::make_unique<MicrophoneService>("1", 20, maxPriority, SERVICES_CORE, loggerFactory); 
  auto serviceTwo = std::make_unique<BeeperService>("2", 20, maxPriority - 1, SERVICES_CORE, loggerFactory);
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