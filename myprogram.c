/* File: myprogram.c
 * -------------
 * Author: John Carlson
 * Main program for the basketball game. Allows normal basketball arcade mode and a mode where
 * the hoops switch which team they score for every 5 seconds. The mode is changed by clicking the button,
 * then selected by holding the button.
 */

#include "uart.h"
#include "mymodule.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "printf.h"
#include "timer.h"
#include "interrupts.h"
#include "gpio_interrupt.h"
#include "Display.h"
#include "sound.h"
#include "ringbuffer.h"
#include "dotstar.h"
#include "hstimer.h"
#include "button.h"

gpio_id_t sensor_1 = GPIO_PB0;
gpio_id_t sensor_2 = GPIO_PB1;
gpio_id_t buzzer_1 = GPIO_PD21;
gpio_id_t buzzer_2 = GPIO_PD22; //not used in current code, all sounds use buzzer_1

gpio_id_t clock_team_1 = GPIO_PG13;
gpio_id_t DIO_team_1 = GPIO_PG12;

gpio_id_t clock_team_2 = GPIO_PB6;
gpio_id_t DIO_team_2 = GPIO_PD17;

gpio_id_t clock_countdown = GPIO_PB12;
gpio_id_t DIO_countdown = GPIO_PB11;
DisplayConfig team1_scoreboard, team2_scoreboard, countdown_timer;

//led strip 1 pins are the default hardware SPI pins as of now (PD11 and PD12)
gpio_id_t strip2_mosi = GPIO_PC1;
gpio_id_t strip2_sclk = GPIO_PD15;
led_strip strip2;

gpio_id_t button = GPIO_PB4; //button for selecting mode
static int nleds = 10; //number of LEDS on DO NOT GO OVER 10!!!!!!

#define RED 0 
#define BLUE 1
int hoop_1_team = RED; 
int hoop_2_team = BLUE;
//the above are starting states

struct hoop {
    int team;
    gpio_id_t IR_sensor;
    gpio_id_t buzzer;
    rb_t *ring_buf; //not used in current version
    DisplayConfig *scoreboard;
}; //hoop struct contains all devices attached to that hoop

struct hoops_in_game {
    struct hoop *hoop1;
    struct hoop *hoop2;
}; //pointers to the hoops used in the game

//Plays the 1 point sound through the buzzer on the breadboard
void play_1point_sound(gpio_id_t buzzer) {
    play_note(B, 1, 5, 300, buzzer);
    play_note(E, 4, 6, 300, buzzer);
}

//Plays the 2 point sound through the buzzer on the breadboard
void play_2point_sound(gpio_id_t buzzer) {
    play_note(B, 1, 5, 300, buzzer);
    play_note(E, 1, 6, 300, buzzer);
    play_note(A, 4, 6, 300, buzzer);
}

//Sound played when a team wins
void play_win_sound(gpio_id_t buzzer) {
    play_note(D, 2, 4, 300, buzzer);
    play_note(D, 2, 4, 300, buzzer);
    play_note(D, 2, 4, 300, buzzer);
    play_note(G, 6, 4, 300, buzzer);
    play_rest(1, 300, buzzer);
    play_note(B, 2, 4, 300, buzzer);
    play_note(B, 2, 4, 300, buzzer);
    play_note(B, 2, 4, 300, buzzer);
    play_note(D, 6, 5, 300, buzzer);
    play_rest(1, 300, buzzer);
    play_note(G, 2, 5, 300, buzzer);
    play_note(G, 2, 5, 300, buzzer);
    play_note(G, 2, 5, 300, buzzer);
    play_note(B, 12, 5, 300, buzzer);
}

void play_game_start(gpio_id_t buzzer) {
    play_note(E, 16, 4, 300, buzzer);
    play_rest(8, 300, buzzer);
    play_note(E, 16, 4, 300, buzzer);
    play_rest(8, 300, buzzer);
    play_note(E, 16, 4, 300, buzzer);
    play_rest(8, 300, buzzer);
    play_note(E, 8, 5, 300, buzzer);
}

//the below array holds the scores for red team (index 0) and blue team (index 1)
int scores[] = {0, 0};
unsigned long starting_ticks; //at the very start of the program
unsigned long ticks_at_entry = 0;
unsigned long ms_elapsed;

