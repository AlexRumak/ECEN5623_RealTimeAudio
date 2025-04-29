
/**
 * @file Sequencer.hpp
 * This is a C++ version of a sequencer using sleeps rather than ISR timers. 
 * The sequencer should run on the main core and the RTCore should run the  
 **/
#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <atomic>
#include <functional>
#include <thread>
#include <semaphore>
#include <string>
#include "Stats.hpp"

class Service
{
public:
  Service(std::string serviceName, uint8_t period, uint8_t priority, uint8_t affinity) :
    _serviceName(serviceName),
    _period(period),
    _priority(priority),
    _affinity(affinity),
    _releaseStats(StatTracker(1000)),
    _executionTimeStats(StatTracker(1000)),
    _releaseService(0)
  {
    _service = std::jthread(&Service::_doService, this);
  }

  virtual ~Service() = default;

  void stop();
  void release();

  uint8_t getPeriod()
  {
    return _period;
  }

  StatTracker releaseStats()
  {
    return _releaseStats;
  }

  StatTracker executionTimeStats()
  {
    return _executionTimeStats;
  }

  std::string serviceName()
  {
    return _serviceName;
  }

protected:
  virtual void _serviceFunction() = 0;

private:
  void _initializeService();
  void _doService();

  // Constructor parameters - order matters
  std::function<void(void)> _function;
  std::string _serviceName;
  uint8_t _period;
  uint8_t _priority;
  uint8_t _affinity;
  std::jthread _service;
  StatTracker _releaseStats;
  StatTracker _executionTimeStats;
  long _releaseNumber;
  std::counting_semaphore<1> _releaseService;

  volatile std::atomic<bool> _serviceStarted = std::atomic<bool>(false);
  volatile std::atomic<bool> _running = std::atomic<bool>(true);
  volatile std::atomic<std::chrono::high_resolution_clock::time_point> _firstRelease;
};

class Sequencer
{
public:
  Sequencer(uint8_t period, uint8_t priority, uint8_t affinity);

  virtual ~Sequencer() = default;
  
  template<typename... Args>
  void addService(std::unique_ptr<Service> service)
  {
    _services.emplace_back(std::move(service));

    _checkPeriodCompatability(_services[_services.size() - 1]->getPeriod());
  }

  void startServices(std::shared_ptr<std::atomic<bool>> keepRunning);
  void stopServices();

protected:
  std::vector<std::unique_ptr<Service>> _services;
  uint8_t _period;
  StatTracker _stats;

  virtual void _waitForRelease() = 0;
  virtual void _initializeSequencer() = 0;

private:
  void _checkPeriodCompatability(uint8_t servicePeriod);
};



class SequencerFactory
{
public:
  SequencerFactory(uint8_t period, uint8_t priority, uint8_t affinity):
    _period(period),
    _priority(priority),
    _affinity(affinity)
  {
  }

  Sequencer* createISRSequencer();
  Sequencer* createSleepSequencer();
  
private:
  uint8_t _period;
  uint8_t _priority;
  uint8_t _affinity;
};