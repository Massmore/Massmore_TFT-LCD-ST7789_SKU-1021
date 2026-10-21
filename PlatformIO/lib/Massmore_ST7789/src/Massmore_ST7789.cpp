/*!
 * @file  Massmore_ST7789.cpp
 * @brief Implementation ของ Massmore_ST7789 (ดูคำอธิบาย API ใน Massmore_ST7789.h)
 *
 * Massmore_ST7789 — Designed and Manufactured by Massmore (https://www.massmore.shop)
 * SPDX-License-Identifier: MIT
 */
#include "Massmore_ST7789.h"

/* ---- Thai code point classes (ต้องตรงกับ extras/make_thai_font.py) --------------------------- */
static const uint16_t TH_TONE[5]  = { 0x0E48, 0x0E49, 0x0E4A, 0x0E4B, 0x0E4C };                          /* ่ ้ ๊ ๋ ์ */
static const uint16_t TH_UPPER[8] = { 0x0E31, 0x0E34, 0x0E35, 0x0E36, 0x0E37, 0x0E47, 0x0E4D, 0x0E4E };  /* ั ิ ี ึ ื ็ ํ ๎ */
static const uint16_t TH_LOWER[3] = { 0x0E38, 0x0E39, 0x0E3A };                                          /* ุ ู ฺ */
#define TH_SARA_AM        0x0E33
#define PUA_TONE_LEFT     0xF700   /* +k  วรรณยุกต์เลื่อนซ้าย (บน ป ฝ ฟ)                 */
#define PUA_TONE_RAISED   0xF705   /* +k  วรรณยุกต์ยกสูง (หลังสระบน / ก่อน ำ)            */
#define PUA_TONE_RAISED_L 0xF70A   /* +k  ยกสูง + เลื่อนซ้าย                              */
#define PUA_UPPER_LEFT    0xF710   /* +j  สระบนเลื่อนซ้าย (บน ป ฝ ฟ)                     */
#define PUA_LOWER_DOWN    0xF718   /* +m  สระล่างเลื่อนลง (ใต้ ญ ฐ)                       */

static inline bool isThaiConsonant(uint16_t c) { return c >= 0x0E01 && c <= 0x0E2E; }
static inline bool isTallConsonant(uint16_t c) { return c == 0x0E1B || c == 0x0E1D || c == 0x0E1F; }   /* ป ฝ ฟ */
static inline bool isDescConsonant(uint16_t c) { return c == 0x0E0D || c == 0x0E10; }                   /* ญ ฐ   */
static int8_t indexOf(const uint16_t *tbl, uint8_t n, uint16_t c) {
    for (uint8_t i = 0; i < n; i++) if (tbl[i] == c) return (int8_t)i;
    return -1;
}

/* ============================================================================================ */
Massmore_ST7789::Massmore_ST7789()
    : TFT_eSPI(TFT_WIDTH, TFT_HEIGHT), _err(ErrorCode::NOT_READY), _ready(false), _pwmReady(false), _brightness(255) {}

const char *Massmore_ST7789::errorToString(ErrorCode e) {
    switch (e) {
        case ErrorCode::OK:              return "OK";
        case ErrorCode::NOT_FOUND:       return "NOT_FOUND";
        case ErrorCode::WRONG_ID:        return "WRONG_ID";
        case ErrorCode::TIMEOUT:         return "TIMEOUT";
        case ErrorCode::CRC_FAIL:        return "CRC_FAIL";
        case ErrorCode::BUS_ERROR:       return "BUS_ERROR";
        case ErrorCode::NOT_READY:       return "NOT_READY";
        case ErrorCode::NO_READ_SUPPORT: return "NO_READ_SUPPORT";
    }
    return "UNKNOWN";
}

