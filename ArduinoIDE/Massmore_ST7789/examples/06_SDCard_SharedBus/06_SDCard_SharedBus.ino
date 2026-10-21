/*
  06_SDCard_SharedBus — Massmore_ST7789 (รุ่น 2.0" ที่มีช่อง Micro SD)
  ---------------------------------------------------------------------------
  จอและ SD card ใช้ SPI bus เส้นเดียวกัน (MOSI / MISO / SCLK ร่วม, แยกกันด้วย CS) — sketch นี้แสดง
  ลำดับที่ถูกต้อง: SPI.begin() โดย sketch → tft.begin() → SD.begin(SD_CS, SPI)
  แล้ว list ไฟล์ใน root, เขียน/อ่านไฟล์ทดสอบ และแสดงผลบนจอ

  SD library มากับ Arduino-ESP32 Core อยู่แล้ว (ไม่ต้องติดตั้ง) — ตัวอย่างนี้จึงใช้ได้เฉพาะ ESP32 family

  Wiring (SD) — MOSI / MISO / SCLK ต่อขาเดียวกับจอ (ดู Config), SD_CS ตามด้านล่าง
    ESP32 (Classic): SD_CS 5
    ESP32-S3       : SD_CS 0   (ตามเอกสารสินค้า — GPIO0 เป็น strapping pin: ถ้าบอร์ด boot ไม่ขึ้นให้ย้ายขา)
  ฟอร์แมต SD เป็น FAT32

  Designed and Manufactured by Massmore — https://www.massmore.shop
*/

#if !defined(ESP32)
  #error "06_SDCard_SharedBus uses the ESP32 core SD library - build for ESP32 / ESP32-S3"
#endif

#include <SPI.h>
#include <SD.h>
#include <Massmore_ST7789.h>

#if defined(CONFIG_IDF_TARGET_ESP32S3)
  #define SD_CS 0
#else
  #define SD_CS 5
#endif
#define SD_SPI_HZ 20000000UL     // SD card บน jumper wire: 20 MHz ปลอดภัย (ลดเป็น 4 MHz ถ้า mount ไม่ขึ้น)

Massmore_ST7789 tft;
int16_t line = 0;

static void say(const char *s, uint16_t colour = TFT_WHITE) {
  Serial.println(s);
  tft.setTextColor(colour, TFT_BLACK);
  tft.drawString(s, 4, 4 + line * 16, 2);
  line++;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) { }
  Serial.println(F("\nMassmore_ST7789 - 06_SDCard_SharedBus"));

  // 1) sketch เป็นเจ้าของ bus — start ด้วยขา MISO จริง (จำเป็นสำหรับ SD)
  pinMode(SD_CS, OUTPUT); digitalWrite(SD_CS, HIGH);     // กัน SD ไปกวน bus ระหว่าง init จอ
  SPI.begin(TFT_SCLK, MASSMORE_ST7789_MISO, TFT_MOSI, -1);

  // 2) จอ
  tft.begin(1);
  say("SD card on shared SPI bus", TFT_CYAN);

  // 3) SD (ใช้ object SPI ตัวเดียวกัน — ไลบรารีจอใช้ transaction lock เดียวกัน)
  if (!SD.begin(SD_CS, SPI, SD_SPI_HZ)) {
    say("SD mount FAILED", TFT_RED);
    say("check FAT32 / 3.3V / CS pin", TFT_RED);
    return;
  }
  char buf[48];
  uint8_t t = SD.cardType();
  snprintf(buf, sizeof(buf), "Card: %s  %llu MB", t == CARD_SD ? "SDSC" : t == CARD_SDHC ? "SDHC" : t == CARD_MMC ? "MMC" : "?",
           SD.cardSize() / (1024ULL * 1024ULL));
  say(buf, TFT_GREEN);

  // เขียน / อ่านไฟล์ทดสอบ
  File f = SD.open("/massmore_test.txt", FILE_WRITE);
  if (f) { f.println("Massmore ST7789 SD test OK"); f.close(); say("write /massmore_test.txt OK"); }
  else say("write FAILED", TFT_RED);
  f = SD.open("/massmore_test.txt");
  if (f) {
    String s = f.readStringUntil('\n'); f.close();
    snprintf(buf, sizeof(buf), "read: %s", s.c_str());
    say(buf);
  }

  // list root
  say("Root directory:", TFT_YELLOW);
  File root = SD.open("/");
  uint8_t n = 0;
  while (File e = root.openNextFile()) {
    if (n++ < 8) {
      snprintf(buf, sizeof(buf), " %c %-20.20s %lu", e.isDirectory() ? 'D' : 'F', e.name(), (unsigned long)e.size());
      say(buf);
    }
    e.close();
  }
  root.close();

  // จอยังวาดได้ตามปกติหลังใช้ SD (bus แชร์กันได้)
  tft.fillCircle(tft.width() - 24, tft.height() - 24, 12, TFT_GREEN);
}

void loop() {
  delay(1000);
}
