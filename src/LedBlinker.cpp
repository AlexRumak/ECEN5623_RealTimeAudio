/**
 * @file LedBlinker.cpp
 */

#include "LedBlinker.hpp"
#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <unistd.h>
#include <exception>

#define ARRAY_SIZE(stuff)       (sizeof(stuff) / sizeof(stuff[0]))

// defaults for cmdline options
#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                10 // gpio 10 for SPI MOSI pin 19
#define DMA                     10
#define STRIP_TYPE            	WS2811_STRIP_GRB // LED strip we have is GRB

#define WIDTH                   12
#define HEIGHT                  1
#define LED_COUNT               (WIDTH * HEIGHT)

int width = WIDTH;
int height = HEIGHT;
int led_count = LED_COUNT;

ws2811_t ledstring =
{
    .freq = TARGET_FREQ,
    .dmanum = DMA,
    .channel =
    {
        [0] =
        {
            .gpionum = GPIO_PIN,
            .invert = 0,
            .count = LED_COUNT,
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

LedBlinker::LedBlinker()
{
  // Initialize the LED strip
  ws2811_return_t ret;
  matrix = (ws2811_led_t *)malloc(sizeof(ws2811_led_t) * LED_COUNT);
  
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

void LedBlinker::simulate()
{
  uint32_t aud_matrix[LED_COUNT] = {0};

  aud_matrix[0] = 0x3<<29;
  aud_matrix[1] = 0x1<<29;
  aud_matrix[2] = 0x0<<29;
  aud_matrix[3] = 0x0<<29;
  aud_matrix[4] = 0x7<<29;
  aud_matrix[5] = 0x6<<29;
  aud_matrix[6] = 0x0<<29;
  aud_matrix[7] = 0x1<<29;
  aud_matrix[8] = 0x0<<29;
  aud_matrix[9] = 0x0<<29;
  aud_matrix[10] = 0x0<<29;
  aud_matrix[11] = 0x0<<29;

  update_led_matrix_from_sound(matrix, &aud_matrix[0],LED_COUNT);
  update_led_color(&ledstring, matrix, LED_COUNT);
}
