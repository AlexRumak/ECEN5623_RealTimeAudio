/**
 * @file Main.cpp
 * @brief Main file for the real-time audio sequencer. 
 */

#include "AudioBuffer.hpp"
#include "Microphone.hpp"
#include "Sequencer.hpp"
#include "Fib.hpp"
#include "RealTime.hpp"
#include "FFT.hpp"
#include "LedBlinker.hpp"

#include <fftw3.h> // FFT library
#include <csignal>
#include <iostream>
#include <ncurses.h> // ncurses for virtual LED display
#include <cstdint>
#include <thread>
#include <memory>

#define SEQUENCER_CORE 2
#define SERVICES_CORE 3

#define TENMS 10
#define TWENTYMS 20
#define SEQ 115900

FibonacciLoadGenerator fib10(SEQ, TENMS);
FibonacciLoadGenerator fib20(SEQ, TWENTYMS);

std::counting_semaphore<1> _fftReady(0);
std::counting_semaphore<1> _fftDone(1);
std::mutex _fftOutputMutex;

volatile uint32_t fftOutput[16] = {0};

struct ServiceConfig
{
  size_t numberOfBuckets;
};

class MicrophoneService : public Service
{
public:
  MicrophoneService(std::string id, uint16_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<logger::LoggerFactory> loggerFactory, std::shared_ptr<AudioBuffer> audioBuffer, std::shared_ptr<Microphone> microphone, ServiceConfig serviceConfig)
    : Service("microphone[" + id + "]", period, priority, affinity, loggerFactory)
  {
    _audioBuffer = audioBuffer;
    _microphone = microphone;
    _serviceConfig = serviceConfig;
    _logger = loggerFactory->createLogger("MicrophoneService");
  }

  ~MicrophoneService()
  {
    delete _logger;
  }

protected:
  ServiceStatus _serviceFunction() override
  {
    _logger->log(logger::TRACE, "Entering MicrophoneService::_serviceFunction");

    int err = _microphone->GetFrames(_audioBuffer);

    if (err == -2)
    {
      _logger->log(logger::ERROR, "Buffer overrun");
    }
    else if (err < 0)
    {
      _logger->log(logger::ERROR, "Failed to get frames from microphone");
      return FAILURE;
    }
    else
    {
      _logger->log(logger::TRACE, "Got " + std::to_string(err) + " frames from microphone");
    }

    bool acquired = _fftDone.try_acquire_for(std::chrono::milliseconds(_period));
    if (!acquired)
    {
      _logger->log(logger::ERROR, "MicrophoneService timed out waiting for FFT service to finish");
      return FAILURE;
    }

    // Swap buffers
    _audioBuffer->swap();

    // Notify FFT service that data is ready
    _fftReady.release();

    _logger->log(logger::TRACE, "Exiting MicrophoneService::_serviceFunction");

    return SUCCESS;
  }

private:
  std::shared_ptr<AudioBuffer> _audioBuffer;
  std::shared_ptr<Microphone> _microphone;
  logger::Logger *_logger;
  ServiceConfig _serviceConfig;
};

class FFTService : public Service
{
public:
  FFTService(std::string id, uint16_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<logger::LoggerFactory> loggerFactory, std::shared_ptr<AudioBuffer> audioBuffer, ServiceConfig serviceConfig)
    : Service("fft[" + id + "]", period, priority, affinity, loggerFactory)
  {
    _audioBuffer = audioBuffer;
    _serviceConfig = serviceConfig;
    _logger = loggerFactory->createLogger("FFTService");
    _fft = new AudioFFT(audioBuffer, loggerFactory); // only one channel
  }

  ~FFTService()
  {
    delete _logger;
    delete _fft;
  }

protected:
  ServiceStatus _serviceFunction() override
  {
    _logger->log(logger::TRACE, "Entering FFTService::_serviceFunction");

    bool acquired = _fftReady.try_acquire_for(std::chrono::milliseconds(_period));

    if (!acquired)
    {
      _logger->log(logger::ERROR, "FFTService timed out waiting for microphone data");
      return FAILURE;
    }

    auto _out = std::make_shared<uint32_t[]>(_serviceConfig.numberOfBuckets);
    _fft->performFFT(_out, _serviceConfig.numberOfBuckets);

    _fftDone.release();

    // OUTPUT
    _fftOutputMutex.lock(); //////////////////////////////////////////// critical section
    
    // Copy FFT output to shared buffer
    for (int i = 0; i < _serviceConfig.numberOfBuckets; i++)
    {
      fftOutput[i] = _out[i];
    }

    _fftOutputMutex.unlock(); //////////////////////////////////////////// critical section


    _logger->log(logger::TRACE, "Exiting FFTService::_serviceFunction");
    return SUCCESS;
  }

private:
  std::shared_ptr<AudioBuffer> _audioBuffer;
  logger::Logger *_logger;
  ServiceConfig _serviceConfig;

