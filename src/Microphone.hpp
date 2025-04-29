/**
 * @file Microphone.hpp
 * Service definition for microphone helpers
 * ALSA implementation
 */
#pragma once

#include <alsa/asoundlib.h>

class Microphone
{
public:
  Microphone();
  virtual ~Microphone() = default;

private: 
};