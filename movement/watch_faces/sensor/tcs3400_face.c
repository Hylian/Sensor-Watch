#include "tcs3400_face.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "tcs3400.h"
#include "hal_ext_irq.h"
#include "watch_utility.h"

typedef enum {
  MODE_AV = 0,
  MODE_TV,
  MODE_EV,
  MODE_LUX,
  NUM_MODES
} metering_mode_t;

static const char *s_mode_strs[NUM_MODES] = {
  "A ",
  "T ",
  "EV",
  "L ",
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
  size_t shutter_speed_idx;
  size_t iso_idx;
  uint32_t last_ev;
  uint32_t last_lux;
  bool alarm_pressed; // Alarm button is held
  bool trigger_reading; // Sensor is running
  bool retry; // Follow-up reading needed for autogain
  bool has_reading; // At least one successful reading has been obtained
} s_state = {0};

// Returns:
//   Success: index of the the shutter speed to display
//   Light too low: -1
//   Light too high: -2
static int prv_pick_shutter_speed(int ev, size_t fstop_idx) {
  // Derived from EV = log2(N^2 / t), so t = N^2 / 2^EV.
  // The shutter speed table is indexed by 1/t (the denominator), starting at
  // index 0 = 1s.  f/1.4 has N^2 = 2, so at EV 1: t = 2/2 = 1s -> index 0.
  // Each stop of aperture adds 1 to fstop_idx and halves t, advancing one
  // index.  Each stop of EV adds 1, also halving t and advancing one index.
  // Together: index = ev - fstop_idx - 1.
  int result = ev - (int)fstop_idx - 1;
  if (result < 0) {
    return -1;
  }
  if (result >= NUM_SHUTTER_SPEEDS) {
    return -2;
  }
  return result;
}

// Returns:
//   Success: index of the aperture to display
//   Light too low (need aperture wider than f/1.4): -1
//   Light too high (need aperture narrower than f/16): -2
static int prv_pick_fstop(int ev, size_t shutter_speed_idx) {
  // Algebraic inverse of prv_pick_shutter_speed:
  // shutter_idx = ev - fstop_idx - 1  =>  fstop_idx = ev - shutter_idx - 1.
  int result = ev - (int)shutter_speed_idx - 1;
  if (result < 0) {
    return -1;
  }
  if (result >= NUM_FSTOPS) {
    return -2;
  }
  return result;
}

