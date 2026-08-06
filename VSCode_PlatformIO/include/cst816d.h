#pragma once

#include <Arduino.h>

#define MAX_TOUCH_MAX_POINTS 1

#define TP_I2C_FREQ 400000
#define CST816D_ADDR 0x15
#define CST816D_ID_REG 0xA7

#define CST816D_TOUCH_NUM_REG 0X02
#define CST816D_TOUCH_XH_REG 0x03
#define CST816D_TOUCH_XL_REG 0x04
#define CST816D_TOUCH_YH_REG 0x05
#define CST816D_TOUCH_YL_REG 0x06

typedef struct {
    uint16_t x;
    uint16_t y;
} toucht_coords_t;

bool get_touch_coords(toucht_coords_t *toucht_coords);

bool touch_init(int tp_sda, int tp_scl, int tp_rst, int tp_int);
