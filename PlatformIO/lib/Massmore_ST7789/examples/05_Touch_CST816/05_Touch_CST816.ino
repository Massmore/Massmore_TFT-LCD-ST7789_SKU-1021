/*
  05_Touch_CST816 — Massmore_ST7789 (รุ่น 2.0" Touch)
  ---------------------------------------------------------------------------
  อ่าน capacitive touch CST816 ผ่าน I2C (address 0x15) แล้ววาดจุดที่สัมผัสลงจอ + แสดงพิกัด / gesture
  ใช้ Non-blocking FSM: touch.update() ใน loop() → isDataReady() → getReadings()

  Wiring (touch) — ไลบรารีไม่ hardcode ขา sketch เป็นผู้กำหนด
    ESP32 (Classic): SDA 21, SCL 22   (INT / RST ไม่ต่อ)
    ESP32-S3       : SDA 14, SCL 15   (ตามเอกสารสินค้า SKU-1021)
  จอต่อตามค่า default ใน Massmore_ST7789_Config.h (ดูตัวอย่าง 01)

  ถ้าจุดที่วาดกลับด้าน/สลับแกน ให้ปรับ touch.setInvert(x, y) / touch.setSwapXY(true) ด้านล่าง

  Designed and Manufactured by Massmore — https://www.massmore.shop
*/

#include <Wire.h>
#include <Massmore_ST7789.h>
#include <Massmore_ST7789_Touch.h>

#if defined(CONFIG_IDF_TARGET_ESP32S3)
  #define TP_SDA 14
  #define TP_SCL 15
#else
  #define TP_SDA 21
  #define TP_SCL 22
#endif
#define TP_INT  -1      // -1 = ไม่ต่อ (FSM จะ poll ทุก 20 ms) — ต่อแล้วใส่เลขขา จะอ่าน bus เฉพาะตอนสัมผัส
#define TP_RST  -1
#define ROTATION 1      // landscape

Massmore_ST7789        tft;
Massmore_ST7789_Touch  touch;

uint32_t tStatus = 0;
uint16_t points = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) { }
  Serial.println(F("\nMassmore_ST7789 - 05_Touch_CST816"));

  tft.begin(ROTATION);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Touch test - draw here", 8, 4, 2);
  tft.drawRect(0, 24, tft.width(), tft.height() - 24, TFT_DARKGREY);

  Wire.begin(TP_SDA, TP_SCL, 400000);                       // Core 3.x API — sketch เป็นเจ้าของ bus
  if (!touch.begin(Wire, TP_INT, TP_RST)) {
    Serial.print(F("CST816 not found: ")); Serial.println(Massmore_ST7789_Touch::errorToString(touch.lastError()));
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("CST816 not found (SDA/SCL?)", 8, 40, 2);
    while (true) delay(1000);
  }
  touch.setPanelSize(Massmore_ST7789::panelWidth(), Massmore_ST7789::panelHeight());
  // touch.setInvert(false, false);   // ปรับถ้าจุดกลับด้าน
  // touch.setSwapXY(false);          // ปรับถ้าแกนสลับ

  Serial.print(F("CST816 chip ID 0x")); Serial.print(touch.getChipID(), HEX);
  Serial.print(F(", firmware 0x"));     Serial.println(touch.readFirmwareVersion(), HEX);
  Serial.println(touch.verifyChipID() ? F("Chip ID recognised") : F("Chip ID not in known list (still usable)"));
}

void loop() {
  // ---- FSM: อ่าน touch เฉพาะเมื่อมีข้อมูล ----------------------------------------------
  touch.update();
  if (touch.isDataReady()) {
    Massmore_ST7789_Touch::Point p;
    touch.getReadings(p, ROTATION);
    if (p.y > 24) tft.fillCircle(p.x, p.y, 3, TFT_CYAN);
    points++;
    Serial.print(F("touch x=")); Serial.print(p.x); Serial.print(F(" y=")); Serial.println(p.y);
  }

  // ---- งานอื่นทำต่อได้ (ไม่ถูก block) -----------------------------------------------
  if ((uint32_t)(millis() - tStatus) >= 250UL) {
    tStatus = millis();
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextPadding(100);
    tft.drawNumber(points, tft.width() - 100, 4, 2);
    tft.setTextPadding(0);
  }
}
