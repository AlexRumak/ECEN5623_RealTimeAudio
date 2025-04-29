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

void fibonacciTenService()
{
  fib10.GenerateLoad(); 
}

void fibonacciTwentyService()
{
  fib20.GenerateLoad();
}

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

  sequencer->addService(fibonacciTenService, "fib10", 20, maxPriority, SERVICES_CORE);
  sequencer->addService(fibonacciTwentyService, "fib20", 50, maxPriority - 1, SERVICES_CORE);

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