static void prv_draw_shutter_speed() {
  if (!s_state.has_reading) return;
  char buf[6] = {0};
  int shutter_speed_idx = prv_pick_shutter_speed(tcs3400_fixed_round_to_int(s_state.last_ev), s_state.fstop_idx);
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

static void prv_draw_tv() {
  watch_clear_colon();
  char speed_buf[5] = {0};
  sprintf(speed_buf, "%4u", s_shutter_speeds[s_state.shutter_speed_idx]);
  watch_display_string(speed_buf, 6);

  if (!s_state.has_reading) return;

  int fstop_idx = prv_pick_fstop(tcs3400_fixed_round_to_int(s_state.last_ev), s_state.shutter_speed_idx);
  if (fstop_idx == -1) {
    watch_display_string("LO", 4);
    watch_set_indicator(WATCH_INDICATOR_LAP);
  } else if (fstop_idx == -2) {
    watch_display_string("HI", 4);
    watch_set_indicator(WATCH_INDICATOR_LAP);
  } else {
    watch_display_string((char *)s_fstop_strs[fstop_idx], 4);
    watch_clear_indicator(WATCH_INDICATOR_LAP);
  }
}

static void prv_draw_ev() {
  if (!s_state.has_reading) return;
  watch_clear_indicator(WATCH_INDICATOR_LAP);
  char buf[6] = {0};
  uint8_t whole = tcs3400_fixed_get_whole(s_state.last_ev);
  uint8_t frac = tcs3400_fixed_get_frac_digit(s_state.last_ev);
  sprintf(buf, "  %2u%1u ", whole, frac);
  watch_display_string(buf, 6);
}

static void prv_draw_lux(uint32_t lux) {
  if (!s_state.has_reading) return;
  char buf[7] = " 0    ";
  watch_set_colon();
  if (lux == 0) {
      return;
  }

  int exponent = (int)log10(lux);
  double mantissa = lux / pow(10, exponent);

  uint8_t left_digits = (uint8_t)(mantissa * 10);
  uint8_t right_digits = (uint8_t)((mantissa * 100) - (left_digits * 10));

  watch_display_string((exponent < 0) ? "-" : " ", 3);

  sprintf(buf, "%02u%02u%02u", left_digits, right_digits, (uint8_t)abs(exponent));
  watch_display_string(buf, 4);
}

static void prv_draw_mode() {
  if (s_state.mode >= NUM_MODES) {
    s_state.mode = MODE_EV;
  }
  watch_display_string((char *)s_mode_strs[s_state.mode], 0);
  watch_clear_colon();
}

static void prv_draw_current_reading() {
  switch (s_state.mode) {
    case MODE_AV:
      prv_draw_shutter_speed();
      break;
    case MODE_TV:
      prv_draw_tv();
      break;
    case MODE_EV:
      prv_draw_ev();
      break;
    case MODE_LUX:
      prv_draw_lux(s_state.last_lux);
      break;
    default:
      break;
  }
}

static void prv_interrupt_handler() {
  uint32_t ev_fixed, lux;
  bool result = tcs3400_ev_measure(&ev_fixed, &lux, s_isos[s_state.iso_idx]);

  if (result) {
    if (s_state.retry) {
      tcs3400_write_wtime(TCS3400_WTIME_2_78MS);
      s_state.retry = false;
    }
    s_state.last_ev = ev_fixed;
    s_state.last_lux = lux;
    s_state.has_reading = true;
    prv_draw_current_reading();
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
  tcs3400_write_wtime(TCS3400_WTIME_2_78MS);
  tcs3400_clear_all_interrupts();
  watch_set_indicator(WATCH_INDICATOR_SIGNAL);
}

static void prv_fstop_incr() {
  s_state.fstop_idx = (s_state.fstop_idx + 1) % NUM_FSTOPS;
  char buf[11] = {0};
  sprintf(buf, "%s", s_fstop_strs[s_state.fstop_idx]);
  watch_display_string(buf, 4);
  prv_draw_current_reading();
}

static void prv_fstop_decr() {
  s_state.fstop_idx = (s_state.fstop_idx - 1) % NUM_FSTOPS;
  char buf[11] = {0};
  sprintf(buf, "%s", s_fstop_strs[s_state.fstop_idx]);
  watch_display_string(buf, 4);
  prv_draw_current_reading();
}

static void prv_iso_incr() {
  s_state.iso_idx = (s_state.iso_idx + 1) % NUM_ISOS;
  char buf[11] = {0};
  sprintf(buf, "%2u", s_isos[s_state.iso_idx]/100);
  watch_display_string(buf, 2);
  prv_draw_current_reading();
}

static void prv_iso_decr() {
  s_state.iso_idx = (s_state.iso_idx - 1) % NUM_ISOS;
  char buf[11] = {0};
  sprintf(buf, "%2u", s_isos[s_state.iso_idx]/100);
  watch_display_string(buf, 2);
  prv_draw_current_reading();
}

static void prv_shutter_speed_incr() {
  s_state.shutter_speed_idx = (s_state.shutter_speed_idx + 1) % NUM_SHUTTER_SPEEDS;
  prv_draw_current_reading();
}

static void prv_shutter_speed_decr() {
  s_state.shutter_speed_idx = (s_state.shutter_speed_idx - 1) % NUM_SHUTTER_SPEEDS;
  prv_draw_current_reading();
}

static void prv_df_incr() {
  tcs3400_ev_set_df(tcs3400_ev_get_df() + 1);
}

static void prv_df_decr() {
  tcs3400_ev_set_df(tcs3400_ev_get_df() - 1);
}

bool tcs3400_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
  (void) settings;
  (void) context;

  ext_irq_disable(A4);

  char buf[11] = {0};
  switch (event.event_type) {
    case EVENT_ACTIVATE:
      s_state.last_ev = 0;
      s_state.last_lux = 0;
      s_state.trigger_reading = false;
      s_state.alarm_pressed = false;
      s_state.retry = false;
      s_state.has_reading = false;

      if (s_state.mode == MODE_TV) {
        sprintf(buf, "  %2u", s_isos[s_state.iso_idx]/100);
        watch_display_string(buf, 0);
        prv_draw_tv();
      } else {
        sprintf(buf, "  %2u%s", s_isos[s_state.iso_idx]/100, s_fstop_strs[s_state.fstop_idx]);
        watch_display_string(buf, 0);
      }

      // Perform a single reading on activation
      tcs3400_start();
      break;
    case EVENT_LIGHT_BUTTON_UP:
      switch (s_state.mode) {
        case MODE_AV:
        case MODE_TV:
          prv_iso_incr();
          break;
        case MODE_LUX:
          prv_df_incr();
          break;
        default:
          break;
      }
      break;
    case EVENT_LIGHT_LONG_PRESS:
      prv_iso_decr();
      break;
    case EVENT_LIGHT_BUTTON_DOWN:
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
        prv_draw_current_reading();
      } else {
        switch (s_state.mode) {
          case MODE_AV:
            prv_fstop_incr();
            break;
          case MODE_TV:
            prv_shutter_speed_incr();
            break;
          case MODE_LUX:
            prv_df_decr();
            break;
          default:
            break;
        }
      }
      break;
    case EVENT_MODE_BUTTON_DOWN:
      break;
    case EVENT_MODE_LONG_PRESS:
      if (s_state.alarm_pressed) {
        movement_move_to_face(0);
      } else {
        switch (s_state.mode) {
          case MODE_AV:
            prv_fstop_decr();
            break;
          case MODE_TV:
            prv_shutter_speed_decr();
            break;
          default:
            break;
        }
      }
      break;
    case EVENT_TICK:
      break;
    case EVENT_LOW_ENERGY_UPDATE:
      movement_move_to_face(0);
      movement_request_wake();
      break;
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
