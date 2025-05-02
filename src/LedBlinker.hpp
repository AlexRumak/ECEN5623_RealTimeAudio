/**
 * @file LedBlinker.hpp 
 */

#pragma once

extern "C" {
  #include "../ws2812b-test/ws2812b_led_control.h"
}

class LedBlinker
{
public:
  LedBlinker();
  ~LedBlinker();

  void simulate();

private:
  ws2811_led_t* matrix;
  int led_count;
};
