/**
 * @file Sequencer.cpp
 */

#include "Sequencer.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <syslog.h>
#include <csignal>

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
Sequencer::Sequencer(uint8_t period, uint8_t priority, uint8_t affinity):
  _period(period),
  _stats(StatTracker(1000))
{
  setCurrentThreadAffinity(affinity);
  setCurrentThreadPriority(priority);
}

void printServiceStatistics(std::unique_ptr<Service>& service, const std::string& name)
{
  auto releaseStats = service->releaseStats();
  auto executionStats = service->executionTimeStats();
  std::cout << "================================================================\n";
  std::cout << "Service " << name << " Execution Statistics\n";
  std::cout << "Execution Time Average: " << executionStats.GetAverageDurationMs() << "ms\n";
  std::cout << "Execution Time Max: " << executionStats.GetMaxVal() << "ms\n";
  std::cout << "Execution Time Min: " << executionStats.GetMinVal() << "ms\n";
  std::cout << "Release Time Average Error: " << releaseStats.GetAverageDurationMs() << "ms\n";
  std::cout << "Executions that met deadline: " << executionStats.GetNumberCompletedOnTime(service->getPeriod()) << "/" << executionStats.GetNumElements() << "\n";
  std::cout << "================================================================\n";
}

void printSequencerStatistics(StatTracker& stats)
{
  std::cout << "\n================================================================\n";
  std::cout << "Sequencer Execution Statistics\n";
  std::cout << "Execution Time Error Average: " << stats.GetAverageDurationMs() << "ms\n";
  std::cout << "Execution Time Error Max: " << stats.GetMaxVal() << "ms\n";
  std::cout << "Execution Time Error Min: " << stats.GetMinVal() << "ms\n";
  std::cout << "================================================================\n";
}

void printStatistics(StatTracker& sequencerStats, std::vector<std::unique_ptr<Service>>& services)
{
  printSequencerStatistics(sequencerStats);
  // Print execution statistics
  for(auto& service : services)
  {
    printServiceStatistics(service, service->serviceName());
  }
  std::cout << std::endl;
}

void Sequencer::startServices(std::shared_ptr<std::atomic<bool>> keepRunning) 
{
  using namespace std::chrono_literals;

  _initializeSequencer();

  const auto start = std::chrono::high_resolution_clock::now();

  long iterations = 0;
  while(keepRunning->load())
  {
    _waitForRelease();

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

void Sequencer::stopServices()
{
  for(auto& service : _services)
  {
    service->stop();
  }

  printStatistics(_stats, _services);
}

void Sequencer::_checkPeriodCompatability(uint8_t servicePeriod)
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
    if (timer_delete(timerid) == ERROR)
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
      if (sigaction(SIGALRM, &sa, NULL) == ERROR)
      {
          perror("sigaction");
          exit(-1);
      }

      // Create Timer
      sev.sigev_notify = SIGEV_SIGNAL;
      sev.sigev_signo = SIGALRM;
      sev.sigev_value.sival_ptr = this;

      if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) == ERROR)
      {
          perror("timer_create");
          exit(-1);
      }

      its.it_value.tv_sec = 0;
      its.it_value.tv_nsec = 1; // Start instantly
      its.it_interval.tv_sec = 0;
      its.it_interval.tv_nsec = intervalMs * 1000000; // Convert milliseconds to nanoseconds

      // Start the timer
      if (timer_settime(timerid, 0, &its, NULL) == ERROR)
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
Sequencer* SequencerFactory::createISRSequencer()
{
  return new ISRSequencer(_period, _priority, _affinity);
}

Sequencer* SequencerFactory::createSleepSequencer()
{
  return new SleepSequencer(_period, _priority, _affinity);
}