/* ---------------------------------------------------------------- SECTION 1: start-up */
bool Massmore_ST7789::begin(uint8_t rotation, uint16_t clearColor) {
#if defined(ESP32)
    /* TFT_eSPI จะเรียก spi.begin(SCLK, -1, MOSI) เองถ้า bus ยังไม่เริ่ม — เริ่มให้ก่อนพร้อมขา MISO จริง
     * เพื่อให้ SD card บน bus เดียวกันใช้งานได้ (sketch ที่เรียก SPI.begin() เองมาก่อนจะไม่ถูกแตะ) */
    if (SPI.bus() == nullptr) {
        SPI.begin(TFT_SCLK, MASSMORE_ST7789_MISO, TFT_MOSI, -1);
    }
#endif
    init();                 /* RST pulse (≥10 µs low, 120 ms settle) + ST7789 init sequence + SLPOUT/DISPON */
    setRotation(rotation);
    fillScreen(clearColor);
    _ready = true;
    _err   = ErrorCode::OK;
    if (hasReadSupport()) {
        verifyChipID();     /* บันทึกผลไว้ใน lastError() เท่านั้น — จอ write-only ก็ยังใช้ได้ */
    }
    return true;
}

void Massmore_ST7789::printConfig(Print &out) {
    out.println(F("Massmore_ST7789 v" MASSMORE_ST7789_VERSION_STR " (TFT_eSPI " TFT_ESPI_VERSION ")"));
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    out.println(F("  MCU        : ESP32-S3 (SPI2/FSPI)"));
#elif defined(CONFIG_IDF_TARGET_ESP32)
    out.println(F("  MCU        : ESP32 Classic (VSPI)"));
#else
    out.println(F("  MCU        : ESP32 variant"));
#endif
    out.print(F("  Panel      : ")); out.print(TFT_WIDTH); out.print('x'); out.print(TFT_HEIGHT);
    out.println(TFT_HEIGHT == 240 ? F(" (1.3\" / 1.54\")") : F(" (2.0\")"));
    out.print(F("  MOSI/SDA   : ")); out.println(TFT_MOSI);
    out.print(F("  SCLK/SCL   : ")); out.println(TFT_SCLK);
    out.print(F("  MISO (SD)  : ")); out.println(MASSMORE_ST7789_MISO);
    out.print(F("  CS         : ")); out.println(TFT_CS);
    out.print(F("  DC         : ")); out.println(TFT_DC);
    out.print(F("  RST        : ")); out.println(TFT_RST);
    out.print(F("  BL         : ")); out.println(MASSMORE_ST7789_BL);
    out.print(F("  SPI write  : ")); out.print(SPI_FREQUENCY / 1000000UL); out.println(F(" MHz"));
    out.print(F("  SPI read   : ")); out.print(SPI_READ_FREQUENCY / 1000000UL); out.println(F(" MHz"));
#ifdef TFT_SDA_READ
    out.println(F("  Read mode  : SDA bidirectional (TFT_SDA_READ)"));
#elif MASSMORE_ST7789_CAN_READ
    out.println(F("  Read mode  : separate SDO/MISO"));
#else
    out.println(F("  Read mode  : none (write-only build)"));
#endif
    out.print(F("  Colour     : ")); out.print(TFT_RGB_ORDER == TFT_BGR ? F("BGR") : F("RGB"));
#ifdef TFT_INVERSION_ON
    out.println(F(", inversion ON"));
#else
    out.println(F(", inversion OFF"));
#endif
}

