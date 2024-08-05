#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    TCS3400_REG_ENABLE = 0x80,
    TCS3400_REG_ATIME = 0x81,
    TCS3400_REG_WTIME = 0x83,
    TCS3400_REG_AILTL = 0x84,
    TCS3400_REG_AILTH = 0x85,
    TCS3400_REG_AIHTL = 0x86,
    TCS3400_REG_AIHTH = 0x87,
    TCS3400_REG_PERS = 0x8C,
    TCS3400_REG_CONFIG = 0x8D,
    TCS3400_REG_CONTROL = 0x8F,
    TCS3400_REG_AUX = 0x90,
    TCS3400_REG_REVID = 0x91,
    TCS3400_REG_ID = 0x92,
    TCS3400_REG_STATUS = 0x93,
    TCS3400_REG_CDATAL = 0x94,
    TCS3400_REG_CDATAH = 0x95,
    TCS3400_REG_RDATAL = 0x96,
    TCS3400_REG_RDATAH = 0x97,
    TCS3400_REG_GDATAL = 0x98,
    TCS3400_REG_GDATAH = 0x99,
    TCS3400_REG_BDATAL = 0x9A,
    TCS3400_REG_BDATAH = 0x9B,
    TCS3400_REG_IR = 0xC0,
    TCS3400_REG_IFORCE = 0xE4,
    TCS3400_REG_CICLEAR = 0xE6,
    TCS3400_REG_AICLEAR = 0xE7,
} tcs3400_reg_e;

typedef union {
    uint8_t raw;
    struct {
        bool pon : 1; // Power On
        bool aen : 1; // ADC Enable
        uint8_t reserved_2 : 1;
        bool wen : 1; // Wait Enable
        bool aien : 1; // ALS Interrupt Enable
        uint8_t reserved_5 : 1;
        bool sai : 1; // Sleep After Interrupt
        uint8_t reserved_7 : 1;
    } field;
} tcs3400_reg_enable_t;

typedef enum {
    TCS3400_ATIME_2_78MS = 0xFF,
    TCS3400_ATIME_27_8MS = 0xF6,
    TCS3400_ATIME_103MS = 0xDB,
    TCS3400_ATIME_178MS = 0xC0,
    TCS3400_ATIME_712MS = 0x00,
} tcs3400_atime_e;

typedef union {
    uint8_t raw;
    struct {
        tcs3400_atime_e atime : 8;
    } field;
} tcs3400_reg_atime_t;

typedef enum {
    TCS3400_WTIME_2_78MS = 0xFF,
    TCS3400_WTIME_27_8MS = 0xF6,
    TCS3400_WTIME_103MS = 0xDB,
    TCS3400_WTIME_178MS = 0xC0,
    TCS3400_WTIME_712MS = 0x00,
} tcs3400_wtime_e;

typedef union {
    uint8_t raw;
    struct {
        tcs3400_wtime_e wtime : 8;
    } field;
} tcs3400_reg_wtime_t;

typedef enum {
    TCS3400_APERS_EVERY = 0,
    TCS3400_APERS_ANY = 1,
    TCS3400_APERS_TWO_CONSECUTIVE = 2,
    TCS3400_APERS_THREE_CONSECUTIVE = 3,
    TCS3400_APERS_FIVE_CONSECUTIVE = 4,
    TCS3400_APERS_TEN_CONSECUTIVE = 5,
    TCS3400_APERS_FIFTEEN_CONSECUTIVE = 6,
    TCS3400_APERS_TWENTY_CONSECUTIVE = 7,
    TCS3400_APERS_TWENTY_FIVE_CONSECUTIVE = 8,
    TCS3400_APERS_THIRTY_CONSECUTIVE = 9,
    TCS3400_APERS_THIRTY_FIVE_CONSECUTIVE = 10,
    TCS3400_APERS_FOURTY_CONSECUTIVE = 11,
    TCS3400_APERS_FOURTY_FIVE_CONSECUTIVE = 12,
    TCS3400_APERS_FIFTY_CONSECUTIVE = 13,
    TCS3400_APERS_FIFTY_FIVE_CONSECUTIVE = 14,
    TCS3400_APERS_SIXTY_CONSECUTIVE = 15,
} tcs3400_apers_e;

