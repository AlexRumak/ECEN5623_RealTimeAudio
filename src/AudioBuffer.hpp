/**
 * @file AudioBuffer.hpp
 * @brief Shared memory interface
 */

#pragma once

#include <array>
#include <memory>

class AudioBuffer {
public:
  AudioBuffer(size_t initialCapacity, unsigned int channels);
  ~AudioBuffer();

  size_t getBufferSize();
  char *getWriteBuffer();
  char *getReadBuffer();
  void resizeBuffer(size_t newSize);
  unsigned int getNumberOfChannels();
  void setNumberOfChannels(unsigned int channels);
  void swap();

private:
  int activeBuffer;
  char *_buffer;
  char *_bufferTwo;
  size_t _bufferSize;
  unsigned int channels;
};