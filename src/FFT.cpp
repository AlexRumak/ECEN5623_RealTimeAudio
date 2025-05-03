/**
 * @file FFT.cpp
 * 
 */

#include "FFT.hpp"
#include "Logger.hpp"
#include <cstdint>
#include <cmath>
#include <fftw3.h>
#include <memory>
#include <algorithm>

AudioFFT::AudioFFT(std::shared_ptr<AudioBuffer> audioBuffer, std::shared_ptr<logger::LoggerFactory> loggerFactory) {
    _logger = loggerFactory->createLogger("AudioFFT");
    _audioBuffer = audioBuffer;
    _fftSize = _audioBuffer->getBufferSize() / sizeof(int16_t);
    _input  = (double*) fftw_malloc(sizeof(double) * _fftSize);
    _output = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (_fftSize/2 + 1));
    _plan = fftw_plan_dft_r2c_1d(_fftSize, _input, _output, FFTW_MEASURE); 
    // ^ using a plan (from fftw) here makes the fft much faster to run over and over again
}

AudioFFT::~AudioFFT() {
    fftw_destroy_plan(_plan);
    fftw_free(_input);
    fftw_free(_output);
    fftw_cleanup();
}

int AudioFFT::performFFT(std::shared_ptr<uint32_t[]> out, size_t buckets) {
    char* bufferData = _audioBuffer->getReadBuffer();
    int16_t* samples = reinterpret_cast<int16_t*>(bufferData); // might be able to consolidate these casts to one static cast to double?

    for (size_t i = 0; i < _fftSize; ++i) {
        _input[i] = static_cast<double>(samples[i]);
    }

    if (_logger->baseLevel() >= logger::TRACE)
    {
        // Print _input buffer for debugging
        std::stringstream output;
        for (size_t i = 0; i < _fftSize; ++i) {
            output << _input[i] << " ";
        }
        _logger->log(logger::TRACE, output.str());
    }

    fftw_execute(_plan);

    const double sampleRate = 48000.0;
    const double binWidth = sampleRate / (double)_fftSize;
    const double minFreq = 20.0;
    const double maxFreq = 15000.0;
    //ratio is so each bucket spans a constant factor of the frequncy ; f_min * r^num_buckets = f_max
    double ratio = std::pow(maxFreq/minFreq, 1.0 / (double)buckets);

    //then loop through values of r^i
    for (size_t b = 0; b < buckets; ++b)
    {
        double f_lo = minFreq * std::pow(ratio, (double)b);
        double f_hi = minFreq * std::pow(ratio, (double)(b + 1));

        size_t idx_lo = (size_t)std::ceil(f_lo / binWidth);
        size_t idx_hi = (size_t)std::floor(f_hi / binWidth);

        // clamp to valid range [1 ... N/2] (not sure how fast this call is, might change)
        idx_lo = std::clamp(idx_lo, (size_t)1, _fftSize/2);
        idx_hi = std::clamp(idx_hi, (size_t)1, _fftSize/2);

        if (idx_hi < idx_lo) { //i.e. no bins possible
            out[b] = 0;
            continue;
        }

        // NEW: find *max* magnitude in bin before dB conversion (rather than prior *sum*, as buckets now unequal width)
        double maxMag = 0.0;
        for (size_t i = idx_lo; i <= idx_hi; ++i)
        {
            double re = _output[i][0];
            double im = _output[i][1];
            double mag = std::sqrt(re*re + im*im);
            if (mag > maxMag) {
                maxMag = mag;
            }
        }

        // the db conversion may be wholly unnecessary I have not made up my mind but its simple enough to do so I'm doing it for now
        double ref = 32768.0; // (2^15) 
        double magRatio = maxMag / ref;
        double magdB;
        if (magRatio <= 0.0) {
            magdB = -100.0;  // treat zero mag. as -100 dB
        } else {
            magdB = 20.0 * log10(magRatio);
        }

        if (magdB < -96.0) { 
            magdB = -96.0; 
        }
        double dBNormalized = magdB + 96.0;
        if (dBNormalized < 0.0) {
            dBNormalized = 0.0;
        }

        out[b] = static_cast<uint32_t>(dBNormalized); //store scaled dB as uint32_t[] for HJ's portion
    }

    return 0;
}

