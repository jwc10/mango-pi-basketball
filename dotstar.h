#ifndef _DOTSTAR_H
#define _DOTSTAR_H

#include "timer.h"
#include "spi.h"
#include "strings.h"

#define DEFAULT_BRIGHTNESS 10

typedef struct {
    uint32_t bright : 5;
    uint32_t header : 3; // fixed 0b111
    uint32_t blue   : 8;
    uint32_t green  : 8;
    uint32_t red    : 8;
} led_t;


#define COLOR(r, g, b) ((led_t){.header = 0b111, .bright= DEFAULT_BRIGHTNESS, .red = (r), .green = (g), .blue = (b)})

typedef struct {
    gpio_id_t mosi;
    gpio_id_t sclk;
} led_strip;

#define SPI2_MOSI GPIO_PC1
#define SPI2_SCLK GPIO_PD15

void spi2_init(led_strip *strip, gpio_id_t SPI2_MOSI, gpio_id_t SPI2_SCLK);

void spi2_send_byte(led_strip *strip, uint8_t byte);

void spi2_transfer(led_strip *strip, uint8_t *data, int len);

void show_strip(led_strip *strip, led_t *pixels, int n, bool spi2);

// function to display solid rgb color on all leds, if spi2 false then color displayed on first led strip connected to hardware spi1 pins (PD11 and PD12)
void display_color(led_strip *strip, int nleds, uint8_t r, uint8_t g, uint8_t b, bool spi2);

#endif