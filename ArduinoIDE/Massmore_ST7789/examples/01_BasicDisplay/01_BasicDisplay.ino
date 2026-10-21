/*
  01_BasicDisplay — Massmore_ST7789
  ---------------------------------------------------------------------------
  ตัวอย่างแรกสุด: init จอ, อ่าน Chip ID, วาดรูปทรง / สี / ข้อความด้วย Simple Blocking API
  (API กราฟิกทั้งหมดของ TFT_eSPI ใช้ได้ผ่าน object เดียวกัน)

  Wiring (ค่า default จาก Massmore_ST7789_Config.h — เปลี่ยนได้ด้วย build_flags ดูตัวอย่าง 02)
    ESP32 (Classic): MOSI/SDA 23, SCLK/SCL 18, CS 16, DC 2, RST 4, BLK → 3V3, VCC → 3V3, GND → GND
    ESP32-S3       : MOSI/SDA 11, SCLK/SCL 12, CS 17, DC 3, RST 18, BLK → 3V3, VCC → 3V3, GND → GND
  จอ 1.3" / 1.54" (240x240) ให้เพิ่ม -DMASSMORE_ST7789_PANEL=240 (PlatformIO) หรือแก้ใน Config

  Designed and Manufactured by Massmore — https://www.massmore.shop
*/

#include <Massmore_ST7789.h>

Massmore_ST7789 tft;

static void drawStaticScene() {
  tft.fillScreen(TFT_BLACK);

  // แถบสี 8 สี ตรวจ colour order / inversion ได้ด้วยตา (ซ้าย → ขวา: แดง เขียว น้ำเงิน ขาว เหลือง ฟ้า ม่วง เทา)
  const uint16_t bars[8] = { TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, TFT_DARKGREY };
  int32_t bw = tft.width() / 8;
  for (int i = 0; i < 8; i++) tft.fillRect(i * bw, 0, bw, 24, bars[i]);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Massmore ST7789", 8, 32, 4);           // built-in font 4 (26 px)
  tft.drawString("SKU-1021 IPS TFT LCD", 8, 60, 2);      // built-in font 2 (16 px)

  // รูปทรงพื้นฐาน
  tft.drawRoundRect(8, 84, tft.width() - 16, tft.height() - 92, 8, TFT_CYAN);
  tft.fillCircle(50, 130, 22, TFT_RED);
  tft.drawCircle(50, 130, 30, TFT_YELLOW);
  tft.fillTriangle(100, 152, 124, 108, 148, 152, TFT_GREEN);
  tft.fillRect(170, 108, 44, 44, TFT_BLUE);
  tft.drawLine(8, tft.height() - 12, tft.width() - 8, tft.height() - 12, TFT_MAGENTA);

  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.drawString("uptime (s):", 16, 170, 2);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) { }
  Serial.println(F("\nMassmore_ST7789 - 01_BasicDisplay"));
  tft.printConfig(Serial);

  tft.begin(1);                       // rotation 1 = landscape (320x240 บนจอ 2.0")

  // Chip identity (ต้องมี SDA read — ดู Config) ---------------------------------------
  Massmore_ST7789::ChipID id = tft.readChipID();
  Serial.print(F("RDID1/2/3 = 0x")); Serial.println(id.raw(), HEX);
  if (tft.verifyChipID()) Serial.println(F("Chip ID OK (ST7789V: 0x85 0x85 0x52)"));
  else { Serial.print(F("Chip ID check: ")); Serial.println(Massmore_ST7789::errorToString(tft.lastError())); }

  drawStaticScene();
}

void loop() {
  static uint32_t lastTick = 0;
  if ((uint32_t)(millis() - lastTick) >= 1000UL) {     // rollover-safe
    lastTick += 1000UL;
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextPadding(120);                            // ลบตัวเลขเก่าอัตโนมัติ
    tft.drawNumber(millis() / 1000, 16, 190, 6);        // font 6 = ตัวเลขใหญ่ 48 px
    tft.setTextPadding(0);
  }
}
