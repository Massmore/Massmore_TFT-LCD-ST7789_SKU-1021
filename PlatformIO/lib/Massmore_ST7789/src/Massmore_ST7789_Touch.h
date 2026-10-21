/*!
 * @file  Massmore_ST7789_Touch.h
 * @brief Driver สำหรับ capacitive touch controller CST816 บนโมดูล 2.0" Touch ของ Massmore (SKU-1021)
 *
 *  - I2C address 0x15, bus reference injection (Wire / Wire1) — sketch เป็นผู้เรียก Wire.begin(sda, scl)
 *  - INT / RST เป็น optional (-1 = ไม่ต่อ); ถ้าต่อ INT ไว้ update() จะอ่าน bus เฉพาะตอนมีการสัมผัส
 *  - Dual API: touched() / getTouch() แบบ blocking และ update() / isDataReady() / getReadings() แบบ FSM
 *  - ไม่มี dynamic allocation, ไม่ hardcode ขา
 *
 * Register map ที่ใช้ (จาก reference implementation ของ Massmore ที่ทดสอบกับบอร์ดจริง):
 *   0x01 GESTURE_ID, 0x02 FINGER_NUM, 0x03 X_H (bit3:0) 0x04 X_L, 0x05 Y_H (bit3:0) 0x06 Y_L,
 *   0xA7 CHIP_ID, 0xA9 FW_VERSION
 * TODO: [MASSMORE_INPUT_REQUIRED: CST816 datasheet] — ยืนยันค่า CHIP_ID ต่อรุ่น (S / T / D) และ gesture code
 *
 * Massmore_ST7789 — Designed and Manufactured by Massmore (https://www.massmore.shop)
 * SPDX-License-Identifier: MIT
 */
#ifndef MASSMORE_ST7789_TOUCH_H
#define MASSMORE_ST7789_TOUCH_H

#include <Arduino.h>
#include <Wire.h>

#define MASSMORE_ST7789_TOUCH_ADDR        0x15
#define MASSMORE_ST7789_TOUCH_REG_GESTURE 0x01
#define MASSMORE_ST7789_TOUCH_REG_FINGERS 0x02
#define MASSMORE_ST7789_TOUCH_REG_XH      0x03
#define MASSMORE_ST7789_TOUCH_REG_XL      0x04
#define MASSMORE_ST7789_TOUCH_REG_YH      0x05
#define MASSMORE_ST7789_TOUCH_REG_YL      0x06
#define MASSMORE_ST7789_TOUCH_REG_CHIPID  0xA7
#define MASSMORE_ST7789_TOUCH_REG_FWVER   0xA9
/* ค่า CHIP_ID ที่พบใน reference implementation ทั่วไป (ยังไม่ยืนยันจาก datasheet — ดู TODO ด้านบน) */
#define MASSMORE_ST7789_TOUCH_ID_CST816S  0xB4
#define MASSMORE_ST7789_TOUCH_ID_CST816T  0xB5
#define MASSMORE_ST7789_TOUCH_ID_CST816D  0xB6

/*!
 * @class Massmore_ST7789_Touch
 * @brief Touch object — สร้างหนึ่งตัวต่อจอสัมผัสหนึ่งจอ
 *
 * @code
 *   Massmore_ST7789_Touch touch;
 *   void setup() {
 *     Wire.begin(14, 15);                       // ESP32-S3 ตามเอกสารสินค้า
 *     touch.begin(Wire);                        // INT / RST ไม่ต่อ
 *     touch.setPanelSize(240, 320);
 *   }
 *   void loop() {
 *     uint16_t x, y;
 *     if (touch.getTouch(x, y, tft.getRotation())) tft.fillCircle(x, y, 3, TFT_RED);
 *   }
 * @endcode
 */
class Massmore_ST7789_Touch {
public:
    enum class ErrorCode : uint8_t { OK = 0, NOT_FOUND, WRONG_ID, TIMEOUT, CRC_FAIL, BUS_ERROR, NOT_READY };

