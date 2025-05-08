/**
 * @file FFT.hpp
 * A file containing FFT logic for splitting audio into frequency parts in N
 * buckets.
 */
#pragma once

#include "AudioBuffer.hpp"
#include "Logger.hpp"
#include <cmath>
#include <fftw3.h>
#include <iostream>
#include <memory>

// Useful resources:
// 1.
// https://www.nti-audio.com/en/support/know-how/fast-fourier-transform-fft#:~:text=The%20%22Fast%20Fourier%20Transform%22%20(,frequency%20information%20about%20the%20signal.
// 2. https://www.fftw.org/fftw3_doc/Real_002ddata-DFTs.html
class AudioFFT {
public:
  explicit AudioFFT(std::shared_ptr<AudioBuffer> audioBuffer,
                    std::shared_ptr<logger::LoggerFactory> loggerFactory);
  ~AudioFFT();

  int performFFT(std::shared_ptr<uint32_t[]> out, size_t buckets);

private:
  std::shared_ptr<AudioBuffer> _audioBuffer;
  fftw_plan _plan;
  double *_input;
  fftw_complex *_output;
  size_t _fftSize;
  logger::Logger *_logger;
};
