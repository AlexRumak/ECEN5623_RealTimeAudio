/**
 * @file Stats.hpp File to calculate statistics for different processes. Uses a circular buffer of a given size to prevent dynamic allocations to the buffer.
 */
#pragma once

#include <chrono>
#include <iostream>
#include <limits>
#include <algorithm>
#include <cmath>

struct StatPoint
{
  double timeMs;
};

inline bool compareAscending(StatPoint a, StatPoint b)
{
  return a.timeMs > b.timeMs;
}

class StatTracker
{
public:
  StatTracker(unsigned int bufferSize)
  {
    _bufferSize = bufferSize;
    _arr = new StatPoint[bufferSize];
    _totalElements = 0;
    _sum = 0;
  }

  // copies the struct. Might not be high perf.
  void Add(StatPoint stat)
  {
    auto tmp = _arr[index];

    _arr[index] = stat;
    index = (index + 1) % _bufferSize;

    _sum += stat.timeMs;
    
    if (_totalElements < _bufferSize)
    {
      _totalElements++;
    }
    else
    {
      _sum -= tmp.timeMs;
    }
  }

  double GetAverageDurationMs()
  {
    return _sum / _totalElements;
  }

  double GetMaxVal()
  {
    double maxVal = 0.0;
    for (int i = 0; i < _totalElements; i++)
    {
      if (_arr[i].timeMs > maxVal)
      {
        maxVal = _arr[i].timeMs;  
      }
    }
    return maxVal;
  }

  double GetMinVal()
  {
    double minVal = std::numeric_limits<double>::max();
    for (int i = 0; i < _totalElements; i++)
    {
      if (_arr[i].timeMs < minVal)
      {
        minVal = _arr[i].timeMs;  
      }
    }
    return minVal;
  }

  double GetPercentile(double percentile)
  {
    StatPoint *sorted = new StatPoint[_bufferSize];
    
    std::copy(_arr, _arr + _bufferSize, sorted);
    std::sort(sorted, sorted + _bufferSize, compareAscending);

    auto el = static_cast<int>(std::floor(static_cast<double>(_bufferSize) * percentile));
    return sorted[el].timeMs;
  }

private:
  StatPoint *_arr;
  double _averageDuration = 0.0;
  int index = 0;
  double _sum;
  int _bufferSize;
  int _totalElements;
};