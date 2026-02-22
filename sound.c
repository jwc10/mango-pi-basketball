/* File: sound.c
 * -------------
 * Author: John Carlson
 * Implementation of module for playing sounds via notes, repurposed from Assignment 2.
 */
#include "sound.h"
#include "gpio.h"
#include "timer.h"

//Plays a note by sending an oscillating voltage from PB0 through buzzer
void play_note(int base_freq_100x, int note_time, int octave, int bpm, gpio_id_t buzzer) {
    //1 quarter note gets a beat
    //scale by 1000000 to act like double later
    int secs_per_sixteenth = 1000000 / (  (bpm * 4) / 60 );
    unsigned long milliseconds_elapsed = 0;
    int frequency_100x = base_freq_100x << octave;
    unsigned long start_ticks = timer_get_ticks();
    
        while (milliseconds_elapsed * 1000000 < (secs_per_sixteenth * note_time * 1000)) {
            milliseconds_elapsed = ((timer_get_ticks() - start_ticks) / 24) / 1000;
            gpio_write(buzzer, 1);
            timer_delay_us( (1000000 * 100 ) / (2 * frequency_100x) );
            gpio_write(buzzer, 0);
            timer_delay_us( (1000000 * 100 ) / (2 * frequency_100x) );
        }
timer_delay_ms(50); // to separate notes very slightly
}

//Simply a delay where no note is played
void play_rest(int rest_time, int bpm, gpio_id_t buzzer) {
    //1 quarter note gets a beat
    //scale by 1000000 to act like double later
    int secs_per_sixteenth = 1000000 / (  (bpm * 4) / 60 );
    unsigned long milliseconds_elapsed = 0;
    unsigned long start_ticks = timer_get_ticks();

        while (milliseconds_elapsed * 1000000 < (secs_per_sixteenth * rest_time * 1000)) {
            milliseconds_elapsed = ((timer_get_ticks() - start_ticks) / 24) / 1000;
            gpio_write(buzzer, 0);
    }
timer_delay_ms(50); // to separate notes slightly
}