/* ---------------------------------------------------------------- SECTION 2: identity */
bool Massmore_ST7789::readRaw(uint8_t cmd, uint8_t *buf, uint8_t len) {
#if !MASSMORE_ST7789_CAN_READ
    (void)cmd; for (uint8_t i = 0; i < len; i++) buf[i] = 0;
    _err = ErrorCode::NO_READ_SUPPORT;
    return false;
#else
    if (!_ready) { _err = ErrorCode::NOT_READY; return false; }
    dmaWait();
    /* transaction ของเราเอง (TFT_eSPI คืน bus เป็น full-duplex ทุกครั้งหลัง end_tft_write) */
    SPI.beginTransaction(SPISettings(SPI_READ_FREQUENCY, MSBFIRST, SPI_MODE0));
  #if TFT_CS >= 0
    digitalWrite(TFT_CS, LOW);
  #endif
    digitalWrite(TFT_DC, LOW);
    SPI.transfer(cmd);
    digitalWrite(TFT_DC, HIGH);
  #ifdef TFT_SDA_READ
    begin_SDA_Read();               /* MOSI → input ต่อกับ MISO signal (half-duplex SDA) */
  #endif
    for (uint8_t i = 0; i < len; i++) buf[i] = SPI.transfer(0x00);
  #ifdef TFT_SDA_READ
    end_SDA_Read();                 /* MOSI กลับเป็น output, attach MISO จริงคืน (vendor patch #6) */
  #endif
  #if TFT_CS >= 0
    digitalWrite(TFT_CS, HIGH);
  #endif
    SPI.endTransaction();
    return true;
#endif
}

uint8_t Massmore_ST7789::readRegister(uint8_t cmd) {
    uint8_t v = 0;
    readRaw(cmd, &v, 1);
    return v;
}

Massmore_ST7789::ChipID Massmore_ST7789::readChipID() {
    ChipID id = { 0, 0, 0 };
    id.id1 = readRegister(MASSMORE_ST7789_RDID1);
    id.id2 = readRegister(MASSMORE_ST7789_RDID2);
    id.id3 = readRegister(MASSMORE_ST7789_RDID3);
    return id;
}

bool Massmore_ST7789::verifyChipID() {
    if (!hasReadSupport()) { _err = ErrorCode::NO_READ_SUPPORT; return false; }
    if (!_ready)           { _err = ErrorCode::NOT_READY;       return false; }
    ChipID id = readChipID();
    if (id.matches()) { _err = ErrorCode::OK; return true; }
    uint32_t raw = id.raw();
    _err = (raw == 0x000000UL || raw == 0xFFFFFFUL) ? ErrorCode::NOT_FOUND : ErrorCode::WRONG_ID;
    return false;
}

Massmore_ST7789::Status Massmore_ST7789::readStatus() {
    Status s;
    s.powerMode   = readRegister(MASSMORE_ST7789_RDDPM);
    s.madctl      = readRegister(MASSMORE_ST7789_RDDMADCTL);
    s.pixelFormat = readRegister(MASSMORE_ST7789_RDDCOLMOD);
    s.imageMode   = readRegister(MASSMORE_ST7789_RDDIM);
    s.signalMode  = readRegister(MASSMORE_ST7789_RDDSM);
    s.selfDiag    = readRegister(MASSMORE_ST7789_RDDSDR);
    return s;
}

uint32_t Massmore_ST7789::readDisplayStatus() {
    uint8_t b[5] = { 0, 0, 0, 0, 0 };
    if (!readRaw(MASSMORE_ST7789_RDDST, b, 5)) return 0;
    /* serial 32-bit read มี dummy clock 1 bit นำหน้า (datasheet §8.4.5) → เลื่อนซ้าย 1 bit ข้าม byte */
    uint32_t v = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
    return (v << 1) | (b[4] >> 7);
}

bool Massmore_ST7789::isGenuine() {
    if (!verifyChipID()) return false;
    uint8_t pm  = readPowerMode();
    uint8_t sdr = readSelfDiagnostic();
    bool ok = ((pm & MASSMORE_ST7789_RDDPM_MASK) == MASSMORE_ST7789_RDDPM_READY) &&
              ((sdr & MASSMORE_ST7789_SDR_OK) == MASSMORE_ST7789_SDR_OK);
    /* TODO: [MASSMORE_INPUT_REQUIRED: authenticity signature] — datasheet ST7789VW ไม่มี trim / lot register
     * ที่อ่านได้ จึงใช้ ID + power mode + self-diagnostic เป็น heuristic */
    if (!ok) _err = ErrorCode::WRONG_ID;
    return ok;
}

