/*!
 * @file  Massmore_ST7789.h
 * @brief Driver สำหรับโมดูลจอ IPS TFT LCD ST7789 ของ Massmore (SKU-1021) — 1.3" / 1.54" 240x240 และ
 *        2.0" 240x320 พร้อม SD card และ Capacitive Touch (CST816) ใช้ได้ทั้ง Arduino IDE และ PlatformIO
 *
 * จุดเด่น
 *  - ต่อยอดจาก TFT_eSPI 2.5.43 (vendor ไว้ใน src/vendor/ ไม่ต้องติดตั้งไลบรารีอื่น) จึงใช้ API กราฟิกของ
 *    TFT_eSPI ได้ครบ: drawString / fillRect / pushImage / TFT_eSprite / DMA / Smooth Font ฯลฯ
 *  - ฟอนต์ไทย Kanit และ Sarabun ฝังมาให้ (Massmore_ST7789_Fonts.h) + drawThaiString() ที่จัดตำแหน่ง
 *    วรรณยุกต์ / สระบน / สระล่างให้ถูกต้อง (ที่ นี้ น้ำ ปิ่น ญุ ...) โดยอัตโนมัติ
 *  - Chip identity: verifyChipID() อ่าน RDID1/2/3 เทียบค่า datasheet, readStatus(), isGenuine(),
 *    testFrameMemory() เขียน-อ่าน GRAM กลับมาตรวจ
 *  - Touch CST816 อยู่ในคลาสแยก Massmore_ST7789_Touch (I2C, bus reference injection, ไม่ hardcode ขา)
 *  - Dual API: Simple Blocking (drawXxx) และ Non-blocking DMA (initDMA / pushImageDMA / dmaBusy)
 *
 * Pin / panel configuration: ST7789 ถูกขับด้วย TFT_eSPI ซึ่งใช้ขาแบบ compile-time (เพื่อความเร็ว)
 * ค่า default ต่อ MCU และวิธี override อยู่ที่ Massmore_ST7789_Config.h (ไฟล์เดียว)
 *
 * Bus ownership: sketch สามารถเรียก SPI.begin(sck, miso, mosi) เองก่อน begin() ได้ (แนะนำเมื่อใช้ร่วมกับ
 * SD card) — ถ้าไม่เรียก begin() จะ start bus ให้ด้วยขาจาก Config เพราะ TFT_eSPI ต้องการ bus ที่พร้อมแล้ว
 *
 * Supported MCUs: ESP32 (Classic), ESP32-S3 — Arduino-ESP32 Core 3.x+ (AVR ATmega328P ไม่รองรับ ดู README)
 *
 * Massmore_ST7789 — Designed and Manufactured by Massmore (https://www.massmore.shop)
 * Product: https://www.massmore.shop/products/5e1f85a3-6174-4861-aef3-dca6186013f2
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef MASSMORE_ST7789_H
#define MASSMORE_ST7789_H

#include <Arduino.h>

#if defined(__AVR__)
  #error "Massmore_ST7789: AVR ATmega328P (Arduino Nano/Uno) is not supported - 2 KB SRAM / 32 KB Flash cannot drive a 240x320 colour display with Thai smooth fonts. Use ESP32 or ESP32-S3."
#endif
#if defined(ESP32)
  #include <esp_arduino_version.h>
  #if ESP_ARDUINO_VERSION_MAJOR < 3
    #error "Massmore_ST7789 requires Arduino-ESP32 Core 3.x or newer (Boards Manager: esp32 by Espressif Systems >= 3.0.0)."
  #endif
#endif

#include <SPI.h>
#include "Massmore_ST7789_Config.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#include "vendor/TFT_eSPI/TFT_eSPI.h"
#pragma GCC diagnostic pop

#define MASSMORE_ST7789_VERSION_STR   "1.0.0"

/* ---- ค่าจาก datasheet ST7789VW v1.0 (Sitronix, 2017/09) ---------------------------------------- */
#define MASSMORE_ST7789_ID1           0x85   /*!< RDID1 (DAh): LCD module manufacturer ID   (§9.1.46) */
#define MASSMORE_ST7789_ID2           0x85   /*!< RDID2 (DBh): module / driver version ID   (§9.1.47) */
#define MASSMORE_ST7789_ID3           0x52   /*!< RDID3 (DCh): module / driver ID           (§9.1.48) */
/* ID ที่อ่านได้จริงจากโมดูล Massmore SKU-1021 (ทั้งรุ่น 2.0" และ 2.0" Touch, ตรวจบนฮาร์ดแวร์ 2026-10)
 * controller ล็อตนี้ตอบ RDID1/2/3 = 0x81 / 0x81 / 0xB3 และ RDDCOLMOD = 0x05 แต่ power mode, self-diagnostic
 * และ GRAM readback ตรงกับ ST7789V ทุกค่า → ยอมรับทั้งสองชุด */
