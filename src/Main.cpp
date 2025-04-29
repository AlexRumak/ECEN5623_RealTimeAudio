/**
 * @file Main.cpp
 * @brief Main file for the SleepBasedTimer experiment.
 */

#include "Sequencer.hpp"
#include "Fib.hpp"

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

void runSequencer(std::string sequencerType)
{
  int maxPriority = sched_get_priority_max(SCHED_FIFO);
  SequencerFactory factory(10, maxPriority, SEQUENCER_CORE);

  Sequencer* sequencer;

  if (sequencerType == "sleep")
  {
    sequencer = factory.createSleepSequencer();
  }
  else if (sequencerType == "isr")
  {
    sequencer = factory.createISRSequencer();
  }
  else
  {
    std::cerr << "Fatal error, not sleep | isr" << std::endl;
  }

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
  if (argc != 2) 
  {
    std::cout << "Usage: sequencer <sleep|isr>" << std::endl;
    exit(1);
  }

  std::string sequencerType = argv[1];

  keepRunning = std::make_shared<std::atomic<bool>>(true);
  signal(SIGINT, interruptHandler);

  runSequencer(sequencerType);
}