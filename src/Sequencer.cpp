/**
 * @file Sequencer.cpp
 */

#include "Sequencer.hpp"
#include <csignal>
#include <chrono>
#include <thread>
#include <iostream>
#include <syslog.h>

#define FATAL_ERR 1

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
        exit(FATAL_ERR);
    }
}

void setCurrentThreadPriority(int priority)
{
    sched_param sch;
    sch.sched_priority = priority;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sch) != 0)
    {
        std::cerr << "Error setting thread priority" << std::endl;
        exit(FATAL_ERR);
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

  int counter = 0;
  while (_running)
  {
    auto acquired = _releaseService.try_acquire_for(std::chrono::milliseconds(_period * 2)); // TODO: wait for less time, so that we can check if service is still running. Add a helper method for this.

    if (!_running)
    {
      _logger->log(logger::INFO, "Service exited " + _serviceName);
      break;
    }
    
    if (!acquired)
    {
      counter++;
      _logger->log(logger::ERROR, "Service " + _serviceName + " did not release in its 2*period: " + std::to_string(_period * 2) + "ms");

      if (counter > 100)
      {
        _logger->log(logger::ERROR, "Service " + _serviceName + " has not released in 100 periods. Stopping service.");
        _running.store(false);
      }
      continue;
    }

    auto start = std::chrono::high_resolution_clock::now();

    _serviceFunction();

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
  if (!_serviceStarted.load())
  {
    _serviceStarted.store(true);
    _firstRelease.store(std::chrono::high_resolution_clock::now());
  }
  else
  {
    auto now = std::chrono::high_resolution_clock::now();
    auto time_released = std::chrono::duration_cast<std::chrono::microseconds>(now - _firstRelease.load()).count();
    auto expected = std::chrono::microseconds(_releaseNumber * _period * 1000).count();
    _releaseStats.Add({static_cast<double>(time_released - expected) / 1000.0});
  }
  _releaseNumber++;
  _releaseService.release();
}

//////////////////// SEQUENCER MAIN ////////////////////
Sequencer::Sequencer(uint16_t period, uint8_t priority, uint8_t affinity):
  _period(period),
  _stats(StatTracker(1000))
{
  setCurrentThreadAffinity(affinity);
  setCurrentThreadPriority(priority);
}

void printServiceStatistics(std::ofstream& file, std::unique_ptr<Service>& service, const std::string& name)
{
  auto releaseStats = service->releaseStats();
  auto executionStats = service->executionTimeStats();
  file << "================================================================\n";
  file << "Service " << name << " Execution Statistics\n";
  file << "Execution Time Average: " << executionStats.GetAverageDurationMs() << "ms\n";
  file << "Execution Time Max: " << executionStats.GetMaxVal() << "ms\n";
  file << "Execution Time Min: " << executionStats.GetMinVal() << "ms\n";
  file << "Release Time Average Error: " << releaseStats.GetAverageDurationMs() << "ms\n";
  file << "Executions that met deadline: " << executionStats.GetNumberCompletedOnTime(service->getPeriod()) << "/" << executionStats.GetNumElements() << "\n";
  file << "================================================================\n";
}

void printSequencerStatistics(std::ofstream& file, StatTracker& stats)
{
  file << "\n================================================================\n";
  file << "Sequencer Execution Statistics\n";
  file << "Execution Time Error Average: " << stats.GetAverageDurationMs() << "ms\n";
  file << "Execution Time Error Max: " << stats.GetMaxVal() << "ms\n";
  file << "Execution Time Error Min: " << stats.GetMinVal() << "ms\n";
  file << "================================================================\n";
}

void printStatistics(StatTracker& sequencerStats, std::vector<std::unique_ptr<Service>>& services)
{
  std::ofstream file("statistics.txt", std::ios::app);
  if (!file.is_open())
  {
    std::cerr << "Error opening statistics.txt for writing" << std::endl;
    return;
  }

  printSequencerStatistics(file, sequencerStats);
  // Print execution statistics
  for(auto& service : services)
  {
    printServiceStatistics(file, service, service->serviceName());
  }
  file << std::endl;

  file.close();
}

void Sequencer::startServices(std::shared_ptr<std::atomic<bool>> keepRunning) 
{
  using namespace std::chrono_literals;

  _initializeSequencer();

  bool start_set = false;
  std::chrono::high_resolution_clock::time_point start;

  long iterations = 0;
  while(keepRunning->load())
  {
    _waitForRelease();

    if (!start_set)
    {
      start = std::chrono::high_resolution_clock::now();
      start_set = true;
    }

    auto now = std::chrono::high_resolution_clock::now();
    long uElapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
    double msElapsed = static_cast<double>(uElapsed) / 1000.0;
    double expectedMs = static_cast<double>(iterations * _period);

    _stats.Add({msElapsed - expectedMs});
    
    for(auto& service : _services)
    {
      if (_period * iterations % service->getPeriod() == 0)
      {
        service->release();
      }
    }

    iterations++;
  }
}

void Sequencer::stopServices(bool statisticsToFile)
{
  for(auto& service : _services)
  {
    service->stop();
  }

  if (statisticsToFile)
  {
    printStatistics(_stats, _services);
  }
}

void Sequencer::_checkPeriodCompatability(uint16_t servicePeriod)
{
  if (servicePeriod % _period != 0)
  {
    throw std::invalid_argument("Service not compatible with sequencer. The service period must be divisible by the sequencer period.");
  }
}

//////////////////// SEQUENCER ISR BASED ////////////////////

class ISRSequencer : public Sequencer
{
public:
  ISRSequencer(uint8_t period, uint8_t priority, uint8_t affinity) : Sequencer(period, priority, affinity),
    _releaseSequencer(0)
  {

  }

  ~ISRSequencer()
  {
    deleteTimer();
  }

protected:
  void _waitForRelease() override
  {
    auto acquired = _releaseSequencer.try_acquire_for(std::chrono::milliseconds(_period * 2));

    if (!acquired)
    {
      std::cerr << "Fatal error - sequencer did not release in twice its period" << std::endl;
      exit(1);
    }
  }

  void _initializeSequencer() override
  {
    setPeriodAlarm(_period);
  }

private:
  timer_t timerid;
  std::counting_semaphore<1> _releaseSequencer;

  void deleteTimer()
  {
    if (timer_delete(timerid) == FATAL_ERR)
    {
        perror("timer_delete");
        exit(-1);
    }
  }

  void onAlarm()
  {
    _releaseSequencer.release();
  }

  void setPeriodAlarm(long intervalMs)
  {
      struct sigevent sev;
      struct itimerspec its;
      struct sigaction sa;

      sa.sa_flags = SA_SIGINFO;
      sa.sa_sigaction = handleAlarm;
      sigemptyset(&sa.sa_mask);
      if (sigaction(SIGALRM, &sa, NULL) == FATAL_ERR)
      {
          perror("sigaction");
          exit(-1);
      }

      // Create Timer
      sev.sigev_notify = SIGEV_SIGNAL;
      sev.sigev_signo = SIGALRM;
      sev.sigev_value.sival_ptr = this;

      if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) == FATAL_ERR)
      {
          perror("timer_create");
          exit(-1);
      }

      its.it_value.tv_sec = 0;
      its.it_value.tv_nsec = 1; // Start instantly
      its.it_interval.tv_sec = 0;
      its.it_interval.tv_nsec = intervalMs * 1000000; // Convert milliseconds to nanoseconds

      // Start the timer
      if (timer_settime(timerid, 0, &its, NULL) == FATAL_ERR)
      {
          perror("timer_settime");
          exit(-1);
      }
  }

  static void handleAlarm(int sig, siginfo_t *si, void *uc)
  {

      if (sig != SIGALRM) {
          write(STDOUT_FILENO, "Unexpected signal\n", 19);
          return;
      }

      auto *self = static_cast<ISRSequencer*>(si->si_value.sival_ptr);
      if (self) {
          self->onAlarm();
      }
      else {
          write(STDOUT_FILENO, "Unexpected signal\n", 19);
      }
  }
};

//////////////////// SEQUENCER SLEEP BASED ////////////////////
class SleepSequencer : public Sequencer
{
public:
  SleepSequencer(uint8_t period, uint8_t priority, uint8_t affinity) : Sequencer(period, priority, affinity)
  {

  }

  ~SleepSequencer()
  {

  }

protected:
  bool firstTime = true;
  void _waitForRelease() override
  {
    if (firstTime)
    {
      firstTime = false;
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(_period));
  }

  void _initializeSequencer() override
  {

  }

private:
};

/////////////// SEQUENCER FACTORY ///////////////////
Sequencer* SequencerFactory::createISRSequencer(uint16_t period, uint8_t priority, uint8_t affinity)
{
  return new ISRSequencer(period, priority, affinity);
}

Sequencer* SequencerFactory::createSleepSequencer(uint16_t period, uint8_t priority, uint8_t affinity)
{
  return new SleepSequencer(period, priority, affinity);
}