#define MASSMORE_ST7789_ALT_ID1       0x81
#define MASSMORE_ST7789_ALT_ID2       0x81
#define MASSMORE_ST7789_ALT_ID3       0xB3
#define MASSMORE_ST7789_RDDID         0x04   /*!< Read Display ID (24-bit, 1 dummy clock)  (§9.1.3)  */
#define MASSMORE_ST7789_RDDST         0x09   /*!< Read Display Status (32-bit, dummy)      (§9.1.4)  */
#define MASSMORE_ST7789_RDDPM         0x0A   /*!< Read Display Power Mode                  (§9.1.5)  */
#define MASSMORE_ST7789_RDDMADCTL     0x0B   /*!< Read MADCTL                              (§9.1.6)  */
#define MASSMORE_ST7789_RDDCOLMOD     0x0C   /*!< Read pixel format                        (§9.1.7)  */
#define MASSMORE_ST7789_RDDIM         0x0D   /*!< Read image mode                          (§9.1.8)  */
#define MASSMORE_ST7789_RDDSM         0x0E   /*!< Read signal mode                         (§9.1.9)  */
#define MASSMORE_ST7789_RDDSDR        0x0F   /*!< Read self-diagnostic result              (§9.1.10) */
#define MASSMORE_ST7789_RDID1         0xDA
#define MASSMORE_ST7789_RDID2         0xDB
#define MASSMORE_ST7789_RDID3         0xDC
#define MASSMORE_ST7789_RDDPM_READY   0x9C   /*!< BSTON | SLPOUT | NORON | DISON หลัง begin()          */
#define MASSMORE_ST7789_RDDPM_MASK    0xFC   /*!< D1/D0 "not used"                                     */
#define MASSMORE_ST7789_COLMOD_16BIT  0x55   /*!< RGB I/F 16-bit + control I/F 16-bit (COLMOD 3Ah = 55h) */
#define MASSMORE_ST7789_COLMOD_MASK   0x07   /*!< SPI ใช้แค่ D2–D0 (control I/F format); D6–D4 เป็นของ RGB I/F */
#define MASSMORE_ST7789_SDR_OK        0xC0   /*!< D7 register loading OK, D6 functionality OK          */
#define MASSMORE_ST7789_T_RESET_US    10     /*!< TRW: reset pulse ≥ 10 µs                  (§7.4.5)  */
#define MASSMORE_ST7789_T_RESET_MS    120    /*!< TRT: reset cancel ≤ 120 ms (sleep-out)    (§7.4.5)  */
#define MASSMORE_ST7789_T_SLPOUT_MS   120    /*!< รอ 120 ms หลัง SLPOUT ก่อน SLPIN          (§9.1.12) */
#define MASSMORE_ST7789_T_SLPIN_MS    120    /*!< รอ 120 ms หลัง SLPIN ก่อน SLPOUT          (§9.1.11) */

#define MASSMORE_ST7789_WIDTH         TFT_WIDTH
#define MASSMORE_ST7789_HEIGHT        TFT_HEIGHT

#ifndef MASSMORE_ST7789_THAI_BUF
  #define MASSMORE_ST7789_THAI_BUF    256    /*!< ขนาด buffer (byte) สำหรับ drawThaiString — stack, ไม่ใช้ heap */
#endif

