
#include "RealTime.hpp"

#include <iostream>
#include <csignal>
#include <fstream>
#include <sstream>
#include <exception>

#define GENERIC_ERROR 1
#define COULD_NOT_OPEN_BOOT_OPTIONS "Could not open boot options"
#define MUST_RUN_AS_ROOT "Must run as root"

struct Option
{
  std::string name;
  std::vector<int> cpus;
  bool found = false;
  bool equal = false;
};

struct Flag
{
  std::string name;
  bool found = false;
};

class RealTimeSettingsImpl : public RealTimeSettings
{
public:
  RealTimeSettingsImpl(SequencerType type, std::shared_ptr<logger::LoggerFactory> factory):
    RealTimeSettings(type, factory)
  {
    _logger = factory->createLogger("RealTimeSettingsImpl");
  }

  ~RealTimeSettingsImpl()
  {
    delete _logger;
  }

  void setRealtimeSettings() override
  {
    checkSudo();
    checkBootSettings();
  }

  Sequencer *createSequencer(uint16_t period, uint8_t priority, uint8_t affinity) override
  {
    if (_factory == nullptr)
    {
      throw std::runtime_error("Factory not initialized");
    }

    if (_sequencerType == SEQUENCER_SLEEP)
    {
      return _factory->createSleepSequencer(period, priority, affinity);
    }
    else if (_sequencerType == SEQUENCER_ISR)
    {
      return _factory->createISRSequencer(period, priority, affinity);
    }
    else
    {
      throw std::invalid_argument("Invalid sequencer type");
    }
  }

private:
  logger::Logger *_logger;

  void checkSudo()
  {
    if (getuid() != 0)
    {
      throw std::runtime_error("Must run as root");
    }
  }

  std::vector<int> getCpusFromOption(std::string optionVal)
  {
    std::vector<int> ret;

    // parse optionValue
    auto dash = optionVal.find("-");
    auto comma = optionVal.find(",");

    if (dash != std::string::npos)
    {
      std::string cpu1 = optionVal.substr(0, dash);
      std::string cpu2 = optionVal.substr(dash + 1);

      int firstCpu = std::stoi(cpu1);
      int secondCpu = std::stoi(cpu2);
      for (int i = firstCpu; i <= secondCpu; i++)
      {
        ret.emplace_back(i);
      }
    }
    else if (comma != std::string::npos)
    {
      std::cerr << "Not implemented" << std::endl;
    }
    else
    {
      // ???
      std::cerr << "Did not know how to parse: " << optionVal << std::endl;
    }

    return ret;
  }

  void parseSetting(std::string setting, std::vector<Flag> &flags, std::vector<Option> &options)
  {
    if (setting.find("=") != std::string::npos)
    {
      // split on "="
      auto pos = setting.find("=");
      auto optionName = setting.substr(0, pos);
      auto optionValue = setting.substr(pos + 1);

      for(size_t j = 0; j < options.size(); j++)
      {
        if (optionName == options[j].name)
        {
          options[j].found = true;

          std::vector<int> cpus = getCpusFromOption(optionValue);
        }
      }
    }
    else
    {
      for (size_t j = 0; j < flags.size(); j++)
      {
        if (setting == flags[j].name)
        {
          flags[j].found = true;
        }
      }
    }
  }

  void checkBootSettings()
  {
    std::string fileName = "/boot/firmware/cmdline.txt";
    std::ifstream file(fileName);

    if (!file.is_open())
    {
      throw std::runtime_error(COULD_NOT_OPEN_BOOT_OPTIONS);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    char delimeter = ' ';

    // TODO: Configuration in the future
    std::vector<Flag> flags = {{"rcu_nocb_poll"}, {"nosoftlockup"}};
    std::vector<Option> cpuOptions = {
        { "isolcpus", {2, 3}}, 
        { "rcu_nocbs", {2, 3}},
        { "nohz_full", {1, 3}},
        { "kthread_cpus", {0, 1}}
      };

    // isolcpus=2-3" "nohz_full=2-3" "rcu_nocbs=2-3" "kthread_cpus=0-1" "nosoftlockup" "rcu_nocb_poll"
    while(content.find(delimeter) != std::string::npos || content.find("\n") != std::string::npos)
    {
      auto post = content.find(delimeter) != std::string::npos ? content.find(delimeter) : content.find("\n");
      auto setting = content.substr(0, post);
      content = content.substr(post + 1);
      parseSetting(setting, flags, cpuOptions);
    }

    // check if flags were set
    std::stringstream flagsErr;
    for (size_t i = 0; i < flags.size(); i++)
    {
      if (!flags[i].found)
      {
        flagsErr << flags[i].name << " ";
      }
    }
    std::string error = flagsErr.str();

    // print error
    if (error.size() != 0)
    {
      std::stringstream cerr;
      cerr << "WARNING - flags not present: " << error;
      _logger->log(logger::INFO, cerr.str());
    }
  }
};

std::shared_ptr<RealTimeSettings> SettingsParser::parseSettings()
{
  if (_argc < 2)
  {
    std::cerr << "Usage: real_time <sleep|isr>" << std::endl;
    exit(1);
  }

  SequencerType sequencerType;
  std::string option = _argv[1];
  if (option == "sleep")
  {
    sequencerType = SEQUENCER_SLEEP; 
  }
  else if (option == "isr")
  {
    sequencerType = SEQUENCER_ISR;
  }
  else
  {
    std::cerr << "Invalid option: " << option << std::endl;
    exit(1);
  }

  auto factory = std::make_shared<logger::LoggerFactory>(logger::LoggerType::STDOUT, logger::LogLevel::INFO);
  std::shared_ptr<RealTimeSettings> settings = std::make_shared<RealTimeSettingsImpl>(sequencerType, factory);

  return settings;
}