    struct Point { uint16_t x; uint16_t y; };

    Massmore_ST7789_Touch();

    /*!
     * @brief  เริ่มต้น touch controller (sketch ต้องเรียก Wire.begin() มาก่อน)
     * @param  wirePort  TwoWire instance (Wire หรือ Wire1)
     * @param  intPin    ขา INT (active-low) หรือ -1
     * @param  rstPin    ขา RST หรือ -1 — ถ้าต่อจะ pulse reset ให้ (LOW 10 ms, รอ 50 ms)
     * @param  address   I2C address (default 0x15)
     * @return true เมื่อชิป ACK; false ให้ดู lastError()
     */
    bool begin(TwoWire &wirePort = Wire, int8_t intPin = -1, int8_t rstPin = -1,
               uint8_t address = MASSMORE_ST7789_TOUCH_ADDR);

    bool      isConnected();                 /*!< ACK probe ที่ address */
    uint8_t   readChipID();                  /*!< register 0xA7 */
    uint8_t   readFirmwareVersion();         /*!< register 0xA9 */
    /*! @brief true เมื่อ CHIP_ID เป็นหนึ่งใน 0xB4 / 0xB5 / 0xB6 (CST816S / T / D) */
    bool      verifyChipID();
    uint8_t   readGesture();                 /*!< register 0x01 (raw) */
    ErrorCode lastError() const { return _err; }
    static const char *errorToString(ErrorCode e);
    uint8_t   getChipID() const { return _chipId; }

    /* ---- Simple blocking API ---------------------------------------------------- */
    bool touched();                                    /*!< FINGER_NUM != 0 */
    bool getRaw(uint16_t &x, uint16_t &y);             /*!< พิกัดดิบ 12-bit ของ controller (portrait) */
    /*!
     * @brief  พิกัดสัมผัสแปลงเป็นพิกัดจอตาม rotation (0–3 เหมือน TFT_eSPI) — ต้อง setPanelSize() ให้ตรงจอ
     * @return true เมื่อมีการสัมผัสอยู่
     */
    bool getTouch(uint16_t &x, uint16_t &y, uint8_t rotation = 0);

    /* ---- Advanced non-blocking FSM ---------------------------------------------- */
    /*!
     * @brief  เรียกใน loop(): อ่าน controller เมื่อ INT ถูกดึง LOW (ถ้าต่อ) หรือครบ poll interval
     * @return true เมื่อมีข้อมูลใหม่ (อ่านด้วย getReadings())
     */
    bool update();
    bool isDataReady() const { return _dataReady; }
    bool getReadings(Point &p, uint8_t rotation = 0);  /*!< คืนพิกัดล่าสุด แล้วเคลียร์ flag */
    void setPollInterval(uint16_t ms) { _pollMs = ms; }

    /* ---- Calibration / orientation ---------------------------------------------- */
    void setPanelSize(uint16_t w, uint16_t h) { _panelW = w; _panelH = h; }
    void setInvert(bool invertX, bool invertY) { _invX = invertX; _invY = invertY; }
    void setSwapXY(bool swap) { _swap = swap; }

private:
    bool readRegs(uint8_t reg, uint8_t *buf, uint8_t len);
    void mapPoint(uint16_t rx, uint16_t ry, uint8_t rotation, uint16_t &x, uint16_t &y) const;

    TwoWire  *_wire;
    uint8_t   _addr;
    int8_t    _intPin, _rstPin;
    ErrorCode _err;
    bool      _ready;
    uint8_t   _chipId;
    uint16_t  _panelW, _panelH;
    bool      _invX, _invY, _swap;
    /* FSM */
    uint32_t  _lastPoll;
    uint16_t  _pollMs;
    bool      _dataReady;
    Point     _last;
};

#endif /* MASSMORE_ST7789_TOUCH_H */
