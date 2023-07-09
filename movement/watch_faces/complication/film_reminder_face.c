#include "film_reminder_face.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "watch.h"
#include "watch_private_display.h"

#define NUM_CAMERAS  (5)

const char* film_types[] = {
  "  ",
  "BW",
  "C4",
  "E6"
};

const size_t num_film_types = sizeof(film_types)/(sizeof(film_types[0]));

const char* camera_types[] = {
  "M3",
  "MF",
  "BN",
  "ZI",
  "RF"
};

const size_t num_camera_types = sizeof(camera_types)/(sizeof(camera_types[0]));

const uint16_t isos[] = {
  0,
  50,
  100,
  200,
  400,
  800,
  1600,
  3200
};

const size_t num_isos = sizeof(isos)/(sizeof(isos[0]));

typedef struct {
  uint8_t film_type;
  uint8_t iso;
} sCameraEntry;

static struct {
  uint8_t current_camera;
  sCameraEntry cameras[NUM_CAMERAS];
} s_state = {0};

void film_reminder_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) context_ptr;
    (void) watch_face_index;
}

void film_reminder_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    watch_clear_all_indicators();
}

static void display()
{
    sCameraEntry *camera = &s_state.cameras[s_state.current_camera];
    char buf[14];
    sprintf(buf, "%s%2i%4i%s",
        camera_types[s_state.current_camera],
        s_state.current_camera,
        isos[camera->iso],
        film_types[camera->film_type]);
    watch_display_string(buf, 0);
}

bool film_reminder_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            display();
        case EVENT_TICK:
            break;
        case EVENT_LIGHT_BUTTON_UP:
            s_state.current_camera = (s_state.current_camera + 1) % num_camera_types;
            display();
            break;
        case EVENT_LIGHT_LONG_PRESS:
            break;
        case EVENT_ALARM_LONG_PRESS:
            uint8_t *film_type = &(s_state.cameras[s_state.current_camera].film_type);
            *film_type = (*film_type + 1) % num_film_types;
            display();
            break;
        case EVENT_ALARM_BUTTON_UP:
            uint8_t *iso = &(s_state.cameras[s_state.current_camera].iso);
            *iso = (*iso + 1) % num_isos;
            display();
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        default:
            movement_default_loop_handler(event, settings);
            break;
    }

    return true;
}

void film_reminder_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}