//Handler function for when an IR sensor is triggered/beam crossed.
//A hoop struct pointer is passed in for aux_data.
static void handle_entry(void *aux_data) {
    struct hoop *cur_hoop = (struct hoop *)aux_data;
    gpio_interrupt_clear(cur_hoop->IR_sensor);

    //delay for start of program since it seemingly treats turning on as a positive edge
    if ( (((timer_get_ticks() - starting_ticks) / 24) / (1000)) < 200) {
        uart_putchar('h');
        return;
    }

    ticks_at_entry = timer_get_ticks();

    while (gpio_read(cur_hoop->IR_sensor) != 0) {}
    ms_elapsed = ((timer_get_ticks() - ticks_at_entry) / 24) / (1000);
    printf("ms elapsed: %ld ", ms_elapsed);

    if (ms_elapsed < 10) {
        return; //give no points, probably a jitter/misread
    }
    else if (ms_elapsed < 65) {
        scores[cur_hoop->team] += 2; //add 2
        play_2point_sound(buzzer_1);
        //rb_enqueue(cur_hoop->ring_buf, 1);
    }
    else {
        scores[cur_hoop->team]++; //add 1
        play_1point_sound(buzzer_1);
        //rb_enqueue(cur_hoop->ring_buf, 2);
    }
    display_num(cur_hoop->scoreboard, scores[cur_hoop->team]);
    //the above displays the score for the team that the current hoop is for at the time,
    //on that hoops scoreboard
}

//Triggers however often we set it when registering,
//used for switching the team of the hoops. A hoops_in_game struct pointer
//is passed in for aux_data.
static void handle_timer_interrupt(void *aux_data) {
    hstimer_interrupt_clear(HSTIMER0);
    struct hoops_in_game *cur_game_hoops = (struct hoops_in_game *)aux_data;
    cur_game_hoops->hoop1->team = ~(cur_game_hoops->hoop1->team & 1) + 2;
    cur_game_hoops->hoop2->team = ~(cur_game_hoops->hoop2->team & 1) + 2;
    //the above switches the team values between 0 and 1 (bit flips them)

    // printf("%d", cur_game_hoops->hoop1->team);
    // printf("%d", cur_game_hoops->hoop2->team);

    if (cur_game_hoops->hoop1->team) {
        //if hoop1 has a team value of 1 (blue team) then display blue for it
        display_color(NULL, nleds, 0x00, 0x00, 0xFF, false); // solid blue color on led strip 1
        display_color(&strip2, nleds, 0xFF, 0x00, 0x00, true); // solid red color on led strip 2
    }
    else {
        //if hoop1 has a team value of 0 (red team) then display red for it
        display_color(NULL, nleds, 0xFF, 0x00, 0x00, false); // solid red color on led strip 1
        display_color(&strip2, nleds, 0x00, 0x00, 0xFF, true); // solid blue color on led strip 2
    }

    display_num(cur_game_hoops->hoop1->scoreboard, scores[cur_game_hoops->hoop1->team]);
    display_num(cur_game_hoops->hoop2->scoreboard, scores[cur_game_hoops->hoop2->team]);
    //the above updates the score displays for each hoop
}

//for another gpio pin attached to the same IR sensor - not used in current version
// static void handle_exit(void *aux_data) {
//     ms_elapsed = ((timer_get_ticks() - start_ticks) / 24) / (1000);

//     struct hoop *cur_hoop = (struct hoop *)aux_data;
//     gpio_interrupt_clear(cur_hoop->IR_sensor);
//     scores[cur_hoop->team]++; //add 1

// }

