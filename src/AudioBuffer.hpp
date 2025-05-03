/**
 * @file AudioBuffer.hpp
 * @brief Shared memory interface
 */

#pragma once

#include <memory>
#include <array>

class AudioBuffer
{
public:
  AudioBuffer(size_t initialCapacity, int channels);
  ~AudioBuffer();

  size_t getBufferSize();
  char *getWriteBuffer();
  char *getReadBuffer();
  void resizeBuffer(size_t newSize);
  void swap();

private:
  int activeBuffer;
  char *_buffer;
  char *_bufferTwo;
  size_t _bufferSize;
  int channels;
};