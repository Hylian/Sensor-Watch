#ifndef LM_TUNE_FACE_H_
#define LM_TUNE_FACE_H_

#include "movement.h"

void lm_tune_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr);
void lm_tune_face_activate(movement_settings_t *settings, void *context);
bool lm_tune_face_loop(movement_event_t event, movement_settings_t *settings, void *context);
void lm_tune_face_resign(movement_settings_t *settings, void *context);

#define lm_tune_face ((const watch_face_t){ \
    lm_tune_face_setup, \
    lm_tune_face_activate, \
    lm_tune_face_loop, \
    lm_tune_face_resign, \
    NULL, \
})

#endif // LM_TUNE_FACE_H_