typedef union {
    uint8_t raw;
    struct {
        tcs3400_apers_e apers : 4;
        uint8_t reserved : 4;
    } field;
} tcs3400_reg_pers_t;

typedef union {
    uint8_t raw;
    struct {
        uint8_t reserved_0 : 1;
        bool wlong : 1;
        uint8_t reserved_2_5 : 3;
        uint8_t reserved_6 : 1;
        uint8_t reserved_7 : 1;
    } field;
} tcs3400_reg_config_t;

typedef enum {
    TCS3400_AGAIN_1X = 0,
    TCS3400_AGAIN_4X = 1,
    TCS3400_AGAIN_16X = 2,
    TCS3400_AGAIN_64X = 3,
} tcs3400_again_e;

typedef union {
    uint8_t raw;
    struct {
        tcs3400_again_e again : 2;
        uint8_t reserved_2_7 : 6;
    } field;
} tcs3400_reg_control_t;

typedef union {
    uint8_t raw;
    struct {
        uint8_t reserved_0_4 : 5;
        bool asien; // Enable ALS Saturation Interrupt
        uint8_t reserved_6_7 : 2;
    } field;
} tcs3400_reg_aux_t;

typedef union {
    uint8_t raw;
    struct {
        uint8_t revid : 4;
        uint8_t reserved_4_7 : 4;
    } field;
} tcs3400_reg_revid_t;

typedef union {
    uint8_t raw;
    struct {
        uint8_t vid : 2;
        uint8_t id : 6;
    } field;
} tcs3400_reg_id_t;

typedef union {
    uint8_t raw;
    struct {
        bool avalid : 1; // RGBC Valid
        uint8_t reserved_1_3 : 3;
        bool aint : 1; // ALS Interrupt
        uint8_t reserved_5_6 : 2;
        bool asat : 1; // ALS Saturation
    } field;
} tcs3400_reg_status_t;

typedef union {
    uint8_t raw;
    struct {
        uint8_t reserved_0_6 : 7;
        bool ir : 1; // IR Sensor Access
    } field;
} tcs3400_reg_ir_t;


tcs3400_reg_id_t tcs3400_read_id(void);
uint16_t tcs3400_read_clear_ir_data(void);
uint16_t tcs3400_read_red_data(void);
uint16_t tcs3400_read_blue_data(void);
uint16_t tcs3400_read_green_data(void);
tcs3400_reg_enable_t tcs3400_read_enable(void);
tcs3400_reg_status_t tcs3400_read_status(void);
void tcs3400_read_data(tcs3400_reg_status_t *status,
                       uint16_t *clear,
                       uint16_t *red,
                       uint16_t *green,
                       uint16_t *blue);

void tcs3400_write_enable(tcs3400_reg_enable_t enable);
void tcs3400_write_again(tcs3400_again_e gain);
void tcs3400_write_atime(tcs3400_atime_e atime);
void tcs3400_write_wtime(tcs3400_wtime_e wtime);
void tcs3400_write_apers(tcs3400_apers_e apers);
void tcs3400_write_aux(tcs3400_reg_aux_t aux);

void tcs3400_clear_all_interrupts();

void tcs3400_start(void);
void tcs3400_disable(void);
void tcs3400_stop(void);
uint32_t tcs3400_atime_to_us(tcs3400_atime_e atime);
uint8_t tcs3400_again_to_gain(tcs3400_again_e again);

uint32_t tcs3400_get_saturation_count(tcs3400_atime_e atime);
uint8_t tcs3400_get_gain();

uint16_t tcs3400_ev_get_df();
void tcs3400_ev_set_df(uint16_t df);
void tcs3400_ev_setup();

uint32_t tcs3400_fixed_get_whole(uint32_t x);
uint32_t tcs3400_fixed_round_to_int(uint32_t x);
uint32_t tcs3400_fixed_get_frac_digit(uint32_t x);

//bool tcs3400_ev_measure(float *ev, size_t iso);
bool tcs3400_ev_measure(uint32_t *ev_fixed, size_t iso);

int tcs3400_test_cmd(int argc, char *argv[]);
