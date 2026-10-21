/*!
 * @file  Massmore_ST7789_Config.h
 * @brief จุดเดียวสำหรับตั้งค่า driver / panel / pins / SPI clock / fonts ของ Massmore_ST7789
 *
 * ทำไมต้องมีไฟล์นี้ (Why compile-time configuration?)
 *   Massmore_ST7789 ต่อยอดจาก TFT_eSPI ซึ่งออกแบบให้ขาสัญญาณและ driver เป็น compile-time
 *   constant เพื่อความเร็วสูงสุด (direct GPIO register write, DMA) จึงไม่สามารถส่งขาเข้า
 *   constructor ตอน runtime ได้เหมือนไลบรารี Massmore ตัวอื่น
 *   ค่าทุกตัวในไฟล์นี้ครอบด้วย #ifndef — override ได้ 2 ทางโดยไม่ต้องแก้ไลบรารี:
 *     1. PlatformIO : build_flags = -DTFT_CS=5 -DTFT_DC=2 -DMASSMORE_ST7789_MISO=19 ... ใน platformio.ini
 *     2. Arduino IDE: แก้ค่า default ในไฟล์นี้ (Documents/Arduino/libraries/Massmore_ST7789/src/)
 *
 * ค่า default ต่อ MCU มาจากเอกสารสินค้า SKU-1021 (https://www.massmore.shop/docs/32)
 *
 * Massmore_ST7789 — Designed and Manufactured by Massmore (https://www.massmore.shop)
 * SPDX-License-Identifier: MIT
 */
#ifndef MASSMORE_ST7789_CONFIG_H
#define MASSMORE_ST7789_CONFIG_H

/* =====================================================================================
 * 1. Panel variant (เลือกตามรุ่นจอที่ซื้อ)
 *      320 = 2.0"  240 x 320 (มี SD / Touch)          ← default
 *      240 = 1.3" / 1.54"  240 x 240 (7-pin, ไม่มี SD / Touch)
 * ===================================================================================== */
#ifndef MASSMORE_ST7789_PANEL
  #define MASSMORE_ST7789_PANEL 320
#endif

#define ST7789_DRIVER
#ifndef TFT_WIDTH
  #define TFT_WIDTH  240
#endif
#ifndef TFT_HEIGHT
  #define TFT_HEIGHT MASSMORE_ST7789_PANEL   /* 240x240 → TFT_eSPI เปิด CGRAM_OFFSET ให้เอง */
#endif

/* Colour order + inversion: ค่าที่ทดสอบแล้วกับจอ IPS ของ Massmore
 * ถ้าสีกลับ (แดง↔น้ำเงิน) ให้ -DTFT_RGB_ORDER=TFT_RGB, ถ้าภาพเป็นเนกาทีฟให้ -DMASSMORE_ST7789_NO_INVERSION */
#ifndef TFT_RGB_ORDER
  #define TFT_RGB_ORDER TFT_BGR
#endif
#ifndef MASSMORE_ST7789_NO_INVERSION
  #define TFT_INVERSION_ON
#endif

/* =====================================================================================
 * 2. Pins — default ตามเอกสารสินค้า เปลี่ยนได้ด้วย -D<PIN>=<gpio> (ดูหัวไฟล์)
 *    TFT_BL = -1 หมายถึงขา BLK ต่อ 3V3 ตรง (ไม่ควบคุม backlight ด้วย MCU)
 * ===================================================================================== */
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  /* ---- ESP32-S3: SPI2 (FSPI) IOMUX pins → ได้ความเร็วสูงสุด ---- */
  #ifndef TFT_MOSI
    #define TFT_MOSI 11
  #endif
  #ifndef TFT_SCLK
    #define TFT_SCLK 12
  #endif
  #ifndef MASSMORE_ST7789_MISO
    #define MASSMORE_ST7789_MISO 13   /* MISO จริงของ bus (SD card); จอเองอ่านผ่าน SDA bidirectional */
  #endif
  #ifndef TFT_CS
    #define TFT_CS   17
  #endif
  #ifndef TFT_DC
    #define TFT_DC    3
  #endif
  #ifndef TFT_RST
    #define TFT_RST  18
  #endif
#elif defined(CONFIG_IDF_TARGET_ESP32)
  /* ---- ESP32 (Classic): VSPI IOMUX pins — Primary Factory Test target ---- */
  #ifndef TFT_MOSI
    #define TFT_MOSI 23
  #endif
  #ifndef TFT_SCLK
    #define TFT_SCLK 18
  #endif
  #ifndef MASSMORE_ST7789_MISO
    #define MASSMORE_ST7789_MISO 19
  #endif
  #ifndef TFT_CS
    #define TFT_CS   16
  #endif
  #ifndef TFT_DC
    #define TFT_DC    2
  #endif
  #ifndef TFT_RST
    #define TFT_RST   4
  #endif
