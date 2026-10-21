/*!
 * @file  Massmore_ST7789_Touch.cpp
 * @brief Implementation ของ Massmore_ST7789_Touch (CST816, I2C)
 * SPDX-License-Identifier: MIT
 */
#include "Massmore_ST7789_Touch.h"

Massmore_ST7789_Touch::Massmore_ST7789_Touch()
    : _wire(nullptr), _addr(MASSMORE_ST7789_TOUCH_ADDR), _intPin(-1), _rstPin(-1), _err(ErrorCode::NOT_READY),
      _ready(false), _chipId(0), _panelW(240), _panelH(320), _invX(MASSMORE_ST7789_TOUCH_INVERT_X != 0), _invY(MASSMORE_ST7789_TOUCH_INVERT_Y != 0),
      _swap(MASSMORE_ST7789_TOUCH_SWAP_XY != 0),
      _lastPoll(0), _pollMs(20), _dataReady(false) { _last.x = 0; _last.y = 0; }

const char *Massmore_ST7789_Touch::errorToString(ErrorCode e) {
    switch (e) {
        case ErrorCode::OK:        return "OK";
        case ErrorCode::NOT_FOUND: return "NOT_FOUND";
        case ErrorCode::WRONG_ID:  return "WRONG_ID";
        case ErrorCode::TIMEOUT:   return "TIMEOUT";
        case ErrorCode::CRC_FAIL:  return "CRC_FAIL";
        case ErrorCode::BUS_ERROR: return "BUS_ERROR";
        case ErrorCode::NOT_READY: return "NOT_READY";
    }
    return "UNKNOWN";
}

bool Massmore_ST7789_Touch::begin(TwoWire &wirePort, int8_t intPin, int8_t rstPin, uint8_t address) {
    _wire = &wirePort; _addr = address; _intPin = intPin; _rstPin = rstPin;
    if (_intPin >= 0) pinMode(_intPin, INPUT_PULLUP);
    if (_rstPin >= 0) {
        pinMode(_rstPin, OUTPUT);
        digitalWrite(_rstPin, LOW);  delay(10);
        digitalWrite(_rstPin, HIGH); delay(50);
    }
    _ready = true;
    if (!isConnected()) { _ready = false; _err = ErrorCode::NOT_FOUND; return false; }
    _chipId = readChipID();
    _err = ErrorCode::OK;
    return true;
}

bool Massmore_ST7789_Touch::isConnected() {
    if (!_wire) return false;
    _wire->beginTransmission(_addr);
    return _wire->endTransmission() == 0;
}

bool Massmore_ST7789_Touch::readRegs(uint8_t reg, uint8_t *buf, uint8_t len) {
    if (!_wire || !_ready) { _err = ErrorCode::NOT_READY; return false; }
    _wire->beginTransmission(_addr);
    _wire->write(reg);
    if (_wire->endTransmission(false) != 0) { _err = ErrorCode::BUS_ERROR; return false; }
    uint8_t n = _wire->requestFrom((uint8_t)_addr, len);
    if (n != len) { _err = ErrorCode::TIMEOUT; return false; }
    for (uint8_t i = 0; i < len; i++) buf[i] = (uint8_t)_wire->read();
    _err = ErrorCode::OK;
    return true;
}

uint8_t Massmore_ST7789_Touch::readChipID()          { uint8_t v = 0; readRegs(MASSMORE_ST7789_TOUCH_REG_CHIPID, &v, 1); return v; }
uint8_t Massmore_ST7789_Touch::readFirmwareVersion() { uint8_t v = 0; readRegs(MASSMORE_ST7789_TOUCH_REG_FWVER,  &v, 1); return v; }
uint8_t Massmore_ST7789_Touch::readGesture()         { uint8_t v = 0; readRegs(MASSMORE_ST7789_TOUCH_REG_GESTURE, &v, 1); return v; }

bool Massmore_ST7789_Touch::verifyChipID() {
    uint8_t id = readChipID();
    _chipId = id;
    bool ok = (id == MASSMORE_ST7789_TOUCH_ID_CST816S || id == MASSMORE_ST7789_TOUCH_ID_CST816T || id == MASSMORE_ST7789_TOUCH_ID_CST816D);
    if (!ok) _err = (id == 0x00 || id == 0xFF) ? ErrorCode::NOT_FOUND : ErrorCode::WRONG_ID;
    return ok;
}

bool Massmore_ST7789_Touch::touched() {
    uint8_t n = 0;
    return readRegs(MASSMORE_ST7789_TOUCH_REG_FINGERS, &n, 1) && n != 0;
}

bool Massmore_ST7789_Touch::getRaw(uint16_t &x, uint16_t &y) {
    uint8_t b[5];   /* 0x02 FINGER_NUM, 0x03 XH, 0x04 XL, 0x05 YH, 0x06 YL — อ่านรวดเดียว */
    if (!readRegs(MASSMORE_ST7789_TOUCH_REG_FINGERS, b, 5)) return false;
    if (b[0] == 0) return false;
    x = ((uint16_t)(b[1] & 0x0F) << 8) | b[2];
    y = ((uint16_t)(b[3] & 0x0F) << 8) | b[4];
    return true;
}

void Massmore_ST7789_Touch::mapPoint(uint16_t rx, uint16_t ry, uint8_t rotation, uint16_t &x, uint16_t &y) const {
    if (_swap) { uint16_t t = rx; rx = ry; ry = t; }
    if (rx >= _panelW) rx = _panelW - 1;
    if (ry >= _panelH) ry = _panelH - 1;
    if (_invX) rx = _panelW - 1 - rx;
    if (_invY) ry = _panelH - 1 - ry;
    /* ควบคุมพิกัดดิบเป็น portrait (rotation 0) — หมุนตาม MADCTL ของ TFT_eSPI rotation 1..3 */
    switch (rotation & 3) {
        default: x = rx;                 y = ry;                 break;   /* 0: portrait            */
        case 1:  x = ry;                 y = _panelW - 1 - rx;   break;   /* 1: landscape (MX|MV)   */
        case 2:  x = _panelW - 1 - rx;   y = _panelH - 1 - ry;   break;   /* 2: portrait inverted   */
        case 3:  x = _panelH - 1 - ry;   y = rx;                 break;   /* 3: landscape inverted  */
    }
}

bool Massmore_ST7789_Touch::getTouch(uint16_t &x, uint16_t &y, uint8_t rotation) {
    uint16_t rx, ry;
    if (!getRaw(rx, ry)) return false;
    mapPoint(rx, ry, rotation, x, y);
    return true;
}

bool Massmore_ST7789_Touch::update() {
    if (!_ready) return false;
    if (_intPin >= 0) {
        if (digitalRead(_intPin) != LOW) return false;      /* INT active-low: ไม่มีสัมผัส → ไม่แตะ bus */
    } else {
        uint32_t now = millis();
        if ((uint32_t)(now - _lastPoll) < _pollMs) return false;   /* rollover-safe */
        _lastPoll = now;
    }
    uint16_t rx, ry;
    if (!getRaw(rx, ry)) return false;
    _last.x = rx; _last.y = ry;
    _dataReady = true;
    return true;
}

bool Massmore_ST7789_Touch::getReadings(Point &p, uint8_t rotation) {
    if (!_dataReady) return false;
    mapPoint(_last.x, _last.y, rotation, p.x, p.y);
    _dataReady = false;
    return true;
}
