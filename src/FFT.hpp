/**
 * @file FFT.hpp 
 * A file containing FFT logic for splitting audio into frequency parts in N buckets.
 */
#pragma once

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
  AudioFFT(size_t bufferSize)
  {
    _out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * (bufferSize / 2 + 1));
    _bufferSize = bufferSize;
  }

  ~AudioFFT()
  {
    if (_out)
    {
      fftw_free(_out);
    }
  }

  int performFFT(std::unique_ptr<double[]> in, size_t n, std::shared_ptr<double[]> out, size_t buckets)
  {

    // Create FFTW plan
    fftw_plan p = fftw_plan_dft_r2c_1d(_bufferSize, in.get(), _out, FFTW_ESTIMATE);

    // Execute FFT
    fftw_execute(p);

    // Process FFT output
    double *magnitudes = (double*) malloc(sizeof(double) * (_bufferSize/2 + 1));
    for (size_t i = 0; i < _bufferSize/2 + 1; i++) {
        // Calculate magnitude from real and imaginary parts
        magnitudes[i] = sqrt(_out[i][0] * _out[i][0] + _out[i][1] * _out[i][1]);
    }

    // Bucketize the FFT output to audio buckets based on frequencies
    for (size_t i = 0; i < buckets; i++)
    {
      double bucketStart = i * (n / buckets);
      double bucketEnd = (i + 1) * (n / buckets);
      double sum = 0.0;
      int count = 0;

      for (size_t j = bucketStart; j < bucketEnd && j < (_bufferSize / 2 + 1); j++)
      {
        sum += magnitudes[j];
        count++;
      }

      out[i] = sum / count; // Average magnitude in the bucket
    }

    fftw_destroy_plan(p);

    return 0;
  }

private:
  double *_in;
  fftw_complex *_out;
  size_t _bufferSize;
};