void main(void) {
    starting_ticks = timer_get_ticks(); //exists to handle the false interrupt at program start
    gpio_init();
    timer_init();
    uart_init();
    printf("\nStarting main() in %s\n", __FILE__);
    interrupts_init();
    say_hello("CS107e");
    printf("hi");
    printf("looping");
    //should all be handled in IR module - actually BEN said not needed
    gpio_set_input(sensor_1);
    gpio_set_input(sensor_2);
    gpio_set_output(buzzer_1);
    gpio_set_output(buzzer_2);
    rb_t *rb1 = rb_new();
    rb_t *rb2 = rb_new();

    display_init(&team1_scoreboard, clock_team_1, DIO_team_1, 100);
    display_init(&team2_scoreboard, clock_team_2, DIO_team_2, 100);
    display_num(&team1_scoreboard, 0);
    display_num(&team2_scoreboard, 0);
    display_init(&countdown_timer, clock_countdown, DIO_countdown, 100);

    spi_init(SPI_MODE_0);
    spi2_init(&strip2, strip2_mosi, strip2_sclk);
    display_color(NULL, nleds, 0xFF, 0x00, 0x00, false); // solid red color on led strip 1
    display_color(&strip2, nleds, 0x00, 0x00, 0xFF, true); // solid blue color on led strip 2

    struct hoop first_hoop = {RED, sensor_1, buzzer_1, rb1, &team1_scoreboard}; 
    struct hoop second_hoop = {BLUE, sensor_2, buzzer_2, rb2, &team2_scoreboard};
    struct hoops_in_game game_hoops = {&first_hoop, &second_hoop};

    gpio_interrupt_init();
    gpio_interrupt_config(sensor_1, GPIO_INTERRUPT_POSITIVE_EDGE, true);
    gpio_interrupt_register_handler(sensor_1, handle_entry, &first_hoop);
    gpio_interrupt_enable(sensor_1);
    gpio_interrupt_config(sensor_2, GPIO_INTERRUPT_POSITIVE_EDGE, true);
    gpio_interrupt_register_handler(sensor_2, handle_entry, &second_hoop);
    gpio_interrupt_enable(sensor_2);
    hstimer_init(HSTIMER0, 5000000);
    interrupts_register_handler(INTERRUPT_SOURCE_HSTIMER0, handle_timer_interrupt, &game_hoops);
    interrupts_enable_source(INTERRUPT_SOURCE_HSTIMER0);

    timer_delay_ms(500);
    button_init(button);
    int mode = button_mode_select(&countdown_timer, button, 2);

    display_countdown(&countdown_timer, 1, 30);
    play_game_start(buzzer_1);

    if (mode == 2) {
        hstimer_enable(HSTIMER0); //this then enables the mode where the
        //teams switch between hoops (shown by score displays and LED switching)
        //mode 1 is default mode, hoop teams stay constant
    }
    interrupts_global_enable();
    start_countdown(&countdown_timer, 1, 30);
    printf("ttt");
    gpio_interrupt_disable(sensor_1);
    gpio_interrupt_disable(sensor_2);
    hstimer_disable(HSTIMER0);
    play_win_sound(buzzer_1);
    if (scores[0] > scores[1]) {
        printf("red wins");
        //the below flashes red on both LEDs 3 times
        for (int i = 0; i < 3; i++) {
            display_color(NULL, nleds, 0x00, 0x00, 0x00, false); // LED STRIP 1 OFF
            display_color(&strip2, nleds, 0x00, 0x00, 0x00, true); // LED STRIP 2 OFF
            timer_delay_ms(500);
            display_color(NULL, nleds, 0xFF, 0x00, 0x00, false); // solid red color on led strip 1
            display_color(&strip2, nleds, 0xFF, 0x00, 0x00, true); // solid red color on led strip 2
            timer_delay_ms(1000);
        }
    }
    else if (scores[1] > scores[0]){
        printf("Blue wins");
        //the below flashes blue on both LEDs 3 times
        for (int i = 0; i < 3; i++) {
            display_color(NULL, nleds, 0x00, 0x00, 0x00, false); // LED STRIP 1 OFF
            display_color(&strip2, nleds, 0x00, 0x00, 0x00, true); // LED STRIP 2 OFF
            timer_delay_ms(500);
            display_color(NULL, nleds, 0x00, 0x00, 0xFF, false); // solid red color on led strip 1
            display_color(&strip2, nleds, 0x00, 0x00, 0xFF, true); // solid red color on led strip 2
            timer_delay_ms(1000);
        }
    }
    else {
        printf("TIE");
        //the below flashes purple on both LEDs 3 times
        for (int i = 0; i < 3; i++) {
            display_color(NULL, nleds, 0x00, 0x00, 0x00, false); // LED STRIP 1 OFF
            display_color(&strip2, nleds, 0x00, 0x00, 0x00, true); // LED STRIP 2 OFF
            timer_delay_ms(500);
            display_color(NULL, nleds, 0xFF, 0x00, 0xFF, false); // solid purple color on led strip 1
            display_color(&strip2, nleds, 0xFF, 0x00, 0xFF, true); // solid purple color on led strip 2
            timer_delay_ms(1000);
        }
    }
}
