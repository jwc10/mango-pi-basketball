/* File Display.c
 * -------------
 * Author: Alexander Jeon
 * The display file allows for the communication with tm1637 4 digit displays.
 * Operations like setting brightness values, displaying numbers, and implementing
 * a countdown are implemented in this file. The code is based on the TM1637
 * datasheet and adapted from an display implementation used for an Arduino library 
 * created by Avishay - avishorp@gmail.com.
 * 
*/

#include "uart.h"
#include "mymodule.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "printf.h"
#include "timer.h"
#include "interrupts.h"
#include "gpio_interrupt.h"

// the following code is adapted from https://github.com/avishorp/TM1637/blob/master/TM1637Display.cpp 
// ported from avishorp@gmail.com's rduino implementation

// data sheet for tm1637 chip that was referenced: https://www.makerguides.com/wp-content/uploads/2019/08/TM1637-Datasheet.pdf

#define TM1637_I2C_COMM1    0x40
#define TM1637_I2C_COMM2    0xC0
#define TM1637_I2C_COMM3    0x80

#define SEG_DP  0b10000000

const uint8_t digitToSegment[] = {
  0b00111111,    // 0
  0b00000110,    // 1
  0b01011011,    // 2
  0b01001111,    // 3
  0b01100110,    // 4
  0b01101101,    // 5
  0b01111101,    // 6
  0b00000111,    // 7
  0b01111111,    // 8
  0b01101111,    // 9
  0b01110111,    // A
  0b01111100,    // b
  0b00111001,    // C
  0b01011110,    // d
  0b01111001,    // E
  0b01110001,    // F
  0b01010100,    // N
  0b01011100,    // o
  0b00111111     // D
  };

typedef struct {
    gpio_id_t pinClk;       // Clock pin
    gpio_id_t pinDIO;       // Data pin
    unsigned int bitDelay;  // Bit delay in microseconds
    uint8_t brightness; // brightness
} DisplayConfig;

void set_brightness(DisplayConfig *Display, uint8_t brightness, bool on);

void display_init(DisplayConfig *Display, gpio_id_t Clk, gpio_id_t DIO, unsigned int bitDelay) {
    // Save pin numbers and bitDelay to Display struct
	Display->pinClk = Clk;
	Display->pinDIO = DIO;
	Display->bitDelay = bitDelay;
    // a common default for bitdelay would be 100 microseconds

	// Clock and data pins are initialized to output mode and set to high
    // this leaves it in an idle state
    gpio_set_output(Clk);
    gpio_set_output(DIO);
	gpio_write(Clk, 1);
	gpio_write(DIO, 1);

    set_brightness(Display, 7, true);
}

void bitDelay(DisplayConfig *Display) {
    timer_delay_us(Display->bitDelay);
}

// the start function signals to the tm1637 chip that data is about to be sent
void start(DisplayConfig *Display) {
    gpio_set_output(Display->pinDIO);
    gpio_write(Display->pinDIO, 1);
    gpio_write(Display->pinClk, 1);
    bitDelay(Display);

    gpio_write(Display->pinDIO, 0);
    bitDelay(Display);
    gpio_write(Display->pinClk, 0);
}

// the stop function resets clock and dio to idle and signals to tm1637 end of data communication
void stop(DisplayConfig *Display) {
    gpio_write(Display->pinClk, 0);
    gpio_set_output(Display->pinDIO);
    gpio_write(Display->pinDIO, 0);
    bitDelay(Display);

    gpio_write(Display->pinClk, 1);
    bitDelay(Display);
    gpio_write(Display->pinDIO, 1);
}

bool write_byte(DisplayConfig *Display, uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++) {
        gpio_write(Display->pinClk, 0); // pull low

        if (byte & 0x01) { // if the least significant byte is 1
            gpio_set_input(Display->pinDIO); // release clock high
        } 

        else {
            gpio_set_output(Display->pinDIO);
            gpio_write(Display->pinDIO, 0);
        }

        bitDelay(Display);
        gpio_write(Display->pinClk, 1); // pull high
        bitDelay(Display);
        byte >>= 1; // shift one bit right to look at next bit
    }

    // wait for the acknowledgment
    gpio_write(Display->pinClk, 0);
    gpio_set_input(Display->pinDIO); // switch to input mode
    bitDelay(Display);

    gpio_write(Display->pinClk, 1); // pull clock high - shows start of acknowledgment
    
    // device pulls DIO low for ACK
    bool state = gpio_read(Display->pinDIO);
    bool ack = !state;
    
    gpio_write(Display->pinClk, 0);

    return ack;
}

