#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "gpio.h"
#include <stdint.h>

#define TM1637_I2C_COMM1    0x40 // Command to set data
#define TM1637_I2C_COMM2    0xC0 // Command to set address
#define TM1637_I2C_COMM3    0x80 // Command to set display control

extern const uint8_t digitToSegment[];

typedef struct {
    gpio_id_t pinClk;       // Clock pin
    gpio_id_t pinDIO;       // Data pin
    unsigned int bitDelay;  // Bit delay in microseconds
} DisplayConfig;

void display_init(DisplayConfig *Display, gpio_id_t Clk, gpio_id_t DIO, unsigned int bitDelay);

void bitDelay( DisplayConfig *Display);

void start(DisplayConfig *Display);

void stop(DisplayConfig *Display);

bool write_byte(DisplayConfig *Display, uint8_t byte);

void set_brightness(DisplayConfig *Display, uint8_t brightness, bool on);

void set_segments(DisplayConfig *Display, const uint8_t segments[], uint8_t length, uint8_t pos, bool clock_mode);

void display_num(DisplayConfig *Display, int num);

void display_countdown(DisplayConfig *Display, int mins, int secs);

void start_countdown(DisplayConfig *Display, int mins, int secs);

#endif