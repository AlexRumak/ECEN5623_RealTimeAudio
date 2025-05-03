/**
 * @file SharedMemory.cpp
 */

#include "AudioBuffer.hpp"

AudioBuffer::AudioBuffer(size_t initialCapacity)
{
  _bufferSize = initialCapacity;
  _buffer = new char[_bufferSize];
  _bufferTwo = new char[_bufferSize];
  activeBuffer = 0;
}

AudioBuffer::~AudioBuffer()
{
  delete [] _buffer;
  delete [] _bufferTwo;
}

// define:   size_t getBufferSize();
//  char *getWriteBuffer();
// char *getReadBuffer();

size_t AudioBuffer::getBufferSize()
{
  return _bufferSize;
}

/// FIX RACE CONDITIONS LOL
char *AudioBuffer::getWriteBuffer()
{
  if (activeBuffer == 0)
  {
    return _buffer;
  }
  else
  {
    return _bufferTwo;
  }
}

char *AudioBuffer::getReadBuffer()
{
  if (activeBuffer == 0)
  {
    return _bufferTwo;
  }
  else
  {
    return _buffer;
  }
}

void AudioBuffer::resizeBuffer(size_t newSize)
{
  if (newSize != _bufferSize)
  {
    delete [] _buffer;
    delete [] _bufferTwo;

    _bufferSize = newSize;
    
    _buffer = new char[_bufferSize];
    _bufferTwo = new char[_bufferSize];
  }
}

void AudioBuffer::swap()
{
  activeBuffer = (activeBuffer + 1) % 2;
  return;
}