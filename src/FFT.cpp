/**
 * @file FFT.cpp
 * 
 */

#include "FFT.hpp"
#include "Logger.hpp"
#include <cstdint>
#include <cmath>
#include <fftw3.h>

AudioFFT::AudioFFT(std::shared_ptr<AudioBuffer> audioBuffer) {
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

    fftw_execute(_plan);

    const double sampleRate = 48000.0;
    const double binWidth = sampleRate / (double)_fftSize;

    size_t startIndex = (size_t)ceil(20.0 / binWidth); // first bin >= 20 Hz
    size_t endIndex   = (size_t)floor(15000.0 / binWidth); // last bin <= 15 kHz
    if (endIndex > _fftSize/2) {
        endIndex = _fftSize/2;  // cap # of bins at Nyquist lim.
    }
    if (startIndex < 1) {
        startIndex = 1;
    }
    if (endIndex < startIndex) {
        for (size_t b = 0; b < buckets; ++b) {
            out[b] = 0;
        }
        return 0;
    }

    size_t totalBins = endIndex - startIndex + 1;
    size_t binsPerBucket = totalBins / buckets;
    size_t remainder     = totalBins % buckets;

    size_t currentBin = startIndex;
    for (size_t b = 0; b < buckets; ++b) {
        // how many bins in this bucket? (distribute remainders to first buckets)
        size_t count = binsPerBucket + ((b < remainder) ? 1 : 0);
        if (count == 0) {
            out[b] = 0;
            continue;
        }
        // sum magnitudes for this bucket 
        double sumMag = 0.0;
        for (size_t j = 0; j < count; ++j) {
            double real = _output[currentBin + j][0];
            double imag = _output[currentBin + j][1];
            double magnitude = sqrt(real*real + imag*imag);
            sumMag += magnitude;
        }
        // move to next set of bins
        currentBin += count;
        double avgMag = sumMag / (double)count;

        // the db conversion may be wholly unnecessary I have not made up my mind but its simple enough to do so I'm doing it for now
        double ref = 32768.0; // (2^15) 
        double magRatio = avgMag / ref;
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

