
/**
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
  template<typename T>
  Service(T&& function, std::string serviceName, uint8_t period, uint8_t priority, uint8_t affinity):
    _function(function),
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

  volatile std::atomic<bool> _running = std::atomic<bool>(true);
  bool _serviceStarted = false;
  std::chrono::high_resolution_clock::time_point _firstRelease;
};

class Sequencer
{
public:
  Sequencer(uint8_t period, uint8_t priority, uint8_t affinity);
  
  template<typename... Args>
  void addService(Args&&... args)
  {
    _services.emplace_back(std::make_unique<Service>(std::forward<Args>(args)...));

    _checkPeriodCompatability(_services[_services.size() - 1]->getPeriod());
  }

  void startServices(std::shared_ptr<std::atomic<bool>> keepRunning);
  void stopServices();

private:
  void _checkPeriodCompatability(uint8_t servicePeriod);
  std::vector<std::unique_ptr<Service>> _services;
  uint8_t _period;
  StatTracker _stats;
};