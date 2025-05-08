/**
 * @file Stats.hpp
 * File to calculate statistics for different processes.
 * Uses a circular buffer of a given size to prevent dynamic allocations to the
 * buffer.
 */
#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <type_traits>
#include <unordered_map>

struct StatPoint {
  double timeMs;
};

inline bool compareAscending(StatPoint a, StatPoint b) {
  return a.timeMs > b.timeMs;
}

template <typename T>
  requires std::is_enum_v<T>
class StatusCounter {
private:
  std::unordered_map<T, int> _counts;

public:
  StatusCounter() = default;

  void Add(T t) { _counts[t]++; }

  int GetCount(T t) { return _counts[t]; }
};

class StatTracker {
public:
  StatTracker(unsigned int bufferSize) {
    _bufferSize = bufferSize;
    _arr = std::make_shared<StatPoint[]>(bufferSize);
    _totalElements = 0;
    _sum = 0;
  }

  // copies the struct. Might not be high perf.
  void Add(StatPoint stat) {
    auto tmp = _arr[index];

    _arr[index] = stat;
    index = (index + 1) % _bufferSize;

    _sum += stat.timeMs;

    if (_totalElements < _bufferSize) {
      _totalElements++;
    } else {
      _sum -= tmp.timeMs;
    }
  }

  double GetAverageDurationMs() {
    return _sum / static_cast<double>(_totalElements);
  }

  double GetMaxVal() {
    double maxVal = 0.0;
    for (int i = 0; i < _totalElements; i++) {
      if (_arr[i].timeMs > maxVal) {
        maxVal = _arr[i].timeMs;
      }
    }
    return maxVal;
  }

  double GetMinVal() {
    double minVal = std::numeric_limits<double>::max();
    for (int i = 0; i < _totalElements; i++) {
      if (_arr[i].timeMs < minVal) {
        minVal = _arr[i].timeMs;
      }
    }
    return minVal;
  }

  double GetPercentile(double percentile) {
    StatPoint *sorted = new StatPoint[_bufferSize];

    std::copy(_arr.get(), _arr.get() + _bufferSize, sorted);
    std::sort(sorted, sorted + _bufferSize, compareAscending);

    auto el = static_cast<int>(
        std::floor(static_cast<double>(_bufferSize) * percentile));
    double result = sorted[el].timeMs;

    delete[] sorted;
    return result;
  }

  int GetNumberCompletedOnTime(double deadline) {
    long count = 0;
    for (int i = 0; i < _totalElements; i++) {
      if (_arr[i].timeMs <= deadline) {
        count++;
      }
    }
    return count;
  }

  int GetNumElements() { return _totalElements; }

private:
  std::shared_ptr<StatPoint[]> _arr;
  double _averageDuration = 0.0;
  int index = 0;
  double _sum;
  int _bufferSize;
  int _totalElements;
};
