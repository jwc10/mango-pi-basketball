/* File: sound.h
 * -------------
 * Author: John Carlson
 * Module for playing sounds via notes through a simple piezobuzzer.
 */
#ifndef _SOUND_H
#define _SOUND_H

#include "gpio.h"
#include "timer.h"
#include "interrupts.h"

//octave 0 note frequencies below
//frequencies multiplied by 100 to essentially act as a double later on
enum {
    C = 1635,
    C_sharp = 1732,
    D_flat = C_sharp,
    D = 1835,
    D_sharp = 1945,
    E_flat = D_sharp,
    E = 2060,
    F = 2183,
    F_sharp = 2312,
    G_flat = F_sharp,
    G = 2450,
    G_sharp = 2750,
    A_flat = G_sharp,
    A = 2750,
    A_sharp = 2914,
    B_flat = A_sharp,
    B = 3087
};
enum {
    sixteenth = 1,
    eighth = 2,
    quarter = 4,
    half = 8,
    whole = 16
};

void play_note(int base_freq_100x, int note_time, int octave, int bpm, gpio_id_t buzzer);

void play_rest(int rest_time, int bpm, gpio_id_t buzzer);

#endif