#if defined(TFT_SDA_READ) || (defined(TFT_MISO) && (TFT_MISO >= 0))
  #define MASSMORE_ST7789_CAN_READ    1
#else
  #define MASSMORE_ST7789_CAN_READ    0
#endif

/*!
 * @class Massmore_ST7789
 * @brief Driver object — สร้างหนึ่งตัวต่อจอหนึ่งจอ (สืบทอด TFT_eSPI ทั้งหมด)
 *
 * ตัวอย่างสั้นที่สุด:
 * @code
 *   #include <Massmore_ST7789.h>
 *   #include <Massmore_ST7789_Fonts.h>
 *   Massmore_ST7789 tft;
 *   void setup() {
 *     tft.begin(1);                                   // rotation 1 = landscape
 *     tft.loadFont(Massmore_Font_Kanit_28);
 *     tft.setTextColor(TFT_WHITE, TFT_BLACK);
 *     tft.drawThaiString("สวัสดี Massmore", 10, 10);
 *   }
 * @endcode
 */
class Massmore_ST7789 : public TFT_eSPI {
public:
    /*! @brief รหัสข้อผิดพลาดมาตรฐาน Massmore — อ่านด้วย lastError() */
    enum class ErrorCode : uint8_t {
        OK = 0,
        NOT_FOUND,        /*!< ไม่ได้รับข้อมูลจากจอ (อ่านได้ 0x00 / 0xFF ทั้งหมด) */
        WRONG_ID,         /*!< RDID1/2/3 ไม่ตรง 0x85 / 0x85 / 0x52 หรือ 0x81 / 0x81 / 0xB3 */
        TIMEOUT,
        CRC_FAIL,         /*!< GRAM readback ไม่ตรงค่าที่เขียน */
        BUS_ERROR,
        NOT_READY,        /*!< ยังไม่ได้เรียก begin() */
        NO_READ_SUPPORT   /*!< build นี้ปิดการอ่าน (MASSMORE_ST7789_NO_SDA_READ และ TFT_MISO = -1) */
    };

    /*! @brief ค่า ID 3 byte จาก RDID1 / RDID2 / RDID3 */
    struct ChipID {
        uint8_t id1, id2, id3;
        uint32_t raw() const { return ((uint32_t)id1 << 16) | ((uint32_t)id2 << 8) | id3; }
        /*! ตรงกับ ST7789VW ตาม datasheet (0x85 0x85 0x52) */
        bool isDatasheetID() const { return id1 == MASSMORE_ST7789_ID1 && id2 == MASSMORE_ST7789_ID2 && id3 == MASSMORE_ST7789_ID3; }
        /*! ตรงกับ controller ล็อตที่ใช้ในโมดูล Massmore SKU-1021 (0x81 0x81 0xB3) */
        bool isAltID() const { return id1 == MASSMORE_ST7789_ALT_ID1 && id2 == MASSMORE_ST7789_ALT_ID2 && id3 == MASSMORE_ST7789_ALT_ID3; }
        bool matches() const { return isDatasheetID() || isAltID(); }
    };

    /*! @brief สถานะจากกลุ่ม read command 0Ah–0Fh */
    struct Status {
        uint8_t powerMode;     /*!< RDDPM   0Ah — คาดหวัง 0x9C หลัง begin() */
        uint8_t madctl;        /*!< RDDMADCTL 0Bh */
        uint8_t pixelFormat;   /*!< RDDCOLMOD 0Ch — คาดหวัง (ค่า & 0x07) = 0x05 (16-bit) */
        uint8_t imageMode;     /*!< RDDIM   0Dh */
        uint8_t signalMode;    /*!< RDDSM   0Eh */
        uint8_t selfDiag;      /*!< RDDSDR  0Fh — คาดหวัง bit7 | bit6 = 0xC0 */
    };

    Massmore_ST7789();

    /* ===================================================================
     * SECTION 1 — Start-up / status
     * =================================================================== */