  AudioFFT *_fft; 
};

class BeeperService : public Service
{
public:
  BeeperService(std::string id, uint16_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<logger::LoggerFactory> loggerFactory, std::shared_ptr<AudioBuffer> audioBuffer, ServiceConfig serviceConfig)
    : Service("beeper[" + id + "]", period, priority, affinity, loggerFactory)
  {
    _audioBuffer = audioBuffer;
    _serviceConfig = serviceConfig;
    _internalBuffer = new double[sizeof(double) * _serviceConfig.numberOfBuckets];
    _logger = loggerFactory->createLogger("BeeperService");
  }

  ~BeeperService()
  {
    delete[] _internalBuffer;
  }

protected:
  ServiceStatus _serviceFunction() override
  {
    _logger->log(logger::TRACE, "Entering BeeperService::_serviceFunction");
    _fftOutputMutex.lock(); ////////////////////////////////// critical section
    
    for (size_t i = 0; i < _serviceConfig.numberOfBuckets; i++)
    {
      ((double *)_internalBuffer)[i] = fftOutput[i];
    }

    _fftOutputMutex.unlock(); //////////////////////////////// critical section

    // print internal buffer
    if (_logger->baseLevel() >= logger::DEBUG)
    {
      std::stringstream output;
      for (size_t i = 0; i < _serviceConfig.numberOfBuckets; i++)
      {
        output << ((double *)_internalBuffer)[i] << " ";
      }
      _logger->log(logger::DEBUG, output.str());
    }

    clear(); // Clear the screen for the new frame
    mvprintw(0, 0, "Virtual LED Display:");

    // TODO: Fix bucketization of FFT output
    for (size_t i = 0; i < _serviceConfig.numberOfBuckets; i++)
    {
      // scale buckets with 50 = 0, 120 = 10
      // 0 - 70
      int baseLined = ((double *)_internalBuffer)[i] - 50.0;

      _logger->log(logger::DEBUG, "baseLined: " + std::to_string(baseLined));

      int intensity = static_cast<int>(baseLined / 70.0 * 10.0);

      mvprintw(i + 1, 0, "LED %2d: ", i);

      for (int j = 0; j < intensity; j++)
      {
        addch('0');
      }
    }

    refresh(); // Refresh the screen to show updates

    _logger->log(logger::TRACE, "Exiting BeeperService::_serviceFunction");
    return SUCCESS;
  }

private:
  std::shared_ptr<AudioBuffer> _audioBuffer;
  logger::Logger *_logger;
  double *_internalBuffer;
  ServiceConfig _serviceConfig;
};

class LEDBlinker : public Service
{
public:
  LEDBlinker(std::string id, uint16_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<logger::LoggerFactory> loggerFactory, ServiceConfig serviceConfig)
    : Service("ledblinker[" + id + "]", period, priority, affinity, loggerFactory)
  {
    _serviceConfig = serviceConfig;
    _internalBuffer = std::make_shared<uint32_t[]>(sizeof(uint32_t) * _serviceConfig.numberOfBuckets);
    _logger = loggerFactory->createLogger("LEDBlinker");
    _ledBlinker = std::make_unique<LedBlinker>(serviceConfig.numberOfBuckets);
  }

  ~LEDBlinker()
  {
  }

protected:
  ServiceStatus _serviceFunction() override
  {
    _logger->log(logger::TRACE, "Entering LEDBlinker::_serviceFunction");

    _fftOutputMutex.lock(); ////////////////////////////////// critical section
    
    for (size_t i = 0; i < _serviceConfig.numberOfBuckets; i++)
    {
      _internalBuffer[i] = fftOutput[i];
    }

    _fftOutputMutex.unlock(); //////////////////////////////// critical section

    _ledBlinker->setColors(_internalBuffer);

    _logger->log(logger::TRACE, "Exiting LEDBlinker::_serviceFunction");
  }

private:
  logger::Logger *_logger;
  std::shared_ptr<uint32_t[]> _internalBuffer;
  ServiceConfig _serviceConfig;
  std::unique_ptr<LedBlinker> _ledBlinker;
};