bool Massmore_ST7789::testFrameMemory(uint8_t samples, uint8_t &failures) {
    static const uint16_t colours[8] = { TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, 0x8410 };
    failures = 0;
    if (!hasReadSupport()) { _err = ErrorCode::NO_READ_SUPPORT; failures = samples; return false; }
    if (!_ready)           { _err = ErrorCode::NOT_READY; failures = samples; return false; }
    for (uint8_t i = 0; i < samples; i++) {
        int32_t  x   = (int32_t)((i * 37UL + 3) % width());
        int32_t  y   = (int32_t)((i * 53UL + 5) % height());
        uint16_t old = readPixel(x, y);
        uint16_t c   = colours[i & 7];
        drawPixel(x, y, c);
        uint16_t rb  = readPixel(x, y);
        drawPixel(x, y, old);
        /* RAMRD คืน 18-bit → 16-bit: อนุญาต ±1 LSB ต่อ channel */
        int dr = (int)((rb >> 11) & 0x1F) - (int)((c >> 11) & 0x1F);
        int dg = (int)((rb >>  5) & 0x3F) - (int)((c >>  5) & 0x3F);
        int db = (int)( rb        & 0x1F) - (int)( c        & 0x1F);
        if (dr < -1 || dr > 1 || dg < -1 || dg > 1 || db < -1 || db > 1) failures++;
    }
    _err = failures ? ErrorCode::CRC_FAIL : ErrorCode::OK;
    return failures == 0;
}

/* ---------------------------------------------------------------- SECTION 3: power */
void Massmore_ST7789::sleep() {
    writecommand(TFT_DISPOFF);
    writecommand(TFT_SLPIN);
    delay(MASSMORE_ST7789_T_SLPIN_MS);
}

void Massmore_ST7789::wake() {
    writecommand(TFT_SLPOUT);
    delay(MASSMORE_ST7789_T_SLPOUT_MS);
    writecommand(TFT_DISPON);
}

void Massmore_ST7789::displayOn(bool on) {
    writecommand(on ? TFT_DISPON : TFT_DISPOFF);
}

bool Massmore_ST7789::setBacklight(bool on) {
#if MASSMORE_ST7789_BL >= 0
    if (_pwmReady) return setBrightness(on ? 255 : 0);
    pinMode(MASSMORE_ST7789_BL, OUTPUT);
    digitalWrite(MASSMORE_ST7789_BL, on ? TFT_BACKLIGHT_ON : !TFT_BACKLIGHT_ON);
    _brightness = on ? 255 : 0;
    return true;
#else
    (void)on;
    return false;
#endif
}

bool Massmore_ST7789::setBrightness(uint8_t level) {
#if (MASSMORE_ST7789_BL >= 0) && defined(ESP32)
    if (!_pwmReady) {
        if (!ledcAttach(MASSMORE_ST7789_BL, 5000, 8)) return false;   /* Core 3.x API: 5 kHz, 8-bit */
        _pwmReady = true;
    }
    _brightness = level;
    ledcWrite(MASSMORE_ST7789_BL, (TFT_BACKLIGHT_ON == HIGH) ? level : (uint8_t)(255 - level));
    return true;
#else
    (void)level;
    return false;
#endif
}

/* ---------------------------------------------------------------- SECTION 4: Thai text */
uint16_t Massmore_ST7789::utf8Next(const char *s, size_t len, size_t &i) {
    uint8_t c = (uint8_t)s[i++];
    if (c < 0x80) return c;
    if ((c & 0xE0) == 0xC0 && i < len)     { uint16_t cp = (c & 0x1F) << 6;  cp |= (uint8_t)s[i++] & 0x3F; return cp; }
    if ((c & 0xF0) == 0xE0 && i + 1 < len) { uint16_t cp = (c & 0x0F) << 12; cp |= ((uint8_t)s[i++] & 0x3F) << 6; cp |= (uint8_t)s[i++] & 0x3F; return cp; }
    if ((c & 0xF8) == 0xF0 && i + 2 < len) { i += 3; return '?'; }     /* นอก BMP — TFT_eSPI ไม่รองรับ */
    return '?';
}

