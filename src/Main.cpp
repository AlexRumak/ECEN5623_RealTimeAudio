/**
 * @file Main.cpp
 * @brief Main file for the real-time audio sequencer. 
 */

#include "AudioBuffer.hpp"
#include "Microphone.hpp"
#include "Sequencer.hpp"
#include "Fib.hpp"
#include "RealTime.hpp"

#include <fftw3.h> // FFT library
#include <csignal>
#include <iostream>

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

double fftOutput[16] = {0};

class MicrophoneService : public Service
{
public:
  MicrophoneService(std::string id, uint16_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<logger::LoggerFactory> loggerFactory, std::shared_ptr<AudioBuffer> audioBuffer, std::shared_ptr<Microphone> microphone)
    : Service("microphone[" + id + "]", period, priority, affinity, loggerFactory)
  {
    _audioBuffer = audioBuffer;
    _microphone = microphone;
    _logger = loggerFactory->createLogger("MicrophoneService");
  }

  ~MicrophoneService()
  {
    delete _logger;
  }

protected:
  void _serviceFunction() override
  {
    int err = _microphone->GetFrames(_audioBuffer);

    if (err == -2)
    {
      _logger->log(logger::ERROR, "Buffer overrun");
    }
    else if (err < 0)
    {
      throw std::runtime_error("Failed to get frames from microphone");
    }

    bool acquired = _fftDone.try_acquire_for(std::chrono::milliseconds(_period));
    if (!acquired)
    {
      _logger->log(logger::ERROR, "MicrophoneService timed out waiting for FFT service to finish");
    }

    // Swap buffers
    _audioBuffer->swap();

    // Notify FFT service that data is ready
    _fftReady.release();
  }

private:
  std::shared_ptr<AudioBuffer> _audioBuffer;
  std::shared_ptr<Microphone> _microphone;
  logger::Logger *_logger;
};

class FFTService : public Service
{
public:
  FFTService(std::string id, uint16_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<logger::LoggerFactory> loggerFactory, std::shared_ptr<AudioBuffer> audioBuffer)
    : Service("fft[" + id + "]", period, priority, affinity, loggerFactory)
  {
    _audioBuffer = audioBuffer;
    _logger = loggerFactory->createLogger("FFTService");
    _in = (double *)fftw_malloc(sizeof(double) * audioBuffer->getBufferSize());
    _out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * (audioBuffer->getBufferSize() / 2 + 1));
  }

  ~FFTService()
  {
  }

protected:
  void _serviceFunction() override
  {
    bool acquired = _fftReady.try_acquire_for(std::chrono::milliseconds(_period));

    if (!acquired)
    {
      _logger->log(logger::ERROR, "FFTService timed out waiting for microphone data");
    }

    auto writeBuffer = _audioBuffer->getWriteBuffer();
    size_t bufferSize = _audioBuffer->getBufferSize();

    // FFT using FFTW
    for (size_t i = 0; i < bufferSize; i++)
    {
      _in[i] = writeBuffer[i];
    }
    // Create FFTW plan
    fftw_plan p = fftw_plan_dft_r2c_1d(bufferSize, _in, _out, FFTW_ESTIMATE);

    // Execute FFT
    fftw_execute(p);

    // Process FFT output
    double *magnitudes = (double*) malloc(sizeof(double) * (bufferSize/2 + 1));
    for (size_t i = 0; i < bufferSize/2 + 1; i++) {
        // Calculate magnitude from real and imaginary parts
        magnitudes[i] = sqrt(_out[i][0] * _out[i][0] + _out[i][1] * _out[i][1]);
    }

    _fftOutputMutex.lock(); //////////////////////////////////////////// critical section
    int bins_per_bucket = (bufferSize/2) / 16;

    for (size_t i = 0; i < 16; i++)
    {
      fftOutput[i] = 0;

      // Start and end indices for this bucket
      size_t start_idx = i * bins_per_bucket;
      size_t end_idx = (i + 1) * bins_per_bucket;
      
      // Make sure we don't exceed the actual output size
      if (end_idx > bufferSize/2) {
          end_idx = bufferSize/2;
      }
      
      // Average the magnitudes in this range
      for (size_t j = start_idx; j < end_idx; j++) {
        fftOutput[i] += magnitudes[j];
      }
      
      // Average the values
      if (end_idx > start_idx) {
        fftOutput[i] /= (end_idx - start_idx);
      }
    }
    _fftOutputMutex.unlock(); //////////////////////////////////////////// critical section

    fftw_destroy_plan(p);

    _fftDone.release();
  }

private:
  std::shared_ptr<AudioBuffer> _audioBuffer;
  logger::Logger *_logger;

  double *_in;
  fftw_complex *_out;
};

// TODO: Replace with LED service
class BeeperService : public Service
{
public:
  BeeperService(std::string id, uint16_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<logger::LoggerFactory> loggerFactory, std::shared_ptr<AudioBuffer> audioBuffer)
    : Service("beeper[" + id + "]", period, priority, affinity, loggerFactory)
  {
    _audioBuffer = audioBuffer;
    _internalBuffer = new char[sizeof(double) * 16];
    _logger = loggerFactory->createLogger("BeeperService");
  }

  ~BeeperService(){

  }

protected:
  void _serviceFunction() override
  {
    _fftOutputMutex.lock(); ////////////////////////////////// critical section
    for (int i = 0; i < 16; i++)
    {
      ((double *)_internalBuffer)[i] = fftOutput[i];
    }
    _fftOutputMutex.unlock(); //////////////////////////////// critical section

    // print output
    for (int i = 0; i < 16; i++)
    {
      _logger->log(logger::INFO, "FFT output[" + std::to_string(i) + "] = " + std::to_string(((double *)_internalBuffer)[i]));
    }
  }

private:
  std::shared_ptr<AudioBuffer> _audioBuffer;
  logger::Logger *_logger;
  char *_internalBuffer;
};

class LogsToFileService : public Service
{
public:
  LogsToFileService(std::string id, uint16_t period, uint8_t priority, uint8_t affinity, std::shared_ptr<logger::LoggerFactory> loggerFactory)
    : Service("logstofile[" + id + "]", period, priority, affinity, loggerFactory)
  {
    
  }

  ~LogsToFileService(){

  }

protected:
  void _serviceFunction() override
  {
  }
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

  std::shared_ptr<AudioBuffer> audioBuffer = std::make_shared<AudioBuffer>(1024);
  MicrophoneFactory microphoneFactory(loggerFactory);
  std::shared_ptr<Microphone> microphone = microphoneFactory.createMicrophone(audioBuffer, "hw:3,0");

  // starts service threads instantly, but will not run anything
  // TODO: Create pattern that creates services while adding them to the sequencer, as this prevents dangling threads.
  auto serviceOne = std::make_unique<MicrophoneService>("1", 20, maxPriority, SERVICES_CORE, loggerFactory, audioBuffer, microphone); 
  auto serviceTwo = std::make_unique<FFTService>("2", 20, maxPriority - 1, SERVICES_CORE, loggerFactory, audioBuffer);
  auto serviceThree = std::make_unique<BeeperService>("3", 100, maxPriority - 2, SERVICES_CORE, loggerFactory, audioBuffer);
  auto serviceFour = std::make_unique<LogsToFileService>("4", 200, minPriority, SERVICES_CORE, loggerFactory);

  sequencer->addService(std::move(serviceOne));
  sequencer->addService(std::move(serviceTwo));
  sequencer->addService(std::move(serviceThree));
  sequencer->addService(std::move(serviceFour));

  sequencer->startServices(keepRunning);
  sequencer->stopServices();

  delete sequencer;
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
