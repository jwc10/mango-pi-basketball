/* File: button.c
 * -------------
 * Author: Alexander Jeon
 * The button implementation takes in one gpio pin for a 4 prong button and then
 * cycles through a user input number of modes. The button cycles through modes with one press,
 * and a long press selects the currently displayed mode.
 */

#include "gpio.h"
#include "gpio_extra.h"
#include "interrupts.h"
#include "gpio_interrupt.h"
#include "Display.h"
#include "timer.h"

void button_init(gpio_id_t button) {
    gpio_set_input(button);
    gpio_set_pullup(button); // use internal pullup resistor
}

bool button_is_pressed(gpio_id_t button) {
    return gpio_read(button) == 0;
}

/* The buttom_mode_select function takes in a display, button, and number of modes and begins the 
cycling process, waiting for the user to press the button and select which game mode to play. A single press
will cycle to the next mode, and a long press will return the mode the user selected.
*/
int button_mode_select(DisplayConfig *display, gpio_id_t button, int num_modes) {
    int mode = 1;
    display_num(display, mode);

    while (1) {
        if (button_is_pressed(button)) {
            unsigned int press_begin = timer_get_ticks();

            while (button_is_pressed(button)) {
                unsigned int press_time = timer_get_ticks() - press_begin;
                if (press_time >= 36000000) {  // if button pressed for 1.5 seconds (24000000 / 1 sec)
                    return mode;
                }
            }
            mode = (mode % num_modes) + 1;
            display_num(display, mode);
            timer_delay_ms(250);
        }
    }
}