void set_brightness(DisplayConfig *Display, uint8_t brightness, bool on) {
    if (on) {
        Display->brightness = (brightness & 0x7) | 0x08;
    } 

    else {
        Display->brightness = (brightness & 0x7) | 0x00;
    }
}

void set_segments(DisplayConfig *Display, const uint8_t segments[], uint8_t length, uint8_t pos, bool clock_mode) {
    start(Display); // signal start of communication
    write_byte(Display, TM1637_I2C_COMM1);
    stop(Display);

    start(Display);
    write_byte(Display, TM1637_I2C_COMM2 + (pos & 0x03));

    // write data for each segment, each digit has one byte of information
    for (uint8_t k = 0; k < length; k++) {
        if (clock_mode) {
            write_byte(Display, (segments[k] | 0b10000000));
        }
        else {
            write_byte(Display, segments[k]);
        }
    }

	stop(Display);

	start(Display);
    // turn the display on with brightness setting
	write_byte(Display, TM1637_I2C_COMM3 + (Display->brightness & 0x0f));
	stop(Display);
}

void display_num(DisplayConfig *Display, int num) {
    uint8_t segments[4] = {0, 0, 0, 0};

    if (num >= 9999) { // maximum possible number to display
        segments[0] = digitToSegment[9];
        segments[1] = digitToSegment[9];
        segments[2] = digitToSegment[9];
        segments[3] = digitToSegment[9];
        set_segments(Display, segments, 4, 0, false);
        return;
    }

    else if (num < 0) {
        return;
    }
    
    else {
        if (num == 0) { // case for 0
            segments[3] = digitToSegment[0];
            set_segments(Display, segments, 4, 0, false);
            return;
        }

        int num_copy = num;
        int num_digits = 0;

        while (num_copy > 0) {
            num_copy /= 10;
            num_digits++;
        }

        for (int i = 3; i >= 4 - num_digits; i--) {
            int digit = num % 10;
            segments[i] = digitToSegment[digit];
            num /= 10;
        }

        set_segments(Display, segments, 4, 0, false);

    }
}

void convert_to_clock(uint8_t *clock_digits, int num_secs) {
    if (num_secs > 5999) num_secs = 5999;
    if (num_secs < 1) num_secs = 0;
    int total_mins = num_secs / 60;
    int leftover_secs = num_secs % 60;
    clock_digits[0] = digitToSegment[total_mins / 10];
    clock_digits[1] = digitToSegment[total_mins % 10];
    clock_digits[2] = digitToSegment[leftover_secs / 10];
    clock_digits[3] = digitToSegment[leftover_secs % 10];
}

// displays the countdown on the screen
void display_countdown(DisplayConfig *Display, int mins, int secs) {
    uint8_t segments[4];
    segments[0] = digitToSegment[(mins / 10)];
    segments[1] = digitToSegment[(mins % 10)];
    segments[2] = digitToSegment[(secs / 10)];
    segments[3] = digitToSegment[(secs % 10)];
    set_segments(Display, segments, 4, 0, true);
}

//Takes in a number of mins and seconds and initiates a clock countdown from that number
void start_countdown(DisplayConfig *Display, int mins, int secs) {
    int initial_duration = (mins * 60) + secs;
    
    if (initial_duration > 5999) initial_duration = 5999;
    if (initial_duration < 1) return;
    int start_ticks = timer_get_ticks();
    int duration = initial_duration;
    uint8_t clock_digits[4];
    while (duration > 0) {
        int time_elapsed = ((timer_get_ticks() - start_ticks) / 24) / (1000*1000);
        duration = initial_duration - time_elapsed;
        convert_to_clock(clock_digits, duration);
        set_segments(Display, clock_digits, 4, 0, true);
    }
    
}