    /*!
     * @brief  เริ่มต้นจอ: hardware reset (RST), init sequence ของ ST7789, ตั้ง rotation, ล้างจอ
     * @param  rotation    0 = portrait, 1 = landscape, 2 / 3 = กลับด้าน (เหมือน TFT_eSPI::setRotation)
     * @param  clearColor  สีที่ใช้ล้างจอหลัง init (RGB565)
     * @return true เสมอเมื่อ init เสร็จ — จอเป็นอุปกรณ์ write-only จึงตรวจการมีอยู่ไม่ได้ 100%;
     *         ถ้า build รองรับการอ่าน จะอ่าน ID ให้และตั้ง lastError() = WRONG_ID / NOT_FOUND เมื่อไม่ตรง
     * @note   ถ้า sketch ยังไม่ได้เรียก SPI.begin() จะเรียกให้ด้วยขาจาก Massmore_ST7789_Config.h
     */
    bool begin(uint8_t rotation = 0, uint16_t clearColor = TFT_BLACK);

    bool      isReady() const { return _ready; }
    ErrorCode lastError() const { return _err; }
    static const char *errorToString(ErrorCode e);

    /*! @brief ความกว้าง / สูงของ panel ตาม Config (ไม่ขึ้นกับ rotation) */
    static constexpr uint16_t panelWidth()  { return TFT_WIDTH; }
    static constexpr uint16_t panelHeight() { return TFT_HEIGHT; }

    /*! @brief พิมพ์ค่า config ที่ compile มา (MCU, panel, pins, SPI clock, read support) — ใช้ debug การต่อสาย */
    void printConfig(Print &out = Serial);

    /* ===================================================================
     * SECTION 2 — Chip identity / authenticity (ตรวจของแท้)
     * =================================================================== */

    /*! @brief build นี้อ่านค่าจากจอได้หรือไม่ (TFT_SDA_READ หรือ TFT_MISO >= 0) */
    static constexpr bool hasReadSupport() { return MASSMORE_ST7789_CAN_READ != 0; }

    /*!
     * @brief  อ่าน RDID1/2/3 (DAh/DBh/DCh) แล้วเทียบกับ 0x85 / 0x85 / 0x52 (datasheet) หรือ 0x81 / 0x81 / 0xB3 (SKU-1021)
     * @return true = ตรง; false ให้ดู lastError() (WRONG_ID / NOT_FOUND / NO_READ_SUPPORT)
     */
    bool verifyChipID();

    /*! @brief อ่าน ID ดิบ 3 byte (ทั้งหมดเป็น 0x00 หรือ 0xFF = สาย SDA/MISO ไม่ตอบ) */
    ChipID readChipID();

    /*!
     * @brief  ST7789 ไม่มี serial number / lot register ใน datasheet — คืน 0 เสมอ
     * @return 0
     */
    uint32_t getSerialNumber() { return 0; }

    /*!
     * @brief  Heuristic ของแท้: ID ตรง + RDDPM = 0x9C + self-diagnostic (RDDSDR) ผ่านทั้ง 2 bit
     * @note   ต้องเรียกหลัง begin() และก่อน sleep()
     */
    bool isGenuine();

    /*! @brief อ่านสถานะทั้งกลุ่ม 0Ah–0Fh ในครั้งเดียว */
    Status  readStatus();
    uint8_t readPowerMode()      { return readRegister(MASSMORE_ST7789_RDDPM); }
    uint8_t readMADCTL()         { return readRegister(MASSMORE_ST7789_RDDMADCTL); }
    uint8_t readPixelFormat()    { return readRegister(MASSMORE_ST7789_RDDCOLMOD); }
    uint8_t readSelfDiagnostic() { return readRegister(MASSMORE_ST7789_RDDSDR); }

    /*! @brief RDDST (09h) 32-bit — ถอด dummy clock ให้แล้ว (bit31 = BSTON ... ตาม datasheet §9.1.4) */
    uint32_t readDisplayStatus();

    /*!
     * @brief  อ่าน parameter 1 byte ของ read command ใด ๆ (8-bit read, ไม่มี dummy clock)
     * @param  cmd  0Ah–0Fh, DAh–DCh ฯลฯ
     * @return ค่าที่อ่านได้ (0x00 เมื่อไม่รองรับการอ่าน)
     */
    uint8_t readRegister(uint8_t cmd);

