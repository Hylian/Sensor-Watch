#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "tcs3400_face.h"
#include "watch_utility.h"
#include "tcs3400.h"

#define NUM_FSTOPS (8)
static char *fstop_strs[NUM_FSTOPS] = {
    "14",
    " 2",
    "28",
    " 4",
    "56",
    " 8",
    "11",
    "16",
};
static size_t s_fstop_idx = 0;

#define NUM_SHUTTER_SPEEDS (14)
static size_t shutter_speeds[NUM_SHUTTER_SPEEDS] = {
    1,
    2,
    4,
    8,
    15,
    30,
    60,
    125,
    250,
    500,
    1000,
    2000,
    4000,
    8000,
};

#define NUM_ISOS (4)
static size_t isos[NUM_ISOS] = {
    100,
    200,
    400,
    800
};
static size_t s_iso_idx = 0;

static bool s_reading = false;
static bool s_alarm_pressed = false;
static bool s_result_available = false;

// At EV 1, f/1.4 == 1 sec
static size_t s_pick_shutter_speed(int ev, size_t fstop_idx) {
    int result = ev - fstop_idx;
    if (result < 0) {
        return 0;
    }
    if (result >= NUM_SHUTTER_SPEEDS) {
        return NUM_SHUTTER_SPEEDS - 1;
    }
    return result;
}

static void s_interrupt_handler() {
    s_result_available = true;
}

void tcs3400_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;
    (void) context_ptr;
    //watch_enable_pull_up(A4);
    //watch_register_interrupt_callback(A4, s_interrupt_handler, INTERRUPT_TRIGGER_FALLING);
}

void tcs3400_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    char buf[11] = {0};
    sprintf(buf, "%2u%2u%s", tcs3400_get_gain(), isos[s_iso_idx]/100, fstop_strs[s_fstop_idx]);
    s_alarm_pressed = false;
    watch_display_string(buf, 0);
    watch_enable_i2c();
    tcs3400_ev_setup();
}

bool tcs3400_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;

    char buf[11] = {0};
    switch (event.event_type) {
        case EVENT_TICK:
            float ev;
            if (s_reading && tcs3400_ev_measure(&ev, isos[s_iso_idx])) {
                int rounded_ev = roundf(ev);
                size_t shutter_speed_idx = s_pick_shutter_speed(rounded_ev, s_fstop_idx);
                sprintf(buf, "%4u", shutter_speeds[shutter_speed_idx]);
                watch_display_string(buf, 6);
                sprintf(buf, "%2u", tcs3400_get_gain());
                watch_display_string(buf, 0);
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            s_iso_idx = (s_iso_idx + 1) % NUM_ISOS;
            sprintf(buf, "%2u", isos[s_iso_idx]/100);
            watch_display_string(buf, 2);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_BUTTON_UP:
            s_fstop_idx = (s_fstop_idx + 1) % NUM_FSTOPS;
            sprintf(buf, "%s", fstop_strs[s_fstop_idx]);
            watch_display_string(buf, 4);
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            s_reading = true;
            s_alarm_pressed = true;
            tcs3400_start();
            movement_request_tick_frequency(8);
            break;
        case EVENT_ALARM_BUTTON_UP:
        case EVENT_ALARM_LONG_UP:
            s_reading = false;
            s_alarm_pressed = false;
            tcs3400_stop();
            movement_request_tick_frequency(1);
            break;
        case EVENT_TIMEOUT:
            break;
        default:
            movement_default_loop_handler(event, settings);
            break;
    }

    return true;
}

void tcs3400_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    s_reading = false;
    tcs3400_stop();
    watch_disable_i2c();
}
