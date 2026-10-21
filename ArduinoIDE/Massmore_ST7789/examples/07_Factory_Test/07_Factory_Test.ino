/*
  07_Factory_Test — Massmore_ST7789 (Outgoing QA / QC)
  ---------------------------------------------------------------------------
  ใช้ตรวจโมดูลจอ Massmore ST7789 SKU-1021 ก่อนส่งลูกค้า และให้เว็บ Massmore Web Serial Monitor
  อ่านผลอัตโนมัติ — รันเองทันทีหลังบูต

  Test sequence
    1. DISPLAY_INIT   reset + init sequence + rotation (พิมพ์ขนาด panel)
    2. CHIP_ID        RDID1/2/3 (DAh/DBh/DCh) = 0x85 0x85 0x52 (datasheet) หรือ 0x81 0x81 0xB3 (SKU-1021)
    3. POWER_MODE     RDDPM (0Ah) = 0x9C  (booster on, sleep out, normal, display on)
    4. PIXEL_FORMAT   RDDCOLMOD (0Ch) D2–D0 = 101 (16-bit; ST7789V ตอบ 0x55, ล็อต SKU-1021 ตอบ 0x05)
    5. SELF_DIAG      RDDSDR (0Fh) bit7 register-loading + bit6 functionality = 0xC0
    6. FRAME_MEMORY   เขียน pixel 16 จุด อ่านกลับ (RAMRD) ต้องตรงทุกจุด
    7. AUTHENTICITY   isGenuine() → GENUINE / SUSPECT
    8. TOUCH_I2C      (รุ่น Touch) ACK ที่ 0x15,  TOUCH_ID = CHIP_ID (0xA7)
    9. SD_MOUNT       (รุ่น SD) mount + ขนาดการ์ด,  SD_RW เขียน/อ่านไฟล์
   10. CONTINUOUS     อ่าน ID + GRAM readback 20 รอบติดกัน ไม่มีค่าเพี้ยน
   11. VERDICT + แถบสี / ข้อความไทย / สรุปผล LCD-SD-Touch บนจอให้พนักงานตรวจด้วยตา
   12. กดปุ่ม BOOT (GPIO0) → หน้าทดสอบ Touch (ลากนิ้ววาดเส้น) · กด BOOT อีกครั้ง → กลับหน้าสรุปผล

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
  ESP32-S3: GPIO0 เป็นทั้งปุ่ม BOOT และ SD_CS — sketch จะไม่ส่งข้อมูลบน SPI ระหว่างกดปุ่มค้าง
  (CS ของ SD ถูกดึง LOW) และเปลี่ยนหน้าตอนปล่อยปุ่ม

  Designed and Manufactured by Massmore — https://www.massmore.shop
*/

#include <SPI.h>
#include <Wire.h>
#include <Massmore_ST7789.h>
#include <Massmore_ST7789_Touch.h>
#include <fonts/Massmore_Font_Kanit_28.h>
#if defined(ESP32)
  #include <SD.h>
  #include "sd_diskio.h"        /* ใช้ตอน mount ไม่ผ่าน: แยก "ไม่มีการ์ด" กับ "การ์ดไม่ใช่ FAT32" */
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
#define FT_BOOT_PIN    0           /* ปุ่ม BOOT บน ESP32 / ESP32-S3 DevKit และ MOMO ESP32-S3 */
#define FT_ROTATION    1           /* landscape — ใช้กับทั้งจอและพิกัด touch */
#define FT_SAMPLES     20
#define FT_GRAM_POINTS 16
/* ------------------------------------------------------------------------------------------ */

Massmore_ST7789       tft;
Massmore_ST7789_Touch touch;

static bool        ftFailed = false;
static const char *ftReason = nullptr;

/* ผลย่อยที่ต้องแสดงบนจอ (Serial ได้รับครบอยู่แล้วจาก #RESULT) */
static uint32_t ftChipId   = 0;
static bool     ftIdOk     = false;
static bool     ftTouchOk  = false;
static uint8_t  ftTouchId  = 0;
static bool     ftSdMount  = false;
static bool     ftSdRw     = false;
static uint32_t ftSdMB     = 0;
static const char *ftSdType = "";      /* MMC / SDSC / SDHC — ว่าง = ไม่พบการ์ด */

static bool     touchPage  = false;   /* false = หน้าสรุปผล, true = หน้าทดสอบ Touch */
static bool     bootHeld   = false;
static uint32_t bootDownMs = 0;
static uint16_t touchCount = 0;

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

#if FT_HAS_SD && defined(ESP32)
static const char *sdTypeName(sdcard_type_t t) {
  switch (t) {
    case CARD_MMC:  return "MMC";
    case CARD_SD:   return "SDSC";
    case CARD_SDHC: return "SDHC";
    default:        return "";
  }
}

/* SD.begin() ไม่ผ่าน → init การ์ดเองผ่าน diskio ของ SD library (ลำดับเดียวกับ SD.begin) เพื่อดูว่ามีการ์ดไหม
 * f_mount จะล้มอยู่แล้ว (log "no valid FAT volume") แต่การ์ดถูก initialize ไปแล้ว จึงอ่านชนิด / ความจุได้ */
