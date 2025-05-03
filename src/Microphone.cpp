/**
 * @file Microphone.cpp
 * Main class implementation of Microphone
 */

#include "AudioBuffer.hpp"
#include "Microphone.hpp"
#include "Logger.hpp"

#include <memory>
#include <alsa/asoundlib.h>
#include <string>
#include <iostream>
#include <exception>

#define CHANNELS 1
#define FORMAT SND_PCM_FORMAT_S16_LE  // 16-bit signed little-endian format

int             nchannels = 1;
int             buffer_size = 960; // 480 frames per 10ms
unsigned int    sample_rate = 48000;
int             bits = 16;

class ALSAUSBMicrophone : public Microphone
{
public:
  ALSAUSBMicrophone(std::shared_ptr<logger::LoggerFactory> loggerFactory, std::shared_ptr<AudioBuffer> audioBuffer, std::string deviceName)
  {
    _logger = loggerFactory->createLogger("ALSAUSBMicrophone");

    int err;
    if ((err = snd_pcm_open(&_handle, deviceName.c_str(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK)) < 0) {
        printf("Playback open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    if (configure_alsa_audio(CHANNELS) != 0)
    {
      _logger->log(logger::ERROR, "Failed to configure ALSA audio");
      initialized = false;
    }
    else
    {
      initialized = true;
    }
  }

  ~ALSAUSBMicrophone()
  {
    delete _logger;

    if (_handle)
    {
      snd_pcm_close(_handle);
      _handle = nullptr;
    }

    if (_hwParams)
    {
      snd_pcm_hw_params_free(_hwParams);
      _hwParams = nullptr;
    }
  }

  int GetFrames(std::shared_ptr<AudioBuffer> buffer) override
  {
    snd_pcm_uframes_t frames;
    int dir;

    snd_pcm_hw_params_get_period_size(_hwParams, &frames, &dir);

    int err;
    auto dataBuffer = buffer->getWriteBuffer();

    err = snd_pcm_readi(_handle, dataBuffer, frames);
    if (err == -EPIPE) {
        _logger->log(logger::ERROR, "overrun occurred"); 
        snd_pcm_prepare(_handle);
        return -2;
    } else if (err < 0) {
        _logger->log(logger::ERROR, "read from audio interface failed: %s" + std::string(snd_strerror(err)));
        return -1;
    } else if (err != (int)frames) {
        _logger->log(logger::ERROR, "short read, read " + std::to_string(err) + " frames");
        return err;
    }

    return 0;
  }

private:

  int configure_alsa_audio(int channels)
  {
    int                 err;
    int                 tmp = 0;
    unsigned int        fragments = 2;
    snd_pcm_uframes_t frames = 480;

    /* allocate memory for hardware parameter structure */ 
    if ((err = snd_pcm_hw_params_malloc(&_hwParams)) < 0) {
        _logger->log(logger::ERROR, "cannot allocate parameter structure (" + std::string(snd_strerror(err)) + ")");
        return 1;
    }
    /* fill structure from current audio parameters */
    if ((err = snd_pcm_hw_params_any(_handle, _hwParams)) < 0) {
        _logger->log(logger::ERROR, "cannot initialize parameter structure (" + std::string(snd_strerror(err)) + ")");
        return 1;
    }

    /* set access type, sample rate, sample format, channels */
    if ((err = snd_pcm_hw_params_set_access(_handle, _hwParams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        _logger->log(logger::ERROR, "cannot set access type: " + std::string(snd_strerror(err)));
        return 1;
    }
    // bits = 16
    if ((err = snd_pcm_hw_params_set_format(_handle, _hwParams, SND_PCM_FORMAT_S16_LE)) < 0) {
        _logger->log(logger::ERROR, "cannot set sample format: " + std::string(snd_strerror(err)));
        return 1;
    }
    
    unsigned int sampleRate = sample_rate;
    if ((err = snd_pcm_hw_params_set_rate_near(_handle, _hwParams, &sampleRate, 0)) < 0) {
        _logger->log(logger::ERROR, "cannot set sample rate: " + std::string(snd_strerror(err)));
        return 1;
    }
    if (sampleRate != sample_rate) {
        _logger->log(logger::ERROR, "Could not set requested sample rate, asked for " + std::to_string(sample_rate) + " got " + std::to_string(sampleRate));
        sample_rate = tmp;
    }
    if ((err = snd_pcm_hw_params_set_channels(_handle, _hwParams, channels)) < 0) {
        _logger->log(logger::ERROR, "cannot set channel count: " + std::string(snd_strerror(err)));
        return 1;
    }
    
    if ((err = snd_pcm_hw_params_set_periods_near(_handle, _hwParams, &fragments, 0)) < 0) {
      _logger->log(logger::ERROR, "Error setting # fragments to " + std::to_string(fragments) + ": " + std::string(snd_strerror(err)));
      return 1;
    }

    unsigned int frame_size = channels * (bits / 8);
    frames = buffer_size / frame_size * fragments; // want this to be ~480 frames for 10ms

    if ((err = snd_pcm_hw_params_set_buffer_size_near(_handle, _hwParams, &frames)) < 0) {
        _logger->log(logger::ERROR, "Error setting buffer_size " + std::to_string(frames) + " frames: " + std::string(snd_strerror(err)));
      return 1;
    }

    if (buffer_size != static_cast<int>(frames * frame_size / fragments)) {
        _logger->log(logger::ERROR, "Could not set requested buffer size, asked for " + std::to_string(buffer_size) + " got " + std::to_string(frames * frame_size / fragments));
        buffer_size = frames * frame_size / fragments;
    }

    if ((err = snd_pcm_hw_params(_handle, _hwParams)) < 0) {
      _logger->log(logger::ERROR, "Error setting HW params: " + std::string(snd_strerror(err)));
      return 1;
    }
    return 0;
  }

  logger::Logger *_logger;
  snd_pcm_t *_handle;
  snd_pcm_hw_params_t *_hwParams;
};

std::shared_ptr<Microphone> MicrophoneFactory::createMicrophone(std::shared_ptr<AudioBuffer> audioBuffer, std::string deviceName)
{
  return std::make_shared<ALSAUSBMicrophone>(this->_loggerFactory, audioBuffer, deviceName);
}
