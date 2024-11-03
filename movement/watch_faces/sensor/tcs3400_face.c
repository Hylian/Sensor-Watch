#include "tcs3400_face.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "tcs3400.h"
#include "hal_ext_irq.h"
#include "watch_utility.h"

typedef enum {
  MODE_EV = 0,
  MODE_AV,
  MODE_SV,
  NUM_MODES
} metering_mode_t;

static const char *s_mode_strs[NUM_MODES] = {
  "EV",
  "A ",
  "S ",
};

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
  metering_mode_t mode;
  size_t fstop_idx;
  size_t iso_idx;
  uint32_t last_ev;
  bool alarm_pressed; // Alarm button is held
  bool trigger_reading; // Sensor is running
  bool retry; // Follow-up reading needed for autogain
} s_state = {0};

// Returns:
//   Success: index of the the shutter speed to display
//   Light too low: -1
//   Light too high: -2
static int prv_pick_shutter_speed(int ev, size_t fstop_idx) {
  // At EV 1, f/1.4 == 1 sec
  int result = ev - fstop_idx;
  if (result < 0) {
    return -1;
  }
  if (result >= NUM_SHUTTER_SPEEDS) {
    return -2;
  }
  return result;
}

static void prv_draw_shutter_speed() {
  char buf[6] = {0};
  int shutter_speed_idx = prv_pick_shutter_speed(s_state.last_ev, s_state.fstop_idx);
  if (shutter_speed_idx == -1) {
    memcpy(buf, "  LO", 5);
    watch_set_indicator(WATCH_INDICATOR_LAP);
  } else if (shutter_speed_idx == -2) {
    memcpy(buf, "  HI", 5);
    watch_set_indicator(WATCH_INDICATOR_LAP);
  } else {
    sprintf(buf, "%4u", s_shutter_speeds[shutter_speed_idx]);
    watch_clear_indicator(WATCH_INDICATOR_LAP);
  }
  watch_display_string(buf, 6);
}

static void prv_draw_ev() {
  char buf[6] = {0};
  uint32_t whole = tcs3400_fixed_get_whole(s_state.last_ev);
  uint32_t frac = tcs3400_fixed_get_frac_digit(s_state.last_ev);
  sprintf(buf, "  %2u%1u ", whole, frac);
  watch_display_string(buf, 6);
}

static void prv_draw_mode() {
  if (s_state.mode >= NUM_MODES) {
    s_state.mode = MODE_EV;
  }
  watch_display_string(s_mode_strs[s_state.mode], 0);
}

static void prv_interrupt_handler() {
  uint32_t ev_fixed;
  bool result = tcs3400_ev_measure(&ev_fixed, s_isos[s_state.iso_idx]);

  if (result) {
    if (s_state.retry) {
      tcs3400_write_wtime(TCS3400_WTIME_103MS);
      s_state.retry = false;
    }
    s_state.last_ev = tcs3400_fixed_round_to_int(ev_fixed);
    prv_draw_shutter_speed();
  } else if (!s_state.retry) {
    tcs3400_write_wtime(TCS3400_WTIME_27_8MS);
    s_state.retry = true;
  }

  s_state.trigger_reading = (s_state.retry || s_state.alarm_pressed);

  if (!s_state.trigger_reading) {
    tcs3400_disable();
  }

  tcs3400_clear_all_interrupts();
}

void tcs3400_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
  (void) settings;
  (void) watch_face_index;
  (void) context_ptr;
  watch_enable_pull_up(A4);
}

void tcs3400_face_activate(movement_settings_t *settings, void *context) {
  (void) settings;
  (void) context;
  watch_enable_i2c();
  watch_register_interrupt_callback(A4, prv_interrupt_handler, INTERRUPT_TRIGGER_FALLING);
  tcs3400_ev_setup();
  tcs3400_write_wtime(TCS3400_WTIME_103MS);
  tcs3400_clear_all_interrupts();
}

bool tcs3400_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
  (void) settings;
  (void) context;

  ext_irq_disable(A4);

  char buf[11] = {0};
  switch (event.event_type) {
    case EVENT_ACTIVATE:
      s_state.last_ev = 0;
      s_state.trigger_reading = false;
      s_state.alarm_pressed = false;
      s_state.retry = false;

      sprintf(buf, "  %2u%s", s_isos[s_state.iso_idx]/100, s_fstop_strs[s_state.fstop_idx]);
      watch_display_string(buf, 0);

      // Perform a single reading on activation
      tcs3400_start();
      break;
    case EVENT_LIGHT_LONG_PRESS:
      if (!s_state.alarm_pressed) {
        s_state.fstop_idx = (s_state.fstop_idx - 1) % NUM_FSTOPS;
        sprintf(buf, "%s", s_fstop_strs[s_state.fstop_idx]);
        watch_display_string(buf, 4);
        prv_draw_shutter_speed();
      }
      break;
    case EVENT_LIGHT_BUTTON_DOWN:
      break;
    case EVENT_LIGHT_BUTTON_UP:
      if (!s_state.alarm_pressed) {
        s_state.fstop_idx = (s_state.fstop_idx + 1) % NUM_FSTOPS;
        sprintf(buf, "%s", s_fstop_strs[s_state.fstop_idx]);
        watch_display_string(buf, 4);
        prv_draw_shutter_speed();
      } else {
        s_state.iso_idx = (s_state.iso_idx + 1) % NUM_ISOS;
        sprintf(buf, "%2u", s_isos[s_state.iso_idx]/100);
        watch_display_string(buf, 2);
      }
      break;
    case EVENT_ALARM_BUTTON_DOWN:
      s_state.alarm_pressed = true;

      if (!s_state.trigger_reading) {
        tcs3400_start();
      }
      break;
    case EVENT_ALARM_BUTTON_UP:
    case EVENT_ALARM_LONG_UP:
      s_state.alarm_pressed = false;
      break;
    case EVENT_MODE_BUTTON_UP:
      if (s_state.alarm_pressed) {
        // Change metering mode
        s_state.mode = (s_state.mode + 1) % NUM_MODES;
        prv_draw_mode();
      } else {
        movement_move_to_next_face();
      }
      break;
    case EVENT_MODE_LONG_PRESS:
      if (MOVEMENT_SECONDARY_FACE_INDEX && movement_state.current_face_idx == 0) {
        movement_move_to_face(MOVEMENT_SECONDARY_FACE_INDEX);
      } else {
        movement_move_to_face(0);
      }
      break;
    case EVENT_TICK:
      break;
    case EVENT_LOW_ENERGY_UPDATE:
    case EVENT_TIMEOUT:
      movement_move_to_face(0);
      break;
    default:
      break;
  }

  ext_irq_enable(A4);

  return true;
}

void tcs3400_face_resign(movement_settings_t *settings, void *context) {
  (void) settings;
  (void) context;
  ext_irq_disable(A4);
  s_state.alarm_pressed = false;
  s_state.trigger_reading = false;
  tcs3400_stop();
  watch_disable_i2c();
  movement_request_tick_frequency(1);
}
