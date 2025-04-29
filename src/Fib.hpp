/**
 * Simulation code running the fibonacci sequence for X seconds, to simulate full compute time.
 */
#pragma once

#include <cstdint>

class FibonacciLoadGenerator
{
public:
  FibonacciLoadGenerator(uint32_t sequences, uint32_t iterations)
  {
    _iterations = iterations;
    _sequences = sequences;
  }

  // 1 1 2 3 ...
  void GenerateLoad()
  {
    for(uint32_t i = 0; i < _iterations; i++)
    {
      _n1 = 1;
      _n2 = 1;
      uint32_t seq = 0;
      while(seq < _sequences)
      {
        _tmp = _n2;
        _n2 = _n1 + _n2;
        _n1 = _tmp;
        seq++;
      }
    }
  }

private:
  uint32_t _sequences;
  uint32_t _iterations;
  uint32_t _n1 = 0;
  uint32_t _n2 = 0;
  uint32_t _tmp = 0;
};