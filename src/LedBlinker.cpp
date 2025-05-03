/**
 * @file LedBlinker.cpp
 */

#include "LedBlinker.hpp"
#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <unistd.h>
#include <exception>
#include <memory>

#define ARRAY_SIZE(stuff)       (sizeof(stuff) / sizeof(stuff[0]))

// defaults for cmdline options
#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                10 // gpio 10 for SPI MOSI pin 19
#define DMA                     10
#define STRIP_TYPE            	WS2811_STRIP_GRB // LED strip we have is GRB

#define WIDTH                   8
#define HEIGHT                  1
#define LED_COUNT               (WIDTH * HEIGHT)

int width = WIDTH;
int height = HEIGHT;
int led_count = LED_COUNT;

LedBlinker::LedBlinker(int ledCount)
{
  // Initialize the LED strip
  led_count = ledCount;

  ws2811_return_t ret;
  matrix = (ws2811_led_t *)malloc(sizeof(ws2811_led_t) * ledCount);

  ledstring =
  {
      .freq = TARGET_FREQ,
      .dmanum = DMA,
      .channel =
      {
          [0] =
          {
              .gpionum = GPIO_PIN,
              .invert = 0,
              .count = ledCount,
              .strip_type = STRIP_TYPE,
              .brightness = 25,
          },
          [1] =
          {
              .gpionum = 0,
              .invert = 0,
              .count = 0,
              .brightness = 0,
          },
      },
  };
  
  if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS)
  {
    throw std::runtime_error("ws2811_init failed: " + std::string(ws2811_get_return_t_str(ret)));
  }
}

LedBlinker::~LedBlinker()
{
  // Free the allocated memory for the LED matrix
  free(matrix);
  ws2811_fini(&ledstring);
}

void LedBlinker::setColors(std::shared_ptr<uint32_t[]> audio)
{
  // Commit the changes to the LED strip
  update_led_matrix_from_sound(matrix, audio.get(), led_count);
  update_led_color(&ledstring, matrix, led_count);
}
