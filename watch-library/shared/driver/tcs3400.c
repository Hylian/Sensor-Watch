#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "tcs3400.h"
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
    return (prv_read_register(TCS3400_REG_CDATAH) << 8) |
            prv_read_register(TCS3400_REG_CDATAL);
}

uint16_t tcs3400_read_red_data(void) {
    return (prv_read_register(TCS3400_REG_RDATAH) << 8) |
            prv_read_register(TCS3400_REG_RDATAL);
}

uint16_t tcs3400_read_blue_data(void) {
    return (prv_read_register(TCS3400_REG_BDATAH) << 8) |
            prv_read_register(TCS3400_REG_BDATAL);
}

uint16_t tcs3400_read_green_data(void) {
    return (prv_read_register(TCS3400_REG_GDATAH) << 8) |
            prv_read_register(TCS3400_REG_GDATAL);
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

void tcs3400_start(void) {
    tcs3400_reg_enable_t enable = {0};
    enable.field.pon = 1;
    enable.field.aen = 1;
    enable.field.aien = 1;
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
    if (atime_ms > 154) { // if atime_ms > 154ms (digital saturation)
        return 65535;
    } else { // analog saturation
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
    //tcs3400_start();
    tcs3400_atime_e atime = kAtime;
    tcs3400_write_again(s_again);
    tcs3400_write_atime(atime);
    //tcs3400_write_apers(TCS3400_APERS_EVERY);
}

// Device Factor (calibration constant specific to this part)
static uint16_t s_df = 100;
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

static int32_t s_log2_fixed(uint32_t x) {
    x <<= FRACTIONAL_BITS;
    int32_t b = 1U << (FRACTIONAL_BITS - 1);
    int32_t result = 0;

    if (x == 0) {
        return INT32_MIN;
    }

    while (x < 1U << FRACTIONAL_BITS) {
        x <<= 1;
        result -= 1U << FRACTIONAL_BITS;
    }

    while (x >= 2U << FRACTIONAL_BITS) {
        x >>= 1;
        result += 1U << FRACTIONAL_BITS;
    }

    uint64_t z = x;

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
    return (x >> FRACTIONAL_BITS) + ((x & (1U << (FRACTIONAL_BITS) - 1)) >= (1U << (FRACTIONAL_BITS - 1)));
}

uint32_t tcs3400_fixed_get_frac_digit(uint32_t x) {
  uint64_t frac = ((uint64_t) TCS3400_FRAC_MASK(x)) << (32 - FRACTIONAL_BITS);
  frac *= 10;
  uint8_t digit_1 = (frac >> 32) % 10;
  frac *= 10;
  uint8_t digit_2 = (frac >> 32) % 10;
  printf(".%u%u\n", digit_1, digit_2);
  if (digit_2 >= 5) {
    digit_1++;
  }
  return digit_1;
}

bool tcs3400_ev_measure(uint32_t *ev_fixed, size_t iso) {
    if (!ev_fixed) {
        return false;
    }

    tcs3400_reg_status_t status = tcs3400_read_status();
    if (!status.field.avalid) {
        return false;
    }

    int32_t clear = tcs3400_read_clear_ir_data();
    int32_t red = tcs3400_read_red_data();
    int32_t green = tcs3400_read_green_data();
    int32_t blue = tcs3400_read_blue_data();

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

    const uint32_t atime_us = tcs3400_atime_to_us(kAtime);
    const uint8_t again_x = tcs3400_again_to_gain(s_again);

    // Glass Attenuation Factor
    const uint8_t ga = 1;
    const uint32_t cpl = (atime_us * again_x) / (ga * s_df);

    const int32_t r_coef = -20;
    const int32_t g_coef = 1000;
    const int32_t b_coef = -482;

    //int32_t ir = red/2 - clear/2 + green/2 + blue/2;
    int64_t ir = red/2;
    ir += green/2;
    ir += blue/2;
    ir -= clear/2;

    int32_t red_p = red - ir;
    int32_t green_p = green - ir;
    int32_t blue_p = blue - ir;

    int64_t red_c = red_p * r_coef;
    int64_t green_c = green_p * g_coef;
    int64_t blue_c = blue_p * b_coef;

    int64_t lux = ((red_c + green_c + blue_c) / cpl);
    if (lux < 0) {
      lux = 0;
    }
    //*ev = (log(((float) lux) * 100.0 / 12.5)) / (log(2.0)) + 2.6711635357704604;

    //*ev = (log(((float) lux) * ((float) iso) / 250.0)) / log(2.0) + 2.6711635357704604;
    *ev_fixed = s_log2_fixed((uint32_t)(lux * iso / 250)) + EV_OFFSET_FIXED;

    return true;
}

int tcs3400_test_cmd(int argc, char *argv[]) {
    static bool initialized = false;
    if (!initialized) {
        tcs3400_start();
        initialized = true;
    }

    //tcs3400_atime_e atime = TCS3400_ATIME_27_8MS;
    //tcs3400_again_e again = TCS3400_AGAIN_64X;
    tcs3400_atime_e atime = TCS3400_ATIME_178MS;
    tcs3400_again_e again = TCS3400_AGAIN_4X;
    tcs3400_write_again(again);
    tcs3400_write_atime(atime);
    tcs3400_reg_id_t id = tcs3400_read_id();
    uint16_t clear = tcs3400_read_clear_ir_data();
    uint16_t red = tcs3400_read_red_data();
    uint16_t green = tcs3400_read_green_data();
    uint16_t blue = tcs3400_read_blue_data();
    tcs3400_reg_enable_t enable = tcs3400_read_enable();
    tcs3400_reg_status_t status = tcs3400_read_status();
    printf("aen(%i) pon(%i) valid(%i) id(%02x) clear(%i) red(%i) green(%i) blue(%i)\r\n", enable.field.aen, enable.field.pon, status.field.avalid, id.raw, clear, red, green, blue);


    const uint32_t atime_us = tcs3400_atime_to_us(atime);
    const uint8_t again_x = tcs3400_again_to_gain(again);
    const uint8_t ga = 1;
    //const uint16_t df =  127 * 45 / 10 * 186 / 100; // todo
    const uint16_t df = 100;
    const uint32_t cpl = (atime_us * again_x) / (ga * df);

    //const int32_t r_coef = -97;
    const int32_t r_coef = -20;
    const int32_t g_coef = 1000;
    const int32_t b_coef = -482;

    int32_t ir = (((int32_t) red) + green + blue - clear) / 2;

    int32_t red_p = red - ir;
    int32_t green_p = green - ir;
    int32_t blue_p = blue - ir;

    int32_t red_c = red_p * r_coef;
    int32_t green_c = green_p * g_coef;
    int32_t blue_c = blue_p * b_coef;

    const int32_t offset = 0;

    uint32_t saturation_point;
    if ((256 - atime) > 63) { // if atime_ms > 154ms (digital saturation)
        saturation_point = 65535;
    } else { // analog saturation
        saturation_point = 1024 * (256 - atime);
        saturation_point = saturation_point - saturation_point/4;
    }

    if (clear >= saturation_point) {
        printf("saturation! %u > %u\r\n", clear, saturation_point);
    }

    int32_t lux = ((red_c + green_c + blue_c) / cpl) + offset;
    printf("lux(%i) cpl(%u) ir(%i) rc(%i) gc(%i) bc(%i)\r\n", lux, cpl, ir, red_c, green_c, blue_c);

    //float ev = log(((float) lux) / 2.5) / log(2.0);
    float ev = (log((float) lux) / log(2.0))+3.0;

    float ev2 = (log(((float) lux) * 100.0 / 12.5)) / (log(2.0)) + 2.6711635357704604;

    printf("EV(%f) EV2(%f) \r\n", ev, ev2);

    return 0;
}
