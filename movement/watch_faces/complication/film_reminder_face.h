#ifndef FILM_REMINDER_FACE_H_
#define FILM_REMINDER_FACE_H_

#include "movement.h"

void film_reminder_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr);
void film_reminder_face_activate(movement_settings_t *settings, void *context);
bool film_reminder_face_loop(movement_event_t event, movement_settings_t *settings, void *context);
void film_reminder_face_resign(movement_settings_t *settings, void *context);

#define film_reminder_face ((const watch_face_t){ \
    film_reminder_face_setup, \
    film_reminder_face_activate, \
    film_reminder_face_loop, \
    film_reminder_face_resign, \
    NULL, \
})

#endif // FILM_REMINDER_FACE_H_
