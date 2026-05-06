#include "tcs3400.h"

#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "watch_utility.h"

#define TCS3400_ADDR  (0x39)

static uint8_t prv_read_register(tcs3400_reg_e reg) {
    watch_i2c_send(TCS3400_ADDR, &reg, 1);

    uint8_t data;
    watch_i2c_receive(TCS3400_ADDR, &data, 1);

    return data;
}

static void prv_write_register(tcs3400_reg_e reg, uint8_t data) {
    char buf[2] = {reg, data};
    watch_i2c_send(TCS3400_ADDR, (uint8_t *) &buf, 2);
}

tcs3400_reg_id_t tcs3400_read_id(void) {
    return (tcs3400_reg_id_t) prv_read_register(TCS3400_REG_ID);
}

uint16_t tcs3400_read_clear_ir_data(void) {
    return watch_i2c_read16(TCS3400_ADDR, TCS3400_REG_CDATAL);
}

uint16_t tcs3400_read_red_data(void) {
    return watch_i2c_read16(TCS3400_ADDR, TCS3400_REG_RDATAL);
}

uint16_t tcs3400_read_blue_data(void) {
    return watch_i2c_read16(TCS3400_ADDR, TCS3400_REG_BDATAL);
}

uint16_t tcs3400_read_green_data(void) {
    return watch_i2c_read16(TCS3400_ADDR, TCS3400_REG_GDATAL);
}

tcs3400_reg_enable_t tcs3400_read_enable(void) {
    return (tcs3400_reg_enable_t)(prv_read_register(TCS3400_REG_ENABLE));
}

void tcs3400_write_enable(tcs3400_reg_enable_t enable) {
    prv_write_register(TCS3400_REG_ENABLE, enable.raw);
}

tcs3400_reg_status_t tcs3400_read_status(void) {
    tcs3400_reg_status_t status = {0};
    status.raw = prv_read_register(TCS3400_REG_STATUS);
    return status;
}

void tcs3400_read_data(tcs3400_reg_status_t *status,
                       uint16_t *clear,
                       uint16_t *red,
                       uint16_t *green,
                       uint16_t *blue) {
    if (!status || !clear || !red || !green || !blue) {
        return;
    }

    uint8_t reg = TCS3400_REG_STATUS;
    uint8_t data[9] = {0};
    watch_i2c_send(TCS3400_ADDR, &reg, 1);
    watch_i2c_receive(TCS3400_ADDR, data, 9);

    *status = (tcs3400_reg_status_t)data[0];
    *clear = (uint16_t)(data[1] + (data[2] << 8));
    *red   = (uint16_t)(data[3] + (data[4] << 8));
    *green = (uint16_t)(data[5] + (data[6] << 8));
    *blue  = (uint16_t)(data[7] + (data[8] << 8));
}

void tcs3400_write_again(tcs3400_again_e gain) {
    tcs3400_reg_control_t control = {0};
    control.field.again = gain;
    prv_write_register(TCS3400_REG_CONTROL, control.raw);
}

void tcs3400_write_atime(tcs3400_atime_e atime) {
    tcs3400_reg_atime_t atime_reg = {0};
    atime_reg.field.atime = atime;
    prv_write_register(TCS3400_REG_ATIME, atime_reg.raw);
}

void tcs3400_write_apers(tcs3400_apers_e apers) {
    tcs3400_reg_pers_t pers_reg = {0};
    pers_reg.field.apers = apers;
    prv_write_register(TCS3400_REG_PERS, pers_reg.raw);
}

void tcs3400_write_wtime(tcs3400_wtime_e wtime) {
    tcs3400_reg_wtime_t wtime_reg = {0};
    wtime_reg.field.wtime = wtime;
    prv_write_register(TCS3400_REG_WTIME, wtime_reg.raw);
}

void tcs3400_clear_all_interrupts() {
    prv_write_register(TCS3400_REG_AICLEAR, 0);
}