static void probeSdCard() {
  uint8_t pd = sdcard_init(FT_SD_CS, &SPI, FT_SD_HZ);
  if (pd == 0xFF) return;
  sdcard_mount(pd, "/sdprobe", 1, false);
  ftSdType = sdTypeName(sdcard_type(pd));
  if (ftSdType[0]) ftSdMB = (uint32_t)((uint64_t)sdcard_num_sectors(pd) * sdcard_sector_size(pd) / (1024ULL * 1024ULL));
  sdcard_unmount(pd);
  sdcard_uninit(pd);
}
#endif

/* หนึ่งบรรทัดในตารางสรุป: ชื่อ + รายละเอียด + OK/NG */
static void drawResultRow(int32_t y, const char *name, const char *detail, bool ok) {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(name, 8, y, 2);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(detail, 64, y, 2);
  tft.setTextColor(ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.drawString(ok ? "OK" : "NG", tft.width() - 30, y, 2);
}

static void drawVerdictScreen() {
  const uint16_t bars[8] = { TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, TFT_DARKGREY };
  char line[40];
  tft.fillScreen(TFT_BLACK);
  int32_t bw = tft.width() / 8;
  for (int i = 0; i < 8; i++) tft.fillRect(i * bw, 0, bw, 24, bars[i]);
  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_WHITE);          /* ขอบจอ: ตรวจว่า panel size ถูก */
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Massmore ST7789 QA", 8, 30, 4);
  tft.loadFont(Massmore_Font_Kanit_28);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawThaiString("ภาษาไทย ที่นี่ น้ำ ปิ่น", 8, 58);           /* สั้นพอดีจอ 320 px — ไม่ตัดบรรทัด */
  tft.unloadFont();

  snprintf(line, sizeof(line), "ID 0x%06lX %dx%d", (unsigned long)ftChipId, (int)tft.width(), (int)tft.height());
  drawResultRow(100, "LCD", line, ftIdOk);
#if FT_HAS_SD && defined(ESP32)
  if (ftSdMount)        snprintf(line, sizeof(line), "%s %lu MB  R/W %s", ftSdType, (unsigned long)ftSdMB, ftSdRw ? "OK" : "ERROR");
  else if (ftSdType[0]) snprintf(line, sizeof(line), "%s %lu MB - not FAT32", ftSdType, (unsigned long)ftSdMB);
  else                  snprintf(line, sizeof(line), "no card detected");
  drawResultRow(120, "SD", line, ftSdMount && ftSdRw);
#endif
#if FT_HAS_TOUCH
  if (ftTouchOk) snprintf(line, sizeof(line), "CST816 0x15  ID 0x%02X", ftTouchId);
  else           snprintf(line, sizeof(line), "no ACK at 0x15");
  drawResultRow(140, "TOUCH", line, ftTouchOk);
#endif

  tft.setTextColor(ftFailed ? TFT_RED : TFT_GREEN, TFT_BLACK);
  tft.drawString(ftFailed ? "FAIL" : "PASS", 8, 164, 4);
  if (ftFailed) { tft.setTextColor(TFT_RED, TFT_BLACK); tft.drawString(ftReason, 90, 170, 2); }
#if FT_HAS_TOUCH
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("BOOT = Touch test", 8, tft.height() - 22, 2);
#endif
}

