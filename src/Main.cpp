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
  MicrophoneService(std::string id, uint8_t period, uint8_t priority, uint8_t affinity)
    : Service("microphone[" + id + "]", period, priority, affinity)
  {
    
  }

  ~MicrophoneService()
  {

  }

protected:
  void _serviceFunction() override
  {
    std::cout << "Hello World" << std::endl;
  }
};

class BeeperService : public Service
{
public:
  BeeperService(std::string id, uint8_t period, uint8_t priority, uint8_t affinity)
    : Service("beeper[" + id + "]", period, priority, affinity)
  {
    
  }

  ~BeeperService(){

  }

protected:
  void _serviceFunction() override
  {
    std::cout << "Hello World" << std::endl;
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
  int maxPriority = sched_get_priority_max(SCHED_FIFO);

  Sequencer* sequencer = realTimeSettings->createSequencer(10, maxPriority, SEQUENCER_CORE);

  // starts service threads instantly, but will not run anything
  auto serviceOne = std::make_unique<MicrophoneService>("1", 20, maxPriority, SERVICES_CORE); 
  auto serviceTwo = std::make_unique<BeeperService>("2", 20, maxPriority - 1, SERVICES_CORE);

  sequencer->addService(std::move(serviceOne));
  sequencer->addService(std::move(serviceTwo));

  sequencer->startServices(keepRunning);
  sequencer->stopServices();

  delete sequencer;
}

int main(int argc, char* argv[])
{
  std::shared_ptr<RealTimeSettings> realTimeSettings = RealTimeSettings::parseSettings(argc, argv);

  realTimeSettings->setRealtimeSettings();

  // Interrupt handler
  keepRunning = std::make_shared<std::atomic<bool>>(true);
  signal(SIGINT, interruptHandler);

  runSequencer(realTimeSettings);
}