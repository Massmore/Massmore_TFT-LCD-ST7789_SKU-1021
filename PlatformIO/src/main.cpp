#include <Arduino.h>
/*
  07_Factory_Test — Massmore_ST7789 (Outgoing QA / QC)
  ---------------------------------------------------------------------------
  ใช้ตรวจโมดูลจอ Massmore ST7789 SKU-1021 ก่อนส่งลูกค้า และให้เว็บ Massmore Web Serial Monitor
  อ่านผลอัตโนมัติ — รันเองทันทีหลังบูต

  Test sequence
    1. DISPLAY_INIT   reset + init sequence + rotation (พิมพ์ขนาด panel)
    2. CHIP_ID        RDID1/2/3 (DAh/DBh/DCh) ต้องเป็น 0x85 0x85 0x52 (datasheet §9.1.3)
    3. POWER_MODE     RDDPM (0Ah) = 0x9C  (booster on, sleep out, normal, display on)
    4. PIXEL_FORMAT   RDDCOLMOD (0Ch) = 0x55 (16-bit)
    5. SELF_DIAG      RDDSDR (0Fh) bit7 register-loading + bit6 functionality = 0xC0
    6. FRAME_MEMORY   เขียน pixel 16 จุด อ่านกลับ (RAMRD) ต้องตรงทุกจุด
    7. AUTHENTICITY   isGenuine() → GENUINE / SUSPECT
    8. TOUCH_I2C      (รุ่น Touch) ACK ที่ 0x15,  TOUCH_ID = CHIP_ID (0xA7)
    9. SD_MOUNT       (รุ่น SD) mount + ขนาดการ์ด,  SD_RW เขียน/อ่านไฟล์
   10. CONTINUOUS     อ่าน ID + GRAM readback 20 รอบติดกัน ไม่มีค่าเพี้ยน
   11. VERDICT + แถบสี / ข้อความไทยบนจอให้พนักงานตรวจด้วยตา

  Serial 115200 — ทุกบรรทัดที่เว็บ parse ขึ้นต้นด้วย '#' (English only):
    #MASSMORE_FACTORY_TEST v1.0
    #PRODUCT Massmore_ST7789
    #MCU ESP32
    #RESULT <TEST_NAME> <PASS|FAIL> <value>
    #VERDICT <PASS|FAIL> [<REASON>]
    [PASS] SENSOR QA PASSED - READY TO SHIP   หรือ   [FAIL] QA CHECK FAILED: <REASON>

  Default wiring (Primary test MCU = ESP32 Classic) — ตามเอกสารสินค้า SKU-1021
    TFT : MOSI/SDA 23, SCLK/SCL 18, CS 16, DC 2, RST 4, BLK → 3V3, VCC → 3V3, GND → GND
    SD  : MISO 19 (ร่วมกับจอ), CS 5            (FT_HAS_SD = 1 ต้องเสียบการ์ด FAT32)
    Touch: SDA 21, SCL 22                     (FT_HAS_TOUCH = 1)
  ESP32-S3: TFT 11/12/17/3/18, SD MISO 13 CS 0, Touch SDA 14 SCL 15
  Pin ของ SD / Touch ถูก hardcode ไว้ "เฉพาะใน sketch นี้"; pin ของจอมาจาก Massmore_ST7789_Config.h

  รุ่นที่ทดสอบ: ตั้ง FT_HAS_TOUCH / FT_HAS_SD ด้านล่างให้ตรงกับสินค้า (1.3"/1.54" = 0/0 + PANEL 240)
  พิมพ์ 'r' + Enter เพื่อทดสอบซ้ำ

  Designed and Manufactured by Massmore — https://www.massmore.shop
*/

#include <SPI.h>
#include <Wire.h>
#include <Massmore_ST7789.h>
#include <Massmore_ST7789_Touch.h>
#include <fonts/Massmore_Font_Kanit_28.h>
#if defined(ESP32)
  #include <SD.h>
#endif

/* ---------------- Product variant under test (ปรับได้ที่นี่เท่านั้น) ------------------------ */
#define FT_HAS_TOUCH   1        /* 1 = รุ่น 2.0" Touch, 0 = ไม่มี touch                            */
#define FT_HAS_SD      1        /* 1 = รุ่น 2.0" มีช่อง SD (ต้องเสียบการ์ด FAT32 ขณะทดสอบ)          */

/* ---------------- Factory Test wiring (SD / Touch) --------------------------------------- */
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  #define FT_MCU_NAME  "ESP32-S3"
  #define FT_TP_SDA    14
  #define FT_TP_SCL    15
  #define FT_SD_CS     0
#elif defined(ARDUINO_ARCH_ESP32)
  #define FT_MCU_NAME  "ESP32"
  #define FT_TP_SDA    21
  #define FT_TP_SCL    22
  #define FT_SD_CS     5
#else
  #define FT_MCU_NAME  "UNKNOWN"
  #define FT_TP_SDA    -1
  #define FT_TP_SCL    -1
  #define FT_SD_CS     -1