size_t Massmore_ST7789::utf8Put(char *out, size_t cap, size_t pos, uint16_t cp) {
    if (cp < 0x80) {
        if (pos + 1 >= cap) return pos;
        out[pos++] = (char)cp;
    } else if (cp < 0x800) {
        if (pos + 2 >= cap) return pos;
        out[pos++] = (char)(0xC0 | (cp >> 6));
        out[pos++] = (char)(0x80 | (cp & 0x3F));
    } else {
        if (pos + 3 >= cap) return pos;
        out[pos++] = (char)(0xE0 | (cp >> 12));
        out[pos++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[pos++] = (char)(0x80 | (cp & 0x3F));
    }
    return pos;
}

size_t Massmore_ST7789::shapeThai(const char *utf8, char *out, size_t outSize) {
    if (!utf8 || !out || outSize == 0) return 0;
    size_t len = strlen(utf8);
    size_t i = 0, pos = 0;
    bool tall = false, desc = false, upper = false;
    uint16_t cp   = (len > 0) ? utf8Next(utf8, len, i) : 0;
    while (cp != 0) {
        uint16_t next = (i < len) ? utf8Next(utf8, len, i) : 0;   /* lookahead 1 ตัว (สำหรับ ำ) */
        uint16_t outCp = cp;
        int8_t k;
        if (isThaiConsonant(cp)) {
            tall = isTallConsonant(cp); desc = isDescConsonant(cp); upper = false;
        } else if ((k = indexOf(TH_UPPER, 8, cp)) >= 0) {
            if (tall) outCp = PUA_UPPER_LEFT + k;
            upper = true;
        } else if ((k = indexOf(TH_LOWER, 3, cp)) >= 0) {
            if (desc) outCp = PUA_LOWER_DOWN + k;
        } else if ((k = indexOf(TH_TONE, 5, cp)) >= 0) {
            bool raised = upper || (next == TH_SARA_AM);
            if (raised) outCp = (tall ? PUA_TONE_RAISED_L : PUA_TONE_RAISED) + k;
            else if (tall) outCp = PUA_TONE_LEFT + k;
        } else if (cp != TH_SARA_AM) {
            tall = desc = upper = false;    /* สระหน้า/หลัง, เว้นวรรค, อักษรอื่น → เริ่ม cluster ใหม่ */
        }
        pos = utf8Put(out, outSize, pos, outCp);
        cp = next;
    }
    out[pos] = '\0';
    return pos;
}

int16_t Massmore_ST7789::drawThaiString(const char *utf8, int32_t x, int32_t y) {
    char buf[MASSMORE_ST7789_THAI_BUF];
    shapeThai(utf8, buf, sizeof(buf));
    /* smooth font ของ TFT_eSPI ตัดบรรทัดเองเมื่อเกินขอบจอ (textwrapX = true เป็นค่าเริ่มต้น) ซึ่งจะตัด
     * กลาง cluster ไทย — วรรณยุกต์ / สระบนหลุดไปบรรทัดใหม่ → ปิด wrap ชั่วคราวให้เหมือน drawString() ปกติ */
    bool wrapX = textwrapX, wrapY = textwrapY;
    textwrapX = textwrapY = false;
    int16_t w = drawString(buf, x, y);
    textwrapX = wrapX; textwrapY = wrapY;
    return w;
}

int16_t Massmore_ST7789::thaiTextWidth(const char *utf8) {
    char buf[MASSMORE_ST7789_THAI_BUF];
    shapeThai(utf8, buf, sizeof(buf));
    return textWidth(buf);
}

size_t Massmore_ST7789::printThai(const char *utf8) {
    char buf[MASSMORE_ST7789_THAI_BUF];
    shapeThai(utf8, buf, sizeof(buf));
    return print(buf);
}