/* หน้าทดสอบ Touch: วาดจุดตามนิ้ว + พิกัดล่าสุด */
static void drawTouchScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Touch Test", 8, 4, 4);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("BOOT = back", tft.width() - 90, 10, 2);
  tft.drawRect(0, 32, tft.width(), tft.height() - 32, TFT_BLUE);
  if (!ftTouchOk) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("CST816 not found - check SDA/SCL", 8, 60, 2);
  }
  touchCount = 0;
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
  tft.begin(FT_ROTATION, TFT_BLACK);
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
  ftChipId = id.raw(); ftIdOk = idOk;
  ftResult("CHIP_ID", idOk); printHex(id.raw(), 6);
  if (idOk) Serial.println(id.isDatasheetID() ? F(" (ST7789V)") : F(" (ST7789 SKU-1021 lot)"));
  else      Serial.println();
  if (!idOk) {
    if (id.raw() == 0x000000UL || id.raw() == 0xFFFFFFUL) {
      Serial.println(F("  no response on SDA read - check SDA/SCL/CS/DC wiring, 3V3 power, or interface mode (SDO)"));
      ftFail("NO_RESPONSE_ON_SDA");
      ftVerdict(); return;                              /* อ่านจอไม่ได้เลย → ข้อถัดไปไม่มีความหมาย */
    }
    Serial.println(F("  ID does not match 0x858552 (ST7789V) or 0x8181B3 (SKU-1021)"));
    ftFail("CHIP_ID_MISMATCH");                         /* ทดสอบต่อเพื่อให้เห็นผล SD / Touch ด้วย */
  }

  /* 3-5. POWER_MODE / PIXEL_FORMAT / SELF_DIAG --------------------------- */
  Massmore_ST7789::Status st = tft.readStatus();
  bool pmOk  = (st.powerMode & MASSMORE_ST7789_RDDPM_MASK) == MASSMORE_ST7789_RDDPM_READY;
  ftResult("POWER_MODE", pmOk); printHex(st.powerMode, 2); Serial.println();
  if (!pmOk) ftFail("POWER_MODE_NOT_9C");

  bool pfOk = (st.pixelFormat & MASSMORE_ST7789_COLMOD_MASK) == (MASSMORE_ST7789_COLMOD_16BIT & MASSMORE_ST7789_COLMOD_MASK);
  ftResult("PIXEL_FORMAT", pfOk); printHex(st.pixelFormat, 2); Serial.println();
  if (!pfOk) ftFail("PIXEL_FORMAT_NOT_16BIT");

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
  touch.setPanelSize(Massmore_ST7789::panelWidth(), Massmore_ST7789::panelHeight());
  ftTouchOk = tpOk; ftTouchId = 0;
  ftResult("TOUCH_I2C", tpOk); Serial.println(tpOk ? F("0x15") : F("NO_ACK"));
  if (!tpOk) ftFail("TOUCH_NO_ACK_0x15");
  else {
    uint8_t tid = touch.readChipID();
    ftTouchId = tid;
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
  ftSdMount = sdMounted; ftSdRw = false; ftSdMB = 0; ftSdType = "";
  if (sdMounted) { ftSdType = sdTypeName(SD.cardType()); ftSdMB = (uint32_t)(SD.cardSize() / (1024ULL * 1024ULL)); }
  else           probeSdCard();
  ftResult("SD_MOUNT", sdMounted);
  if (sdMounted) { Serial.print(ftSdMB); Serial.print(F("MB ")); Serial.println(ftSdType); }
  else if (ftSdType[0]) {
    Serial.print(F("NOT_FAT32 ")); Serial.print(ftSdMB); Serial.print(F("MB ")); Serial.println(ftSdType);
    Serial.println(F("  card answers but has no FAT volume - format it as FAT32 (MBR)"));
    ftFail("SD_NOT_FAT32");
  }
  else { Serial.println(F("NO_CARD")); ftFail("SD_MOUNT_FAILED"); }
  if (sdMounted) {
    bool rw = false;
    File f = SD.open("/massmore_qa.txt", FILE_WRITE);
    if (f) { f.print("QA"); f.close(); f = SD.open("/massmore_qa.txt"); if (f) { rw = (f.read() == 'Q' && f.read() == 'A'); f.close(); } }
    SD.remove("/massmore_qa.txt");
    ftSdRw = rw;
    ftResult("SD_RW", rw); Serial.println(rw ? F("OK") : F("IO_ERROR"));
    if (!rw) ftFail("SD_READ_WRITE_FAILED");
  }
  SD.end();
#endif
  pinMode(FT_BOOT_PIN, INPUT_PULLUP);                   /* S3: SD_CS = GPIO0 → คืนให้ปุ่ม BOOT (ค้าง HIGH ด้วย pull-up) */

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

/* ปุ่ม BOOT: ระหว่างกดค้างไม่แตะบัส SPI (S3: CS ของ SD ถูกดึง LOW), สลับหน้าตอนปล่อย */
static bool bootReleased() {
  bool down = digitalRead(FT_BOOT_PIN) == LOW;
  if (down && !bootHeld) { bootHeld = true; bootDownMs = millis(); }
  if (!down && bootHeld) {
    bootHeld = false;
    return (uint32_t)(millis() - bootDownMs) >= 30UL;  /* debounce */
  }
  return false;
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
    if (c == 'r' || c == 'R') { touchPage = false; runFactoryTest(); }
  }

  if (bootReleased()) {
    touchPage = !touchPage;
    Serial.println(touchPage ? F("BOOT -> touch test page") : F("BOOT -> result page"));
    if (touchPage) drawTouchScreen(); else drawVerdictScreen();
  }
  if (bootHeld) return;                                 /* ห้ามใช้ SPI ขณะกดปุ่ม */

#if FT_HAS_TOUCH
  if (touchPage && ftTouchOk) {
    touch.update();                                     /* FSM: poll ทุก 20 ms (ไม่ต่อ INT) */
    if (touch.isDataReady()) {
      Massmore_ST7789_Touch::Point p;
      touch.getReadings(p, FT_ROTATION);
      if (p.y > 34 && p.y < tft.height() - 2 && p.x > 1 && p.x < tft.width() - 2) tft.fillCircle(p.x, p.y, 3, TFT_GREEN);
      char xy[24];
      snprintf(xy, sizeof(xy), "x=%3u y=%3u", p.x, p.y);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.drawString(xy, 130, 10, 2);
      touchCount++;
    }
  }
#endif
}