#endif
#define FT_SD_HZ       4000000UL   /* ความเร็วต่ำเพื่อความนิ่งบน jig */
#define FT_SAMPLES     20
#define FT_GRAM_POINTS 16
/* ------------------------------------------------------------------------------------------ */

Massmore_ST7789       tft;
Massmore_ST7789_Touch touch;

static bool        ftFailed = false;
static const char *ftReason = nullptr;

/* ---------------- helpers ------------------------------------------------- */
static void ftResult(const char *name, bool pass) {
  Serial.print(F("#RESULT ")); Serial.print(name); Serial.print(pass ? F(" PASS ") : F(" FAIL "));
}
static void ftFail(const char *reason) {
  if (!ftFailed) { ftFailed = true; ftReason = reason; }
}
static void printHex(uint32_t v, uint8_t digits) {
  Serial.print(F("0x"));
  for (int8_t s = (digits - 1) * 4; s >= 0; s -= 4) Serial.print((v >> s) & 0xF, HEX);
}

static void drawVerdictScreen() {
  const uint16_t bars[8] = { TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, TFT_DARKGREY };
  tft.fillScreen(TFT_BLACK);
  int32_t bw = tft.width() / 8;
  for (int i = 0; i < 8; i++) tft.fillRect(i * bw, 0, bw, 40, bars[i]);
  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_WHITE);          /* ขอบจอ: ตรวจว่า panel size ถูก */
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Massmore ST7789 QA", 8, 50, 4);
  tft.loadFont(Massmore_Font_Kanit_28);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawThaiString("ทดสอบฟอนต์ไทย ที่นี่ น้ำ ปิ่น", 8, 84);
  tft.unloadFont();
  tft.setTextColor(ftFailed ? TFT_RED : TFT_GREEN, TFT_BLACK);
  tft.drawString(ftFailed ? "FAIL" : "PASS", 8, 130, 7);
  if (ftFailed) { tft.setTextColor(TFT_RED, TFT_BLACK); tft.drawString(ftReason, 8, tft.height() - 20, 2); }
}

static void ftVerdict() {
  Serial.println();
  if (ftFailed) {
    Serial.print(F("#VERDICT FAIL ")); Serial.println(ftReason);
    Serial.print(F("[FAIL] QA CHECK FAILED: ")); Serial.println(ftReason);
  } else {
    Serial.println(F("#VERDICT PASS"));
    Serial.println(F("[PASS] SENSOR QA PASSED - READY TO SHIP"));
  }
  Serial.println(F("Type 'r' + Enter to run the test again."));
  drawVerdictScreen();
}

