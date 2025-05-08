/**
 * @file LedBlinker.hpp
 */

#pragma once

extern "C" {
#include "../ws2812b-test/ws2812b_led_control.h"
}

#include <memory>

class LedBlinker {
public:
  LedBlinker(int ledCount);
  ~LedBlinker();

  void setColors(std::shared_ptr<uint32_t[]> audio);

private:
  ws2811_led_t *matrix;
  int led_count;
  ws2811_t ledstring;
};
