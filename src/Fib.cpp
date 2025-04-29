/**
 * Tuning the Fib.hpp
 */

#include "Fib.hpp"
#include "Stats.hpp"

#include <cstdint>
#include <iostream>
#include <chrono>
#include <thread>

#define ERROR 1

#define BUFFER_SIZE 100

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

int main(int argc, char* argv[])
{
  using namespace std::chrono_literals;

  if (argc != 3)
  {
    std::cout << "Usage: fib <iterations> <sequences>" << std::endl;
    exit(1);
  }

  int maxPriority = sched_get_priority_max(SCHED_FIFO);

  uint32_t iterations = static_cast<uint32_t>(std::stoul(argv[1]));
  uint32_t sequences = static_cast<uint32_t>(std::stoul(argv[2]));

  FibonacciLoadGenerator fib(sequences, iterations);

  setCurrentThreadAffinity(3);
  setCurrentThreadPriority(maxPriority);

  // warm cache
  for(int i = 0; i < 10; i++)
    fib.GenerateLoad();

  // Convert microseconds to float of milliseconds
  // 1000 microseconds in 1 ms 
  StatTracker stats(BUFFER_SIZE);

  for(int i = 0; i < BUFFER_SIZE; i++)
  {
    auto start = std::chrono::high_resolution_clock::now();
    fib.GenerateLoad();
    auto stop = std::chrono::high_resolution_clock::now();

    auto duration = duration_cast<std::chrono::microseconds>(stop - start);
    double milliseconds = static_cast<double>(duration.count()) / 1000.0;
    stats.Add({milliseconds});
  }

  std::cout << "Milliseconds elapsed: " << stats.GetAverageDurationMs() << std::endl;
  std::cout << "Milliseconds Max: " << stats.GetMaxVal() << std::endl;
  std::cout << "Milliseconds Min: " << stats.GetMinVal() << std::endl;
  std::cout << "95 percentile: " << stats.GetPercentile(0.95) << std::endl;
}