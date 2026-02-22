#ifndef _BUTTON_H
#define _BUTTON_H

#include "gpio.h"
#include "gpio_extra.h"
#include "interrupts.h"
#include "gpio_interrupt.h"
#include "Display.h"
#include "timer.h"

void button_init(gpio_id_t button);

bool button_is_pressed(gpio_id_t button);

int button_mode_select(DisplayConfig *display, gpio_id_t button, int num_modes);

#endif