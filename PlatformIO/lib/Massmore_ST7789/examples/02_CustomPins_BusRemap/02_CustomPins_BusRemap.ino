/*
  02_CustomPins_BusRemap — Massmore_ST7789
  ---------------------------------------------------------------------------
  ST7789 ถูกขับด้วย TFT_eSPI ซึ่งใช้ "ขาแบบ compile-time" (direct register write + DMA จึงเร็วมาก)
  ไลบรารีนี้จึงไม่รับขาเข้าทาง constructor เหมือนไลบรารี Massmore ตัวอื่น แต่รวมการตั้งค่าไว้ที่
  ไฟล์เดียว: src/Massmore_ST7789_Config.h ซึ่งทุกค่า override ได้โดยไม่แก้ไลบรารี:

    PlatformIO  (platformio.ini)
      build_flags = ${env.build_flags} -DTFT_MOSI=13 -DTFT_SCLK=14 -DMASSMORE_ST7789_MISO=12 -DTFT_CS=15 -DTFT_DC=27 -DTFT_RST=26
                    -DMASSMORE_ST7789_PANEL=240        ; จอ 1.3" / 1.54"
                    -DTFT_BL=21                         ; ถ้าต่อ BLK กับ GPIO เพื่อคุมความสว่าง
                    -DSPI_FREQUENCY=80000000            ; สายสั้นบน IOMUX pin
    Arduino IDE
      แก้ค่า default ใน Documents/Arduino/libraries/Massmore_ST7789/src/Massmore_ST7789_Config.h

  สิ่งที่ sketch นี้สาธิต
    1. sketch เป็นเจ้าของ SPI bus: เรียก SPI.begin(SCLK, MISO, MOSI) เอง (แนะนำเมื่อใช้ร่วมกับ SD card)
       ไลบรารีตรวจพบว่า bus เริ่มแล้วจะไม่ start ซ้ำ
    2. printConfig() พิมพ์ขาที่ compile มาจริง ๆ — ใช้ยืนยันว่าสายตรงกับโค้ด
    3. ESP32 Classic / ESP32-S3 มี GPIO matrix: ใช้ขาใดก็ได้ (ยกเว้น strapping / flash pins)

  Designed and Manufactured by Massmore — https://www.massmore.shop
*/

#include <SPI.h>
#include <Massmore_ST7789.h>

Massmore_ST7789 tft;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) { }
  Serial.println(F("\nMassmore_ST7789 - 02_CustomPins_BusRemap"));

  // 1) sketch start SPI bus เองด้วยขาจาก Config (หรือใส่เลขขาตรง ๆ ก็ได้ แต่ต้องตรงกับ TFT_* ที่ compile)
  SPI.begin(TFT_SCLK, MASSMORE_ST7789_MISO, TFT_MOSI, -1);
  Serial.print(F("SPI.begin(SCLK=")); Serial.print(TFT_SCLK);
  Serial.print(F(", MISO="));          Serial.print(MASSMORE_ST7789_MISO);
  Serial.print(F(", MOSI="));          Serial.print(TFT_MOSI); Serial.println(')');

  // 2) ค่าที่ compile มาจริง
  tft.printConfig(Serial);

  tft.begin(0);                                   // portrait
  Serial.print(F("Chip ID: ")); Serial.println(tft.verifyChipID() ? "OK" : Massmore_ST7789::errorToString(tft.lastError()));

  // แสดงขาบนจอ เพื่อเทียบกับสายที่ต่อ
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(6, 8);
  tft.setTextFont(2);
  tft.println("Massmore_ST7789 pin map");
  tft.println();
  tft.print("MOSI/SDA : "); tft.println(TFT_MOSI);
  tft.print("SCLK/SCL : "); tft.println(TFT_SCLK);
  tft.print("MISO(SD) : "); tft.println(MASSMORE_ST7789_MISO);
  tft.print("CS       : "); tft.println(TFT_CS);
  tft.print("DC       : "); tft.println(TFT_DC);
  tft.print("RST      : "); tft.println(TFT_RST);
  tft.print("BL       : "); tft.println(MASSMORE_ST7789_BL);
  tft.print("Panel    : "); tft.print(TFT_WIDTH); tft.print("x"); tft.println(TFT_HEIGHT);
  tft.print("SPI      : "); tft.print(SPI_FREQUENCY / 1000000); tft.println(" MHz");
  tft.println();
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.println("Override with -D flags");
  tft.println("or edit Massmore_ST7789_Config.h");

  // กรอบขอบจอ: ถ้ากรอบไม่ชิดขอบ/ขาด แปลว่า MASSMORE_ST7789_PANEL ไม่ตรงกับจอ
  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_YELLOW);
}

void loop() {
  delay(1000);
}
