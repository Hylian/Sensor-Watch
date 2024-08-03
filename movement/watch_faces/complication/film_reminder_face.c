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

typedef struct {
    uint8_t first;
    uint8_t second;
} sCameraName;

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

const char char_0[] = {
    ' ',
    'A',
    'B',
    'C',
    'D',
    'E',
    'F',
    'G',
    'H',
    'I',
    'J',
    'K',
    'L',
    'M',
    'N',
    'O',
    'P',
    'Q',
    'R',
    'S',
    'T',
    'U',
    'V',
    'W',
    'X',
    'Y',
    'Z',
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
};

const char char_1[] = {
    ' ',
    'A',
    'B',
    'C',
    'D',
    'E',
    'F',
    'H',
    'I',
    'J',
    'L',
    'N',
    'O',
    'R',
    'T',
    'U',
    'X',
    '0',
    '1',
    '3',
    '7',
    '8',
};

const size_t num_isos = sizeof(isos)/(sizeof(isos[0]));

typedef struct {
  uint8_t film_type;
  uint8_t iso;
  uint8_t name[2];
} sCameraEntry;

static struct {
  uint8_t current_camera;
  sCameraEntry cameras[NUM_CAMERAS];
} s_state = {0};

static uint8_t s_edit_idx = 0;
static bool s_edit_mode = false;


void film_reminder_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) context_ptr;
    (void) watch_face_index;
}

void film_reminder_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    watch_clear_all_indicators();
    movement_request_tick_frequency(1);
    s_edit_mode = false;
    s_edit_idx = 0;
}

static void display()
{
    sCameraEntry *camera = &s_state.cameras[s_state.current_camera];
    char buf[14];
    sprintf(buf, "%c%c%2i%4i%s",
        char_0[camera->name[0]],
        char_1[camera->name[1]],
        s_state.current_camera,
        isos[camera->iso],
        film_types[camera->film_type]);
    watch_display_string(buf, 0);
}

static void s_blink(bool phase) {
    sCameraEntry *camera = &s_state.cameras[s_state.current_camera];
    char buf[14];
    sprintf(buf, "%c%c%2i%4i%s",
        char_0[camera->name[0]],
        char_1[camera->name[1]],
        s_state.current_camera,
        isos[camera->iso],
        film_types[camera->film_type]);

    if (phase) {
        buf[s_edit_idx] = ' ';
        buf[2] = ' ';
        buf[3] = ' ';
    }
    watch_display_string(buf, 0);
}

bool film_reminder_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;

    char c;

    if (s_edit_mode) {
        int phase = event.subsecond % 2;
        switch (event.event_type) {
            case EVENT_TICK:
                s_blink(phase);
                break;
            case EVENT_MODE_BUTTON_UP:
                c = s_state.cameras[s_state.current_camera].name[s_edit_idx];
                c = (c - 1) % (s_edit_idx ? sizeof(char_1) : sizeof(char_0));
                s_state.cameras[s_state.current_camera].name[s_edit_idx] = c;
                s_blink(phase);
                break;
            case EVENT_ALARM_BUTTON_UP:
                c = s_state.cameras[s_state.current_camera].name[s_edit_idx];
                c = (c + 1) % (s_edit_idx ? sizeof(char_1) : sizeof(char_0));
                s_state.cameras[s_state.current_camera].name[s_edit_idx] = c;
                s_blink(phase);
                break;
            case EVENT_LIGHT_BUTTON_UP:
                if (s_edit_idx == 0) {
                    s_edit_idx++;
                    s_blink(phase);
                } else {
                    s_edit_mode = false;
                    watch_clear_indicator(WATCH_INDICATOR_BELL);
                    display();
                    movement_request_tick_frequency(1);
                }
                break;
            case EVENT_LIGHT_LONG_PRESS:
                s_edit_mode = false;
                watch_clear_indicator(WATCH_INDICATOR_BELL);
                movement_request_tick_frequency(1);
                display();
                break;
            case EVENT_TIMEOUT:
                s_edit_mode = false;
                movement_move_to_face(0);
                break;
            case EVENT_LIGHT_BUTTON_DOWN:
                break;
            default:
                break;
        }
    } else {
        switch (event.event_type) {
            case EVENT_ACTIVATE:
                display();
            case EVENT_TICK:
                break;
            case EVENT_LIGHT_BUTTON_UP:
                s_state.current_camera = (s_state.current_camera + 1) % NUM_CAMERAS;
                display();
                break;
            case EVENT_LIGHT_LONG_PRESS:
                s_edit_idx = 0;
                s_edit_mode = true;
                watch_set_indicator(WATCH_INDICATOR_BELL);
                movement_request_tick_frequency(4);
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
            case EVENT_TIMEOUT:
                movement_move_to_face(0);
                break;
            case EVENT_LIGHT_BUTTON_DOWN:
                break;
            default:
                movement_default_loop_handler(event, settings);
                break;
        }
    }

    return true;
}

void film_reminder_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}
