/*
  04_ThaiFont_Kanit_Sarabun — Massmore_ST7789
  ---------------------------------------------------------------------------
  แสดงข้อความภาษาไทยด้วยฟอนต์ Kanit และ Sarabun ที่ฝังมากับไลบรารี (ไม่ต้องติดตั้ง / อัปโหลดไฟล์ฟอนต์)
  drawThaiString() จัดตำแหน่งวรรณยุกต์ / สระบน / สระล่างให้ถูกต้องอัตโนมัติ เช่น ที่ นี้ น้ำ ปิ่น ญุ

  ฟอนต์ที่ให้มา (ใช้เท่าที่ include — ที่ไม่ใช้ไม่กิน Flash):
    Massmore_Font_Kanit_20 / 28 / 40, Massmore_Font_Sarabun_24 / 32 / 48
  ต้องการขนาด / น้ำหนักอื่น: python3 extras/make_thai_font.py <font.ttf> <px> <ArrayName> <out.h>

  หน้าจอสลับทุก 4 วินาที (non-blocking): 1 Kanit, 2 Sarabun, 3 การจัดตำแหน่ง / พื้นหลัง

  Designed and Manufactured by Massmore — https://www.massmore.shop
*/

#include <Massmore_ST7789.h>
#include <Massmore_ST7789_Fonts.h>     // หรือ include เฉพาะ fonts/Massmore_Font_Kanit_28.h ฯลฯ

Massmore_ST7789 tft;
uint8_t  page = 0;
uint32_t tPage = 0;

static void pageKanit() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.loadFont(Massmore_Font_Kanit_40);
  tft.drawThaiString("สวัสดี Massmore", 8, 6);
  tft.loadFont(Massmore_Font_Kanit_28);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawThaiString("ที่นี่ น้ำ ปิ่น ป้า ฝ้าย ฟ้า", 8, 66);
  tft.drawThaiString("ญี่ปุ่น ตั๊กแตน ชื่อ ไม่มี", 8, 112);
  tft.loadFont(Massmore_Font_Kanit_20);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.drawThaiString("Kanit 40 / 28 / 20 px — จัดวรรณยุกต์อัตโนมัติ", 8, 160);
  tft.drawThaiString("อุณหภูมิ 25.5°C  ราคา ฿199", 8, 196);
  tft.unloadFont();
}

static void pageSarabun() {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.loadFont(Massmore_Font_Sarabun_48);
  tft.drawThaiString("สวัสดีครับ", 8, 0);
  tft.loadFont(Massmore_Font_Sarabun_32);
  tft.setTextColor(TFT_NAVY, TFT_WHITE);
  tft.drawThaiString("ที่นี่ น้ำ ปิ่น ป้า ฝ้าย ฟ้า", 8, 78);
  tft.drawThaiString("ญุ ฐุ กำ ตั๊ก ปุ๋ย ชื่อ", 8, 132);
  tft.loadFont(Massmore_Font_Sarabun_24);
  tft.setTextColor(TFT_DARKGREEN, TFT_WHITE);
  tft.drawThaiString("Sarabun 48 / 32 / 24 px", 8, 190);
  tft.unloadFont();
}

static void pageLayout() {
  tft.fillScreen(TFT_BLACK);
  tft.loadFont(Massmore_Font_Kanit_28);

  // จัดกึ่งกลางด้วย setTextDatum (ทำงานกับ drawThaiString เหมือน drawString)
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawThaiString("กึ่งกลางจอ", tft.width() / 2, 30);
  tft.setTextDatum(TR_DATUM);
  tft.drawThaiString("ชิดขวา", tft.width() - 8, 60);
  tft.setTextDatum(TL_DATUM);

  // พื้นหลังทึบ (ลบข้อความเก่าได้เอง) — พารามิเตอร์ที่ 3 = true
  tft.setTextColor(TFT_BLACK, TFT_ORANGE, true);
  tft.drawThaiString(" พื้นหลังสีส้ม ", 8, 100);
  tft.setTextColor(TFT_WHITE, TFT_BLACK, false);

  // ความกว้างข้อความ → วาดกรอบรอบข้อความ
  const char *msg = "วัดความกว้างได้";
  int16_t w = tft.thaiTextWidth(msg);
  tft.drawThaiString(msg, 8, 150);
  tft.drawRect(6, 146, w + 4, tft.fontHeight() + 4, TFT_RED);

  // printThai ที่ตำแหน่ง cursor
  tft.setCursor(8, 200);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.printThai("printThai() ");
  tft.print(millis() / 1000);
  tft.printThai(" วินาที");
  tft.unloadFont();
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) { }
  Serial.println(F("\nMassmore_ST7789 - 04_ThaiFont_Kanit_Sarabun"));
  tft.begin(1);                    // landscape 320x240

  // ดูผลการจัดรูปใน Serial (code point U+F7xx คือ glyph สำรองที่ถูกเลือก)
  char shaped[64];
  Massmore_ST7789::shapeThai("น้ำ ที่ ปิ่น", shaped, sizeof(shaped));
  Serial.print(F("shapeThai bytes: "));
  for (const char *p = shaped; *p; p++) { Serial.print((uint8_t)*p, HEX); Serial.print(' '); }
  Serial.println();

  pageKanit();
  tPage = millis();
}

void loop() {
  if ((uint32_t)(millis() - tPage) >= 4000UL) {
    tPage = millis();
    page = (page + 1) % 3;
    if (page == 0) pageKanit();
    else if (page == 1) pageSarabun();
    else pageLayout();
  }
}
