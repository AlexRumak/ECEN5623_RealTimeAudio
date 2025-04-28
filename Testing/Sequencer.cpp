/**
 * @file Sequencer.cpp
 */

#include "Sequencer.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <syslog.h>

#define ERROR 1

//////////////////// HELPER ////////////////////
void setCurrentThreadAffinity(int cpu)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    pthread_t self = pthread_self();
    if (pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset) != 0)
    {
        std::cerr << "Error setting CPU affinity" << std::endl;
        exit(ERROR);
    }
}

void setCurrentThreadPriority(int priority)
{
    sched_param sch;
    sch.sched_priority = priority;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sch) != 0)
    {
        std::cerr << "Error setting thread priority" << std::endl;
        exit(ERROR);
    }
}

//////////////////// SERVICE ////////////////////
void Service::_initializeService()
{
  setCurrentThreadAffinity(_affinity);
  setCurrentThreadPriority(_priority); 
  _running.store(true);
}

void Service::_doService()
{
  _initializeService();

  while (_running)
  {
    auto acquired = _releaseService.try_acquire_for(std::chrono::milliseconds(_period)); 
    
    if (!acquired)
    {
      syslog(LOG_WARNING, "Likely service overrun in service");
      continue;
    }

    auto start = std::chrono::high_resolution_clock::now();

    _function();

    auto stop = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    _executionTimeStats.Add({static_cast<double>(elapsed.count()) / 1000.0 });
  }
}

void Service::stop()
{
  _running.store(false);
  _service.join();
}

void Service::release()
{
  if (_serviceStarted)
  {
    _serviceStarted = true;
    _firstRelease = std::chrono::high_resolution_clock::now();
  }
  else
  {
    auto now = std::chrono::high_resolution_clock::now();
    auto time_released = std::chrono::duration_cast<std::chrono::microseconds>(now - _firstRelease).count();
    auto expected = std::chrono::microseconds(_releaseNumber * _period * 1000).count();
    _releaseStats.Add({static_cast<double>(time_released - expected) / 1000.0});
  }
  _releaseService.release();
  _releaseNumber++;
}

//////////////////// SEQUENCER ////////////////////
Sequencer::Sequencer(uint8_t period, uint8_t priority, uint8_t affinity):
  _period(period),
  _stats(StatTracker(1000))
{
  setCurrentThreadAffinity(affinity);
  setCurrentThreadPriority(priority);
}

void Sequencer::startServices(std::shared_ptr<std::atomic<bool>> keepRunning)
{
  using namespace std::chrono_literals;

  const auto start = std::chrono::high_resolution_clock::now();

  long iterations = 0;
  while(keepRunning->load())
  {
    auto now = std::chrono::high_resolution_clock::now();

    long uElapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
    double msElapsed = static_cast<double>(uElapsed) / 1000.0;
    double expectedMs = static_cast<double>(iterations * _period);

    _stats.Add({msElapsed - expectedMs});
    
    for(auto& service : _services)
    {
      if (_period * iterations == service->getPeriod())
      {
        service->release();
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(_period));

    iterations++;
  }
}

void Sequencer::stopServices()
{
  for(auto& service : _services)
  {
    service->stop();
  }

  // Print execution statistics
  for(auto& service : _services)
  {
    auto releaseStats = service->releaseStats();
    auto exeuctionTimeStats = service->executionTimeStats();

    std::cout << "Service " << service->serviceName() << " Execution Statistics\n";
    std::cout << "Execution Time Average: " << exeuctionTimeStats.GetAverageDurationMs() << "\n";
    std::cout << "Execution Time Max: " << exeuctionTimeStats.GetMaxVal() << "\n";
    std::cout << "Execution Time Min: " << exeuctionTimeStats.GetMinVal() << "\n";
    std::cout << "Release Time Average Error: " << releaseStats.GetAverageDurationMs();
    std::cout << std::endl;
  } 
}

void Sequencer::_checkPeriodCompatability(uint8_t servicePeriod)
{
  if (servicePeriod % _period != 0)
  {
    throw std::invalid_argument("Service not compatible with sequencer. The service period must be divisible by the sequencer period.");
  }
}