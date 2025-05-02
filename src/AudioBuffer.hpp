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
  AudioBuffer(size_t initialCapacity);
  ~AudioBuffer();

  size_t getBufferSize();
  char *getWriteBuffer();
  char *getReadBuffer();
  void commitWrite();

private:
  int activeBuffer;
  char *_buffer;
  char *_bufferTwo;
  size_t _bufferSize;
};