class LogsToFileService : public Service
{
public:
  LogsToFileService(std::string id, uint16_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<logger::LoggerFactory> loggerFactory, ServiceConfig serviceConfig)
    : Service("logstofile[" + id + "]", period, priority, affinity, loggerFactory)
  {
    _factory = loggerFactory;
  }

  ~LogsToFileService(){

  }

protected:
  ServiceStatus _serviceFunction() override
  {
    _factory->flush();
    _factory->clear();

    return SUCCESS;
  }

private:
  std::shared_ptr<logger::LoggerFactory> _factory;
};

std::shared_ptr<std::atomic<bool>> keepRunning; 

void interruptHandler(int sig)
{
  if (sig == SIGINT)
    keepRunning->store(false);
}

void runSequencer(std::shared_ptr<RealTimeSettings> realTimeSettings)
{
  std::shared_ptr<logger::LoggerFactory> loggerFactory = realTimeSettings->getLoggerFactory();

  int maxPriority = sched_get_priority_max(SCHED_FIFO);
  int minPriority = sched_get_priority_min(SCHED_FIFO);

  Sequencer* sequencer = realTimeSettings->createSequencer(10, maxPriority, SEQUENCER_CORE);

  ServiceConfig serviceConfig;
  serviceConfig.numberOfBuckets = 8;

  std::shared_ptr<AudioBuffer> audioBuffer = std::make_shared<AudioBuffer>(960 * 2, 2);
  MicrophoneFactory microphoneFactory(loggerFactory);
  std::shared_ptr<Microphone> microphone = microphoneFactory.createMicrophone(audioBuffer, "hw:3,0");

  // starts service threads instantly, but will not run anything
  // TODO: Create pattern that creates services while adding them to the sequencer, as this prevents dangling threads.
  auto serviceOne = std::make_unique<MicrophoneService>("1", 10, maxPriority, SERVICES_CORE, loggerFactory, audioBuffer, microphone, serviceConfig); 
  auto serviceTwo = std::make_unique<FFTService>("2", 10, maxPriority - 1, SERVICES_CORE, loggerFactory, audioBuffer, serviceConfig);

  sequencer->addService(std::move(serviceOne));
  sequencer->addService(std::move(serviceTwo));

  if (realTimeSettings->outputType() == CONSOLE)
  {
    initscr(); // Initialize ncurses
    noecho();  // Disable echoing of typed characters
    curs_set(0); // Hide the cursor
    clear();   // Clear the screen
    auto serviceThree = std::make_unique<BeeperService>("3", 100, maxPriority - 2, SERVICES_CORE, loggerFactory, audioBuffer, serviceConfig);
    sequencer->addService(std::move(serviceThree));
  }
  else if (realTimeSettings->outputType() == MUTED)
  {
    // do nothing
  }
  else if (realTimeSettings->outputType() == LED)
  {
    auto serviceThree = std::make_unique<LEDBlinker>("3", 100, maxPriority - 2, SERVICES_CORE, loggerFactory, serviceConfig);
    sequencer->addService(std::move(serviceThree));
  }
  else
  {
    throw std::runtime_error("not yet supported - to be implemented");
  }

  auto serviceFour = std::make_unique<LogsToFileService>("4", 200, minPriority, SERVICES_CORE, loggerFactory, serviceConfig);
  sequencer->addService(std::move(serviceFour));

  sequencer->startServices(keepRunning);
  sequencer->stopServices(true);

  delete sequencer;

  endwin(); // Clean up ncurses
}

int main(int argc, char **argv)
{
  SettingsParser *parser = new SettingsParser(argc, argv);

  auto realTimeSettings = parser->parseSettings();

  delete parser;

  realTimeSettings->setRealtimeSettings();

  // Interrupt handler
  keepRunning = std::make_shared<std::atomic<bool>>(true);
  signal(SIGINT, interruptHandler);

  runSequencer(realTimeSettings);
}
