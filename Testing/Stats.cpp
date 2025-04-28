/**
 * Class for testing Stats.hpp
 */
#include "Stats.hpp"

#include <iostream>

#define BUFFER_SIZE 1000

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "Usage: stat <numbers> Example: stat 1 2 3" << std::endl;
    exit(1);
  }

  StatTracker statTracker(argc - 1);
  for (int i = 0; i < argc - 1; i++)
  {
    statTracker.Add({static_cast<double>(std::atoi(argv[i+1]))});
  }
  
  std::cout << "Average Execution Duration: " << statTracker.GetAverageDurationMs() << std::endl;

  for (int i = 0; i < argc - 1; i++)
  {
    statTracker.Add({5});
  }

  std::cout << "Average Execution Duration: " << statTracker.GetAverageDurationMs() << std::endl;

  statTracker.Add({20});

  std::cout << "Average Execution Duration: " << statTracker.GetAverageDurationMs() << std::endl;

  return 0;
}