void tcs3400_start(void) {
    tcs3400_reg_enable_t enable = {0};
    enable.field.pon = 1;
    enable.field.aen = 1;
    enable.field.aien = 1;
    enable.field.wen = 1;  // Enable wait timer so WTIME register takes effect
    tcs3400_write_enable(enable);
}

void tcs3400_disable(void) {
    tcs3400_reg_enable_t enable = {0};
    enable.field.pon = 1;
    enable.field.aen = 0;
    tcs3400_write_enable(enable);
}

void tcs3400_stop(void) {
    tcs3400_reg_enable_t enable = {0};
    enable.field.pon = 0;
    enable.field.aen = 0;
    tcs3400_write_enable(enable);
}

uint32_t tcs3400_atime_to_us(tcs3400_atime_e atime) {
    switch (atime) {
        case TCS3400_ATIME_2_78MS:
            return 2780;
        case TCS3400_ATIME_27_8MS:
            return 27800;
        case TCS3400_ATIME_103MS:
            return 103000;
        case TCS3400_ATIME_178MS:
            return 178000;
        case TCS3400_ATIME_712MS:
            return 712000;
        default:
            return 0;
    }
}

uint8_t tcs3400_again_to_gain(tcs3400_again_e again) {
    switch (again) {
        case TCS3400_AGAIN_1X:
            return 1;
        case TCS3400_AGAIN_4X:
            return 4;
        case TCS3400_AGAIN_16X:
            return 16;
        case TCS3400_AGAIN_64X:
            return 64;
        default:
            return 0;
    }
}

uint32_t tcs3400_get_saturation_count(tcs3400_atime_e atime) {
    uint32_t atime_ms = tcs3400_atime_to_us(atime)/1000;
    if (atime_ms > 154) {
        // Digital saturation applies if atime_ms > 154ms
        return 65535;
    } else {
        // Otherwise, analog saturation will occur first.
        return 1024 * (atime_ms/3);
    }
}

static tcs3400_again_e s_again = TCS3400_AGAIN_16X;
uint8_t tcs3400_get_gain() {
    return tcs3400_again_to_gain(s_again);
}

static void s_increase_gain() {
    if (s_again < 0x3) {
        s_again++;
    }
    tcs3400_write_again(s_again);
}

static void s_decrease_gain() {
    if (s_again > 0x0) {
        s_again--;
    }
    tcs3400_write_again(s_again);
}

const tcs3400_atime_e kAtime = TCS3400_ATIME_27_8MS;

void tcs3400_ev_setup() {
    tcs3400_atime_e atime = kAtime;
    tcs3400_write_again(s_again);
    tcs3400_write_atime(atime);
    tcs3400_write_apers(TCS3400_APERS_EVERY);
}

// Device Factor (DF) — calibration scalar in the lux formula:
//   lux = (r_coef*R' + g_coef*G' + b_coef*B') * s_df / (ATIME_us * AGAINx)
//
// The DN40 standard device factor for the TCS34725 / TCS3472 family is 310
// (when ATIME is in milliseconds and the coefficients are unit-normalised, i.e.
// g_coef = 1.0). Because the coefficients in tcs3400_ev_measure are scaled by
// 1000 (g_coef = 1000) and ATIME is in microseconds (1000× larger), the two
// ×1000 factors cancel algebraically and s_df still equals the DN40 DF value
// for TCS34725 — except that the TCS3400 has a different photodetector
// sensitivity, making its effective DF larger.
//
// Empirically (measured against a reference lux meter under broadband white
// light at 27.8 ms / 16× gain): s_df ≈ 1130 gives the correct lux value;
// 1000 is a convenient round-number default that is ~0.2 EV (~15%) low.
// Call tcs3400_ev_set_df() to override. Larger values increase lux output.
static uint16_t s_df = 1000;
uint16_t tcs3400_ev_get_df() {
    return s_df;
}
void tcs3400_ev_set_df(uint16_t df) {
    s_df = df;
}

// Fixed point log2 implementation
// Based off of https://github.com/dmoulding/log2fix by Dan Moulding
#define FRACTIONAL_BITS (14)
// 2.6711635357704604 => b101011 => 0x2b
#define EV_OFFSET_FIXED ((uint32_t)(2.6711635357704604 * (1 << FRACTIONAL_BITS)))

