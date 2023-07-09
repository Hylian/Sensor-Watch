#include "lm_tune_face.h"

#include <stdlib.h>
#include <stdio.h>
#include "tcs3400.h"
#include "watch.h"

static void s_update_display() {
    char buf[11] = {0};
    sprintf(buf, "DF    %4u", tcs3400_ev_get_df());
    watch_display_string(buf, 0);
}

void lm_tune_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;
}

void lm_tune_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    s_update_display();
}

bool lm_tune_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    uint8_t current_page = *((uint8_t *)context);
    watch_date_time date_time = watch_rtc_get_date_time();

    switch (event.event_type) {
        case EVENT_LIGHT_BUTTON_UP:
            tcs3400_ev_set_df(tcs3400_ev_get_df() + 1);
            s_update_display();
            break;
        case EVENT_ALARM_BUTTON_UP:
            tcs3400_ev_set_df(tcs3400_ev_get_df() - 1);
            s_update_display();
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }

    return true;
}

void lm_tune_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}
