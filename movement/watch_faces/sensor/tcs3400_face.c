#include "tcs3400_face.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "tcs3400.h"
#include "watch_utility.h"

#define NUM_FSTOPS (8)
static const char *s_fstop_strs[NUM_FSTOPS] = {
    "14",
    " 2",
    "28",
    " 4",
    "56",
    " 8",
    "11",
    "16",
};

#define NUM_SHUTTER_SPEEDS (14)
static const size_t s_shutter_speeds[NUM_SHUTTER_SPEEDS] = {
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
static const size_t s_isos[NUM_ISOS] = {
    100,
    200,
    400,
    800
};

static struct {
  size_t fstop_idx;
  size_t iso_idx;
  uint32_t last_ev;
  bool request_reading;
  bool trigger_reading;
  bool result_available;
} s_state = {0};

// Returns:
//   Success: index of the the shutter speed to display
//   Light too low: -1
//   Light too high: -2
static int prv_pick_shutter_speed(int ev, size_t fstop_idx) {
    static bool s_indicator_set = false;
    // At EV 1, f/1.4 == 1 sec
    int result = ev - fstop_idx;
    if (result < 0) {
        return -1;
        if (!s_indicator_set) {
            watch_set_indicator(WATCH_INDICATOR_LAP);
            s_indicator_set = true;
        }
        return 0;
    }
    if (result >= NUM_SHUTTER_SPEEDS) {
        return -2;
        if (!s_indicator_set) {
            watch_set_indicator(WATCH_INDICATOR_LAP);
            s_indicator_set = true;
        }
        return NUM_SHUTTER_SPEEDS - 1;
    }
    if (s_indicator_set) {
        watch_clear_indicator(WATCH_INDICATOR_LAP);
        s_indicator_set = false;
    }
    return result;
}

static void prv_update_shutter_speed() {
    char buf[6] = {0};
    int shutter_speed_idx = prv_pick_shutter_speed(s_state.last_ev, s_state.fstop_idx);
    if (shutter_speed_idx == -1) {
        memcpy(buf, "  LO", 5);
    } else if (shutter_speed_idx == -2) {
        memcpy(buf, "  HI", 5);
    } else {
        sprintf(buf, "%4u", s_shutter_speeds[shutter_speed_idx]);
    }
    watch_display_string(buf, 6);
}

static void prv_interrupt_handler() {
    s_state.result_available = true;
}

void tcs3400_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;
    (void) context_ptr;
    watch_enable_pull_up(A4);
    watch_register_interrupt_callback(A4, prv_interrupt_handler, INTERRUPT_TRIGGER_FALLING);
}

void tcs3400_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    watch_enable_i2c();
    tcs3400_ev_setup();
}

bool tcs3400_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;

    char buf[11] = {0};
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            s_state.last_ev = 0;
            sprintf(buf, "  %2u%s", s_isos[s_state.iso_idx]/100, s_fstop_strs[s_state.fstop_idx]);
            watch_display_string(buf, 0);
            break;
        case EVENT_TICK:
            bool retry = false;
            if (s_state.result_available && (s_state.trigger_reading || s_state.request_reading)) {
                uint32_t ev_fixed;
                bool result = tcs3400_ev_measure(&ev_fixed, s_isos[s_state.iso_idx]);
                retry = !result;

                if (result) {
                    s_state.result_available = false;
                    s_state.last_ev = tcs3400_fixed_round_to_int(ev_fixed);
                    prv_update_shutter_speed();
                }
            }

            s_state.trigger_reading = (retry || s_state.request_reading);

            if (s_state.trigger_reading) {
                if (retry) {
                    movement_request_tick_frequency(20);
                } else {
                    movement_request_tick_frequency(4);
                }
            } else {
                tcs3400_disable();
                movement_request_tick_frequency(1);
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            s_state.iso_idx = (s_state.iso_idx + 1) % NUM_ISOS;
            sprintf(buf, "%2u", s_isos[s_state.iso_idx]/100);
            watch_display_string(buf, 2);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_BUTTON_UP:
            s_state.fstop_idx = (s_state.fstop_idx + 1) % NUM_FSTOPS;
            sprintf(buf, "%s", s_fstop_strs[s_state.fstop_idx]);
            watch_display_string(buf, 4);
            prv_update_shutter_speed();
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            s_state.request_reading = true;

            if (!s_state.trigger_reading) {
                tcs3400_start();
                movement_request_tick_frequency(20);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
        case EVENT_ALARM_LONG_UP:
            s_state.request_reading = false;
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
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
    s_state.request_reading = false;
    s_state.trigger_reading = false;
    tcs3400_stop();
    watch_disable_i2c();
    movement_request_tick_frequency(1);
}