static uint32_t s_log2_fixed(uint32_t x) {
    // Use uint64_t to avoid overflow when x >= 2^18 (i.e. scaled_lux > ~262k)
    uint64_t xw = (uint64_t)x << FRACTIONAL_BITS;
    int32_t b = 1U << (FRACTIONAL_BITS - 1);
    int32_t result = 0;

    if (xw == 0) {
        return 0;
    }

    while (xw < 1U << FRACTIONAL_BITS) {
        xw <<= 1;
        result -= 1U << FRACTIONAL_BITS;
    }

    while (xw >= 2U << FRACTIONAL_BITS) {
        xw >>= 1;
        result += 1U << FRACTIONAL_BITS;
    }

    uint64_t z = xw;

    for (size_t i = 0; i < FRACTIONAL_BITS; i++) {
        z = z * z >> FRACTIONAL_BITS;
        if (z >= 2U << FRACTIONAL_BITS) {
            z >>= 1;
            result += b;
        }
        b >>= 1;
    }

    return result;
}

#define TCS3400_FRAC_MASK(x) (x & ((1U << FRACTIONAL_BITS) - 1))

uint32_t tcs3400_fixed_get_whole(uint32_t x) {
    return x >> FRACTIONAL_BITS;
}

uint32_t tcs3400_fixed_round_to_int(uint32_t x) {
    return (x >> FRACTIONAL_BITS) +
           ((x & ((1U << FRACTIONAL_BITS) - 1)) >= (1U << (FRACTIONAL_BITS - 1)));
}

uint32_t tcs3400_fixed_get_frac_digit(uint32_t x) {
    uint64_t frac = ((uint64_t) TCS3400_FRAC_MASK(x)) << (32 - FRACTIONAL_BITS);
    frac *= 10;
    uint8_t digit_1 = (frac >> 32) % 10;
    frac *= 10;
    uint8_t digit_2 = (frac >> 32) % 10;
    if (digit_2 >= 5) {
        digit_1++;
    }
    return digit_1;
}

