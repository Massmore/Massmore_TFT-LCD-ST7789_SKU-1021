/*!
 * @file  Massmore_ST7789_Fonts.h
 * @brief รวมฟอนต์ไทย Smooth Font ที่มากับไลบรารี (Kanit / Sarabun) — include ไฟล์นี้ไฟล์เดียวใช้ได้ทุกฟอนต์
 *
 * ฟอนต์ที่ไม่ได้ถูกอ้างถึงใน sketch จะไม่ถูก link เข้า firmware (ไม่เปลือง Flash)
 * ถ้าต้องการประหยัดเวลา compile ให้ include เฉพาะไฟล์ใน fonts/ ที่ใช้จริง
 *
 *   Array                     Font              Size   Flash
 *   Massmore_Font_Kanit_20    Kanit Regular     20 px  ~28 KB
 *   Massmore_Font_Kanit_28    Kanit Regular     28 px  ~48 KB
 *   Massmore_Font_Kanit_40    Kanit Regular     40 px  ~87 KB
 *   Massmore_Font_Sarabun_24  Sarabun Regular   24 px  ~38 KB
 *   Massmore_Font_Sarabun_32  Sarabun Regular   32 px  ~62 KB
 *   Massmore_Font_Sarabun_48  Sarabun Regular   48 px  ~134 KB
 *
 * ทุกฟอนต์มี ASCII 0x21-0x7E, ° µ, อักษรไทย U+0E01-U+0E5B และ glyph สำรองสำหรับการจัดวรรณยุกต์
 * (ดู extras/make_thai_font.py สำหรับสร้างขนาด / น้ำหนักอื่น)
 *
 * Usage:
 *   tft.loadFont(Massmore_Font_Kanit_28);
 *   tft.drawThaiString("สวัสดี Massmore", 10, 10);
 *   tft.unloadFont();
 */
#ifndef MASSMORE_ST7789_FONTS_H
#define MASSMORE_ST7789_FONTS_H
#include "fonts/Massmore_Font_Kanit_20.h"
#include "fonts/Massmore_Font_Kanit_28.h"
#include "fonts/Massmore_Font_Kanit_40.h"
#include "fonts/Massmore_Font_Sarabun_24.h"
#include "fonts/Massmore_Font_Sarabun_32.h"
#include "fonts/Massmore_Font_Sarabun_48.h"
#endif
