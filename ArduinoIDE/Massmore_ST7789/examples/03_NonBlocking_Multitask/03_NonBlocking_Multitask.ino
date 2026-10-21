/*
  03_NonBlocking_Multitask — Massmore_ST7789
  ---------------------------------------------------------------------------
  Advanced Non-blocking API: render ลง TFT_eSprite (RAM) แล้วส่งขึ้นจอด้วย DMA (pushImageDMA)
  ระหว่างที่ DMA กำลังส่ง CPU ว่าง → loop() ไปทำงานอื่นต่อได้ (กระพริบ LED, นับรอบ, อ่านเซ็นเซอร์ ฯลฯ)
  ไม่มี delay() ใน loop เลย — ทุกอย่างใช้ millis() แบบ rollover-safe

    - แถบแอนิเมชัน (sprite 240 x 100) ลูกบอลเด้ง 6 ลูก อัปเดตทุก 16 ms (~60 fps)
    - LED กระพริบทุก 500 ms
    - ทุก 1 s พิมพ์ loop/s และ fps ออก Serial + บนจอ

  หมายเหตุ: DMA ของ TFT_eSPI บน ESP32 ใช้ SPI bus แบบ exclusive — อย่าใช้ร่วมกับ SD card บน bus เดียวกัน
  ในขณะที่ DMA ทำงาน (ดูตัวอย่าง 06 สำหรับ SD)

  Designed and Manufactured by Massmore — https://www.massmore.shop
*/

#include <Massmore_ST7789.h>

#if defined(LED_BUILTIN)
  #define LED_PIN LED_BUILTIN
#else
  #define LED_PIN 2
#endif

#define BAND_W   240
#define BAND_H   100
#define N_BALLS  6

Massmore_ST7789 tft;
TFT_eSprite band = TFT_eSprite(&tft);
uint16_t *bandBuf;                        // pointer ไปยัง buffer ของ sprite (ใช้กับ DMA)

struct Ball { int16_t x, y, dx, dy, r; uint16_t c; } balls[N_BALLS];

uint32_t tFrame = 0, tLed = 0, tStat = 0;
uint32_t loops = 0, frames = 0;
bool ledOn = false;

static void renderBand() {
  band.fillSprite(TFT_NAVY);
  for (int i = 0; i < N_BALLS; i++) {
    Ball &b = balls[i];
    b.x += b.dx; b.y += b.dy;
    if (b.x - b.r <= 0 || b.x + b.r >= BAND_W - 1) b.dx = -b.dx;
    if (b.y - b.r <= 0 || b.y + b.r >= BAND_H - 1) b.dy = -b.dy;
    band.fillCircle(b.x, b.y, b.r, b.c);
  }
  band.setTextColor(TFT_WHITE);
  band.drawString("DMA sprite", 4, 4, 2);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) { }
  Serial.println(F("\nMassmore_ST7789 - 03_NonBlocking_Multitask"));

  pinMode(LED_PIN, OUTPUT);
  tft.begin(0);
  tft.initDMA();                                      // เปิด DMA engine (ต้องเรียกหลัง begin)

  bandBuf = (uint16_t *)band.createSprite(BAND_W, BAND_H);   // RAM 240*100*2 = 48 KB
  if (!bandBuf) { Serial.println(F("createSprite failed (RAM)")); while (true) delay(1000); }

  const uint16_t cols[N_BALLS] = { TFT_RED, TFT_GREEN, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, TFT_ORANGE };
  for (int i = 0; i < N_BALLS; i++) {
    balls[i].r = 8 + i * 2;
    balls[i].x = 20 + i * 35; balls[i].y = 20 + i * 10;
    balls[i].dx = 2 + (i & 1); balls[i].dy = 1 + (i % 3);
    balls[i].c = cols[i];
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("loop() is never blocked", 8, 120, 2);
  tft.drawString("loops/s :", 8, 150, 2);
  tft.drawString("fps     :", 8, 170, 2);
  tFrame = tLed = tStat = millis();
}

void loop() {
  loops++;
  uint32_t now = millis();

  // ---- งาน 1: แอนิเมชัน 60 fps ผ่าน DMA -------------------------------------------
  if ((uint32_t)(now - tFrame) >= 16UL) {
    tFrame = now;
    if (!tft.dmaBusy()) {                              // frame ก่อนส่งเสร็จแล้วค่อยวาดใหม่
      renderBand();
      tft.startWrite();
      tft.pushImageDMA(0, 0, BAND_W, BAND_H, bandBuf); // คืนทันที — DMA ส่งเอง
      tft.endWrite();
      frames++;
    }
  }

  // ---- งาน 2: LED กระพริบ --------------------------------------------------------
  if ((uint32_t)(now - tLed) >= 500UL) {
    tLed = now;
    ledOn = !ledOn;
    digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
  }

  // ---- งาน 3: สถิติทุก 1 s --------------------------------------------------------
  if ((uint32_t)(now - tStat) >= 1000UL) {
    tStat = now;
    Serial.print(F("loops/s=")); Serial.print(loops); Serial.print(F("  fps=")); Serial.println(frames);
    tft.dmaWait();                                     // รอ DMA ก่อนวาดนอก sprite
    tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
    tft.setTextPadding(80);
    tft.drawNumber(loops, 80, 150, 2);
    tft.drawNumber(frames, 80, 170, 2);
    tft.setTextPadding(0);
    loops = 0; frames = 0;
  }
}