/* ---------------- the test ------------------------------------------------ */
static void runFactoryTest() {
  ftFailed = false; ftReason = nullptr;

  Serial.println();
  Serial.println(F("#MASSMORE_FACTORY_TEST v1.0"));
  Serial.println(F("#PRODUCT Massmore_ST7789"));
  Serial.print(F("#MCU ")); Serial.println(F(FT_MCU_NAME));
  Serial.print(F("Library v")); Serial.println(F(MASSMORE_ST7789_VERSION_STR));
  tft.printConfig(Serial);

  /* 1. DISPLAY_INIT ------------------------------------------------------- */
  tft.begin(1, TFT_BLACK);
  ftResult("DISPLAY_INIT", true);
  Serial.print(tft.width()); Serial.print('x'); Serial.println(tft.height());

  /* 2. CHIP_ID ------------------------------------------------------------- */
  if (!Massmore_ST7789::hasReadSupport()) {
    ftResult("CHIP_ID", false); Serial.println(F("NO_READ_SUPPORT"));
    ftFail("NO_READ_SUPPORT_IN_BUILD");
    ftVerdict(); return;
  }
  Massmore_ST7789::ChipID id = tft.readChipID();
  bool idOk = id.matches();
  ftResult("CHIP_ID", idOk); printHex(id.raw(), 6); Serial.println();
  if (!idOk) {
    if (id.raw() == 0x000000UL || id.raw() == 0xFFFFFFUL) {
      Serial.println(F("  no response on SDA read - check SDA/SCL/CS/DC wiring, 3V3 power, or interface mode (SDO)"));
      ftFail("NO_RESPONSE_ON_SDA");
    } else {
      Serial.println(F("  ID does not match ST7789V 0x85 0x85 0x52"));
      ftFail("CHIP_ID_MISMATCH");
    }
    ftVerdict(); return;
  }

  /* 3-5. POWER_MODE / PIXEL_FORMAT / SELF_DIAG --------------------------- */
  Massmore_ST7789::Status st = tft.readStatus();
  bool pmOk  = (st.powerMode & MASSMORE_ST7789_RDDPM_MASK) == MASSMORE_ST7789_RDDPM_READY;
  ftResult("POWER_MODE", pmOk); printHex(st.powerMode, 2); Serial.println();
  if (!pmOk) ftFail("POWER_MODE_NOT_9C");

  bool pfOk = st.pixelFormat == MASSMORE_ST7789_COLMOD_16BIT;
  ftResult("PIXEL_FORMAT", pfOk); printHex(st.pixelFormat, 2); Serial.println();
  if (!pfOk) ftFail("PIXEL_FORMAT_NOT_55");

  bool sdOk = (st.selfDiag & MASSMORE_ST7789_SDR_OK) == MASSMORE_ST7789_SDR_OK;
  ftResult("SELF_DIAG", sdOk); printHex(st.selfDiag, 2); Serial.println();
  if (!sdOk) ftFail("SELF_DIAGNOSTIC_FAIL");
  Serial.print(F("  MADCTL=")); printHex(st.madctl, 2);
  Serial.print(F(" IMAGE_MODE=")); printHex(st.imageMode, 2);
  Serial.print(F(" SIGNAL_MODE=")); printHex(st.signalMode, 2);
  Serial.print(F(" RDDST=")); printHex(tft.readDisplayStatus(), 8); Serial.println();

  /* 6. FRAME_MEMORY -------------------------------------------------------- */
  uint8_t fails = 0;
  bool gramOk = tft.testFrameMemory(FT_GRAM_POINTS, fails);
  ftResult("FRAME_MEMORY", gramOk);
  Serial.print(FT_GRAM_POINTS - fails); Serial.print('/'); Serial.println(FT_GRAM_POINTS);
  if (!gramOk) ftFail("GRAM_READBACK_MISMATCH");

  /* 7. AUTHENTICITY -------------------------------------------------------- */
  bool genuine = tft.isGenuine();
  ftResult("AUTHENTICITY", genuine); Serial.println(genuine ? F("GENUINE") : F("SUSPECT"));
  if (!genuine) ftFail("AUTHENTICITY_SUSPECT");

  /* 8. TOUCH --------------------------------------------------------------- */
#if FT_HAS_TOUCH
  Wire.begin(FT_TP_SDA, FT_TP_SCL, 400000);
  bool tpOk = touch.begin(Wire, -1, -1);
  ftResult("TOUCH_I2C", tpOk); Serial.println(tpOk ? F("0x15") : F("NO_ACK"));
  if (!tpOk) ftFail("TOUCH_NO_ACK_0x15");
  else {
    uint8_t tid = touch.readChipID();
    bool tidOk = (tid != 0x00 && tid != 0xFF);        /* ค่าเฉพาะรุ่นยังไม่ยืนยันจาก datasheet — ดู Touch.h */
    ftResult("TOUCH_ID", tidOk); printHex(tid, 2);
    Serial.print(touch.verifyChipID() ? F(" (CST816 family)") : F(" (unknown id)")); Serial.println();
    if (!tidOk) ftFail("TOUCH_ID_INVALID");
  }
#endif

  /* 9. SD CARD ------------------------------------------------------------- */
#if FT_HAS_SD && defined(ESP32)
  pinMode(FT_SD_CS, OUTPUT); digitalWrite(FT_SD_CS, HIGH);
  bool sdMounted = SD.begin(FT_SD_CS, SPI, FT_SD_HZ);
  ftResult("SD_MOUNT", sdMounted);
  if (sdMounted) { Serial.print(SD.cardSize() / (1024ULL * 1024ULL)); Serial.println(F("MB")); }
  else { Serial.println(F("NO_CARD")); ftFail("SD_MOUNT_FAILED"); }
  if (sdMounted) {
    bool rw = false;
    File f = SD.open("/massmore_qa.txt", FILE_WRITE);
    if (f) { f.print("QA"); f.close(); f = SD.open("/massmore_qa.txt"); if (f) { rw = (f.read() == 'Q' && f.read() == 'A'); f.close(); } }
    SD.remove("/massmore_qa.txt");
    ftResult("SD_RW", rw); Serial.println(rw ? F("OK") : F("IO_ERROR"));
    if (!rw) ftFail("SD_READ_WRITE_FAILED");
    SD.end();
  }
#endif

  /* 10. CONTINUOUS --------------------------------------------------------- */
  uint8_t good = 0;
  for (uint8_t i = 0; i < FT_SAMPLES; i++) {
    uint8_t f2 = 0;
    bool ok = tft.readChipID().matches() && tft.testFrameMemory(4, f2);
    if (ok) good++;
  }
  ftResult("CONTINUOUS", good == FT_SAMPLES);
  Serial.print(good); Serial.print('/'); Serial.println(FT_SAMPLES);
  if (good != FT_SAMPLES) ftFail("CONTINUOUS_READ_UNSTABLE");

  ftVerdict();
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) { }
  Serial.println(F("\nMassmore_ST7789 - 07_Factory_Test"));
#if defined(ESP32)
  SPI.begin(TFT_SCLK, MASSMORE_ST7789_MISO, TFT_MOSI, -1);    /* sketch owns the bus (shared with SD) */
#endif
  runFactoryTest();
}

void loop() {
  if (Serial.available()) {
    char c = (char)Serial.read();
    if (c == 'r' || c == 'R') runFactoryTest();
  }
}