#elif defined(ESP32)
  /* ESP32-S2 / C3 / C6 ฯลฯ: ไม่ได้ทดสอบ — ต้องกำหนดขาเองด้วย -D ทุกตัว */
  #if !defined(TFT_MOSI) || !defined(TFT_SCLK) || !defined(TFT_CS) || !defined(TFT_DC) || !defined(TFT_RST) || !defined(MASSMORE_ST7789_MISO)
    #error "Massmore_ST7789: this ESP32 variant is not pre-configured. Define TFT_MOSI/TFT_SCLK/MASSMORE_ST7789_MISO/TFT_CS/TFT_DC/TFT_RST via build_flags."
  #endif
#else
  #error "Massmore_ST7789: supported MCUs are ESP32 (Classic) and ESP32-S3 (Arduino-ESP32 Core 3.x+). AVR ATmega328P has too little SRAM/Flash for a 240x320 colour display + Thai smooth fonts."
#endif

/* Backlight: ปล่อย TFT_BL ไว้ไม่กำหนด = ขา BLK ต่อ 3V3 ตรง; ถ้าต่อกับ GPIO ให้ -DTFT_BL=<gpio>
 * แล้วใช้ setBacklight() / setBrightness() ได้ (TFT_BACKLIGHT_ON = ระดับที่ทำให้ไฟติด) */
#if defined(TFT_BL) && (TFT_BL >= 0)
  #define MASSMORE_ST7789_BL TFT_BL
#else
  #undef  TFT_BL
  #define MASSMORE_ST7789_BL -1
#endif
#ifndef TFT_BACKLIGHT_ON
  #define TFT_BACKLIGHT_ON HIGH
#endif

/* =====================================================================================
 * 3. SPI bus
 *    Datasheet ST7789V §7.4.3: write clock cycle ≥ 16 ns (≤ 62.5 MHz), read cycle ≥ 150 ns (≤ 6.67 MHz)
 *    40 MHz เสถียรบนสายจั๊มเปอร์; สายสั้นบน IOMUX pin ลอง -DSPI_FREQUENCY=80000000 ได้
 * ===================================================================================== */
#ifndef SPI_FREQUENCY
  #define SPI_FREQUENCY       40000000
#endif
#ifndef SPI_READ_FREQUENCY
  #define SPI_READ_FREQUENCY   6000000   /* ≤ 6.67 MHz ตาม datasheet */
#endif
/* โมดูลใช้ 4-line serial interface I (SDA bidirectional, ไม่มีขา SDO แยก)
 * TFT_SDA_READ ทำให้ readcommand8() / readPixel() / verifyChipID() ทำงานได้บน ESP32
 * ในโหมดนี้ TFT_eSPI บังคับ TFT_MISO = -1 — ขา MISO จริง (ใช้กับ SD card) จึงถูกเก็บไว้ที่
 * MASSMORE_ST7789_MISO และไลบรารีจะ attach กลับให้หลังอ่านทุกครั้ง (vendor patch #6) */
/* ขา MISO จริงของ bus = MASSMORE_ST7789_MISO (override: -DMASSMORE_ST7789_MISO=<gpio>, -1 = ไม่ใช้ SD) */
#ifndef MASSMORE_ST7789_NO_SDA_READ
  #define TFT_SDA_READ
  #undef  TFT_MISO
  #define TFT_MISO -1                 /* TFT_eSPI ต้องการ -1 ในโหมด SDA read */
#else
  #undef  TFT_MISO
  #define TFT_MISO MASSMORE_ST7789_MISO   /* โมดูลที่มีขา SDO แยก: SDO ต่อกับ MISO ของ bus */
#endif
/* ใช้ object SPI ตัวเดียวกับ sketch / SD library (ESP32 Classic = VSPI, ESP32-S3 = FSPI) */
#define MASSMORE_ST7789_USE_GLOBAL_SPI

/* =====================================================================================
 * 4. Fonts ที่ compile เข้าไป (TFT_eSPI built-in) + Smooth Font สำหรับฟอนต์ไทย Kanit / Sarabun
 * ===================================================================================== */
/* ปิด #warning ของ TFT_eSPI เรื่อง TOUCH_CS (โมดูลนี้ใช้ CST816 ผ่าน I2C ไม่ใช่ XPT2046) */
#define DISABLE_ALL_LIBRARY_WARNINGS

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#endif /* MASSMORE_ST7789_CONFIG_H */
