/**
 * @file Microphone.hpp
 * Service definition for microphone helpers
 * ALSA implementation
 */
#pragma once
#include "Logger.hpp"

namespace Mic {
  enum Error
  {
    MIC_OK = 0,
    MIC_ERROR = -1,
    MIC_BUFFER_OVERRUN = -2,
  };
};

class Microphone
{
public:
  Microphone() = default;
  ~Microphone() = default;

  virtual int GetFrames(std::shared_ptr<AudioBuffer> buffer) = 0;
};

class MicrophoneFactory
{
public:
  MicrophoneFactory(std::shared_ptr<logger::LoggerFactory> loggerFactory)
  {
    _loggerFactory = loggerFactory;
  }
  
  ~MicrophoneFactory()
  {
  }

  std::shared_ptr<Microphone> createMicrophone(std::shared_ptr<AudioBuffer> audioBuffer, std::string deviceName);

private:
  std::shared_ptr<logger::LoggerFactory> _loggerFactory;
};