bool tcs3400_ev_measure(uint32_t *ev_fixed, uint32_t *lux, size_t iso) {
    if (!ev_fixed) {
        return false;
    }

    tcs3400_reg_status_t status;
    uint16_t clear, red, green, blue;
    tcs3400_read_data(&status, &clear, &red, &green, &blue);

    if (!status.field.avalid) {
        return false;
    }

    // Is saturated?
    if (clear > tcs3400_get_saturation_count(kAtime)) {
        s_decrease_gain();
        return false;
    }

    // Is gain too low?
    if (clear < 100) {
        s_increase_gain();
        return false;
    }

    // Estimate IR component of reading
    int32_t ir = red/2;
    ir += green/2;
    ir += blue/2;
    ir -= clear/2;

    // Subtract IR component from each channel
    int32_t red_p = red - ir;
    int32_t green_p = green - ir;
    int32_t blue_p = blue - ir;

    // Apply device-specific correction factors from the ams DN40 application note
    // ("Lux and CCT Calculations using ams Color Sensors", Appendix I).
    //
    // The DN40 lux equation is:
    //   lux = (R_Coef·R' + G_Coef·G' + B_Coef·B') · DF / (ATIME_ms · AGAINx)
    //
    // Reference values for the TCS34725 / TCS3472 family (closest published match):
    //   R_Coef = 0.136,  G_Coef = 1.000,  B_Coef = -0.444,  DF = 310
    //
    // These coefficients are scaled here by 1000 so that integer arithmetic is used
    // throughout (G_Coef = 1000 here vs 1.0 in the note). The ×1000 scaling of
    // the coefficients is algebraically cancelled by the ATIME unit conversion
    // (µs → ms), so s_df still absorbs the DN40 device factor.
    //
    // The TCS3400 has a different photodetector sensitivity than the TCS34725, so
    // its effective DF is higher. Empirically, s_df ≈ 1130 reproduces the reference
    // lux under broadband white light; the default s_df = 1000 is a round-number
    // approximation that gives ~0.2 EV error. Adjust s_df with tcs3400_ev_set_df()
    // after measuring against a calibrated lux meter.
    const int32_t r_coef = 136;   // DN40 R_Coef × 1000 = 0.136 × 1000
    const int32_t g_coef = 1000;  // DN40 G_Coef × 1000 = 1.000 × 1000 (reference)
    const int32_t b_coef = -444;  // DN40 B_Coef × 1000 = -0.444 × 1000
    int64_t red_c = red_p * r_coef;
    int64_t green_c = green_p * g_coef;
    int64_t blue_c = blue_p * b_coef;

    // Per ams DN40: lux = EV_Lux / CPL, where CPL = (ATIME_ms * AGAINx) / (GA * DF).
    // Equivalently: lux = EV_Lux * GA * DF / (ATIME_ms * AGAINx)
    //
    // The original formula computed CPL via integer division, which:
    //   - truncated CPL to 1 for 16x gain  (should be ~1.43)
    //   - truncated CPL to 0 for 1x/4x gain → division by zero
    //
    // Fix: keep ATIME in microseconds and multiply before dividing so no
    // precision is lost to integer truncation, and the denominator is always
    // nonzero (atime_us >= 2780, again_x >= 1 for any valid gain setting).
    //
    //   lux = EV_Lux * s_df / (ATIME_us * AGAINx)
    //
    const uint32_t atime_us = tcs3400_atime_to_us(kAtime);
    const uint8_t again_x = tcs3400_again_to_gain(s_again);

    int64_t raw_lux = ((red_c + green_c + blue_c) * s_df) / ((int64_t)atime_us * again_x);
    if (raw_lux < 0) {
      raw_lux = 0;
    }

    *lux = (uint32_t) raw_lux;

    uint32_t scaled_lux = (raw_lux * iso / 250);
    if (scaled_lux < 2) {
      *ev_fixed = 0;
      return true;
    }

    //*ev_fixed = s_log2_fixed((uint32_t)(raw_lux * iso / 250)) + EV_OFFSET_FIXED;
    *ev_fixed = s_log2_fixed((uint32_t)(raw_lux * iso / 250));

    return true;
}

static bool s_test_got_interrupt = false;
static void prv_test_interrupt_handler() {
  s_test_got_interrupt = true;
  tcs3400_clear_all_interrupts();
}

int tcs3400_test_cmd(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    static bool s_initialized = false;
    if (!s_initialized) {
        watch_enable_pull_up(A4);
        watch_register_interrupt_callback(A4, prv_test_interrupt_handler, INTERRUPT_TRIGGER_FALLING);
        watch_enable_i2c();
        tcs3400_write_wtime(TCS3400_WTIME_103MS);
        tcs3400_atime_e atime = kAtime;
        tcs3400_write_again(s_again);
        tcs3400_write_atime(atime);
        tcs3400_write_apers(TCS3400_APERS_EVERY);
        tcs3400_start();
        tcs3400_clear_all_interrupts();
        s_initialized = true;
    }


    tcs3400_reg_status_t status;
    uint16_t clear, red, green, blue;
    tcs3400_read_data(&status, &clear, &red, &green, &blue);

    uint32_t ev_fixed = 0;
    uint32_t lux = 0;
    size_t iso = 100;
    bool result = tcs3400_ev_measure(&ev_fixed, &lux, iso);

    printf("valid(%i) clear(%i) red(%i) green(%i) blue(%i)\r\n",
           status.field.avalid, clear, red, green, blue);
    if (result) {
        printf("ev_raw(%08x) ev_round(%u) ev_whole(%u) ev_frac(%u) lux(%u)\r\n",
               ev_fixed,
               tcs3400_fixed_round_to_int(ev_fixed),
               tcs3400_fixed_get_whole(ev_fixed),
               tcs3400_fixed_get_frac_digit(ev_fixed),
               lux);
    }
    printf("interrupt(%u)\r\n", s_test_got_interrupt);
    s_test_got_interrupt = false;

    return 0;
}
