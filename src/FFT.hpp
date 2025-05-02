/**
 * @file FFT.hpp 
 * A file containing FFT logic for splitting audio into frequency parts in N buckets.
 */
#pragma once

#include "AudioBuffer.hpp"
#include <fftw3.h>
#include <cmath>
#include <iostream>
#include <memory>

// Useful resources:
// 1. https://www.nti-audio.com/en/support/know-how/fast-fourier-transform-fft#:~:text=The%20%22Fast%20Fourier%20Transform%22%20(,frequency%20information%20about%20the%20signal.
// 2. 
class AudioFFT
{
public:
  AudioFFT(std::shared_ptr<AudioBuffer> audioBuffer)
  {
    _audioBuffer = audioBuffer;
  }
  ~AudioFFT()
  {

  }

  int performFFT(std::shared_ptr<uint32_t[]> out, size_t buckets)
  {
    _audioBuffer->getReadBuffer();
    
    // TODO: Write FFT logic

    for (size_t i = 0; i < buckets; i++)
    {
      out[i] = i * 65536 / 10; // TODO: Replace with actual FFT output
    }
    return 0;
  }

private:
  std::shared_ptr<AudioBuffer> _audioBuffer;
};
