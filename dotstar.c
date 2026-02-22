/* File: dotstar.c
 * -------------
 * Author: Alexander Jeon
 * The dotstar implementation uses spi pins to drive rgb led strips (DotStar APA102). A data
 * sequence is sent to the strip to control the leds. 
 * 
 * base code taken from professor Julie Zelenski lecture code for dotstar LED strip:
 * Use spi to drive rgb led strip
 * Leds are DotStar APA102, strip contains 144 leds
 * To control strip, send data sequence consisting of
 * start frame + n pixels + end frame
 * https://cdn-shop.adafruit.com/product-files/2343/SK9822_SHIJI.pdf
*/


#include "timer.h"
#include "spi.h"
#include "strings.h"
#include "gpio.h"

// brightness range: 0 (off) to 31 (crazy bright)
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

// Initialize a set of spi pins used for LED strip, copy spi pin information to led strip struct
void spi2_init(led_strip *strip, gpio_id_t SPI2_MOSI, gpio_id_t SPI2_SCLK) {
    gpio_set_output(SPI2_MOSI);
    gpio_set_output(SPI2_SCLK);
    gpio_write(SPI2_MOSI, 0);
    gpio_write(SPI2_SCLK, 0);
    strip->mosi = SPI2_MOSI;
    strip->sclk = SPI2_SCLK;
}

// Write a byte of data to the led strip struct
void spi2_send_byte(led_strip *strip, uint8_t byte) {
    for (int i = 7; i >= 0; i--) {
        // set spi2 MOSI data to the current bit
        gpio_write(strip->mosi, (byte >> i) & 1);

        // pulse high and low to start reading bit
        gpio_write(strip->sclk, 1); // high
        timer_delay_us(1);
        gpio_write(strip->sclk, 0); // low
        timer_delay_us(1);
    }
}

void spi2_transfer(led_strip *strip, uint8_t *data, int len) {
    for (int i = 0; i < len; i++) { // send data buffer information
        spi2_send_byte(strip, data[i]);
    }
}

// if spi2 true, send data to second led strip
void show_strip(led_strip *strip, led_t *pixels, int n, bool spi2) {
    int n_start = 4;
    int n_end = (n/2)/8 + 1; // half-bit per pixel
    int n_total = n_start + n*sizeof(led_t) + n_end;
    uint8_t strip_data[n_total], unused[n_total];

    memset(&strip_data[0], 0, n_start);                     // start frame
    memcpy(&strip_data[4], pixels, n*sizeof(led_t));        // pixel data
    memset(&strip_data[4 + n*sizeof(led_t)], 0xff, n_end);  // end frame

    if (spi2) {
        spi2_transfer(strip, strip_data, sizeof(strip_data));
    }

    else {
        spi_transfer(strip_data, unused, sizeof(strip_data));
    }
}

// The display color function takes in a strip struct containing information about the 
// led strip clock an data pins, the number of leds to be turned on, the rgb values for the led,
// and a boolean condition of whether or not this is using the hardware assigned spi pins or 
// digitally assigned spi pins. 
void display_color(led_strip *strip, int nleds, uint8_t r, uint8_t g, uint8_t b, bool spi2) {
    led_t led_strip_array[nleds];

    for (int i = 0; i < nleds; i++) { // initialize all elements of the array to 0 to clear garbage data
        led_strip_array[i] = COLOR(0, 0, 0);
    }

    for (int i = 0; i < nleds; i++) {
        led_strip_array[i] = COLOR(r, g, b);
    }

    if (spi2) {
        show_strip(strip, led_strip_array, nleds, true);
    }
    else {
        show_strip(strip, led_strip_array, nleds, false);
    }
}