    /*!
     * @brief  ทดสอบ frame memory: เขียน pixel สีที่รู้ค่าลง GRAM แล้วอ่านกลับ (RAMRD) เทียบกัน แล้วคืนค่าเดิม
     * @param  samples   จำนวน pixel ที่ทดสอบ (กระจายทั่วจอ)
     * @param  failures  [out] จำนวนที่อ่านกลับไม่ตรง
     * @return true เมื่อ failures == 0
     */
    bool testFrameMemory(uint8_t samples, uint8_t &failures);

    /* ===================================================================
     * SECTION 3 — Power / backlight
     * =================================================================== */

    /*! @brief DISPOFF + SLPIN แล้วรอ 120 ms ตาม datasheet (จอดับ, กินไฟต่ำ) */
    void sleep();
    /*! @brief SLPOUT (รอ 120 ms) + DISPON */
    void wake();
    /*! @brief เปิด / ปิดการแสดงผล (DISPON / DISPOFF) โดยไม่เข้า sleep */
    void displayOn(bool on);
    /*! @brief เปิด / ปิด backlight — ใช้ได้เมื่อกำหนด TFT_BL (ดู Config) มิฉะนั้นคืน false */
    bool setBacklight(bool on);
    /*!
     * @brief  ปรับความสว่าง backlight ด้วย PWM (ESP32 LEDC, Core 3.x ledcAttach) 0–255
     * @return false เมื่อไม่ได้กำหนด TFT_BL
     */
    bool setBrightness(uint8_t level);
    uint8_t getBrightness() const { return _brightness; }

    /* ===================================================================
     * SECTION 4 — Thai text (ใช้กับ Smooth Font ที่ loadFont() แล้ว)
     * =================================================================== */

    /*!
     * @brief  จัดรูปข้อความไทย (UTF-8) ให้วรรณยุกต์ / สระบน / สระล่างไปอยู่ตำแหน่งที่ถูกต้อง
     *         โดยแทน code point ด้วย glyph สำรองใน Private Use Area (U+F700–U+F71A) ที่ฟอนต์ Massmore มีให้
     * @param  utf8     ข้อความต้นทาง
     * @param  out      buffer ปลายทาง (ใช้ตัวเดียวกับ utf8 ได้ = in-place; ความยาวผลลัพธ์ = ต้นทางเสมอ)
     * @param  outSize  ขนาด out รวม null
     * @return จำนวน byte ที่เขียน (ไม่รวม null)
     */
    static size_t shapeThai(const char *utf8, char *out, size_t outSize);

    /*!
     * @brief  วาดข้อความไทย (หรือปนอังกฤษ) ด้วย Smooth Font ปัจจุบัน ที่ตำแหน่ง x, y ตาม setTextDatum()
     * @return ความกว้างเป็น pixel เหมือน drawString()
     * @note   ข้อความยาวกว่า MASSMORE_ST7789_THAI_BUF-1 byte จะถูกตัด; ถ้ายังไม่ loadFont() จะใช้ font ปกติ (แสดงไทยไม่ได้)
     */
    int16_t drawThaiString(const char *utf8, int32_t x, int32_t y);
    int16_t drawThaiString(const String &s, int32_t x, int32_t y) { return drawThaiString(s.c_str(), x, y); }

    /*! @brief ความกว้าง (pixel) ของข้อความไทยหลังจัดรูป — ใช้จัดกึ่งกลาง / ชิดขวาเอง */
    int16_t thaiTextWidth(const char *utf8);

    /*! @brief print() แบบจัดรูปไทย ที่ตำแหน่ง cursor ปัจจุบัน (setCursor) */
    size_t printThai(const char *utf8);

private:
    bool readRaw(uint8_t cmd, uint8_t *buf, uint8_t len);
    static uint16_t utf8Next(const char *s, size_t len, size_t &i);
    static size_t   utf8Put(char *out, size_t cap, size_t pos, uint16_t cp);

    ErrorCode _err;
    bool      _ready;
    bool      _pwmReady;
    uint8_t   _brightness;
};

#endif /* MASSMORE_ST7789_H */
