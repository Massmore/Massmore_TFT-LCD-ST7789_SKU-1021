#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// --- Color Definitions ---
#define MASSMORE_BLUE   0x041F
#define MASSMORE_PINK   0xF81F
#define MASSMORE_CYAN   0x07FF
#define NEON_GREEN      0x07E0
#define DARK_BG         0x10A2 // Dark Grey
#define ORANGE_RED      0xFA20
#define PURPLE_Haze     0x780F

// --- Global Variables ---
int currentPage = 1;
unsigned long lastPageSwitchTime = 0;
const unsigned long pageDuration = 5000; // 5000ms = 5 วินาที
bool pageInitialized = false; // ตัวแปรเช็กว่าวาด Background หรือยัง

// ตัวแปรสำหรับ Animation หน้า 1
int p1_circleR = 5;
int p1_grow = 1;

// ตัวแปรสำหรับ Animation หน้า 2 (Bouncing Ball)
int p2_ballX = 120, p2_ballY = 120;
int p2_dirX = 3, p2_dirY = 3;
int p2_radius = 8;

// ตัวแปรสำหรับ Animation หน้า 3 (Radar Pulse)
int p3_pulseR = 10;

void setup() {
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  lastPageSwitchTime = millis();
}

void loop() {
  unsigned long currentTime = millis();

  // ตรวจสอบว่าถึงเวลาเปลี่ยนหน้าหรือยัง
  if (currentTime - lastPageSwitchTime > pageDuration) {
    currentPage++;
    if (currentPage > 3) currentPage = 1; // วนกลับไปหน้า 1
    lastPageSwitchTime = currentTime;
    pageInitialized = false; // บอกให้วาด Background ใหม่
    tft.fillScreen(TFT_BLACK); // ล้างหน้าจอ
  }

  // เลือกทำงานตามหน้าปัจจุบัน
  switch (currentPage) {
    case 1:
      if (!pageInitialized) drawPage1_Static();
      runPage1_Anim();
      break;
    case 2:
      if (!pageInitialized) drawPage2_Static();
      runPage2_Anim();
      break;
    case 3:
      if (!pageInitialized) drawPage3_Static();
      runPage3_Anim();
      break;
  }
  
  delay(30); // หน่วงนิดหน่อยเพื่อความลื่นไหล (30-50ms)
}

// ==========================================
// PAGE 1: "Hello Worlds" (ธีมสีฟ้า/ชมพู)
// ==========================================
void drawPage1_Static() {
  tft.fillScreen(TFT_BLACK);
  // กรอบ Card
  tft.drawRoundRect(10, 10, 220, 220, 10, MASSMORE_CYAN);
  tft.drawRoundRect(12, 12, 216, 216, 9, MASSMORE_BLUE);
  // Header
  tft.fillRoundRect(20, 30, 200, 40, 5, MASSMORE_BLUE);
  
  tft.setTextColor(TFT_WHITE, MASSMORE_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString("Hello Worlds", 120, 50);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextFont(4);
  tft.drawString("We are", 120, 130);
  
  tft.setTextColor(MASSMORE_CYAN, TFT_BLACK);
  tft.drawString("Massmore", 120, 160);
  
  pageInitialized = true;
}

void runPage1_Anim() {
  // Animation: วงกลมหายใจ (Breathing Circle)
  tft.drawCircle(120, 200, p1_circleR, TFT_BLACK); // ลบวงเก่า
  
  if (p1_grow) {
    p1_circleR++;
    if (p1_circleR > 18) p1_grow = 0;
  } else {
    p1_circleR--;
    if (p1_circleR < 5) p1_grow = 1;
  }
  tft.drawCircle(120, 200, p1_circleR, MASSMORE_PINK); // วาดวงใหม่
}

// ==========================================
// PAGE 2: "System Ready" (ธีมสีเขียว/Cyber)
// ==========================================
void drawPage2_Static() {
  tft.fillScreen(DARK_BG);
  // เส้นตาราง Grid แบบ Cyber
  for(int i=0; i<240; i+=40) {
    tft.drawFastHLine(0, i, 240, 0x2124); // เส้นแนวนอนสีเทาเข้ม
    tft.drawFastVLine(i, 0, 240, 0x2124); // เส้นแนวตั้ง
  }
  
  // กล่องข้อความตรงกลาง
  tft.fillRect(40, 90, 160, 60, TFT_BLACK);
  tft.drawRect(40, 90, 160, 60, NEON_GREEN);
  
  tft.setTextColor(NEON_GREEN, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(1);
  tft.drawString("SYSTEM", 120, 110);
  tft.drawString("READY...", 120, 135);
  
  pageInitialized = true;
}

void runPage2_Anim() {
  // Animation: ลูกบอลเด้งดึ๋ง (Bouncing Ball)
  
  // 1. ลบลูกบอลเก่า (ใช้สีพื้นหลังทับ)
  // เนื่องจากพื้นหลังมี Grid การลบอาจจะทำให้เส้นหาย (เขียนทับด้วยสีพื้น)
  // แต่เพื่อความง่าย เราจะทับด้วยสีพื้น Dark BG ไปเลย
  tft.fillCircle(p2_ballX, p2_ballY, p2_radius, DARK_BG); 
  
  // 2. คำนวณตำแหน่งใหม่
  p2_ballX += p2_dirX;
  p2_ballY += p2_dirY;
  
  // 3. เช็กการชนขอบ (Bounce)
  if (p2_ballX > 230 || p2_ballX < 10) p2_dirX *= -1;
  if (p2_ballY > 230 || p2_ballY < 10) p2_dirY *= -1;
  
  // 4. วาดลูกบอลใหม่
  tft.fillCircle(p2_ballX, p2_ballY, p2_radius, ORANGE_RED);
}

// ==========================================
// PAGE 3: "Processing" (ธีมสีม่วง/ส้ม)
// ==========================================
void drawPage3_Static() {
  tft.fillScreen(TFT_BLACK);
  
  // วาดวงแหวนรอบนอก
  tft.drawCircle(120, 120, 110, PURPLE_Haze);
  tft.drawCircle(120, 120, 100, PURPLE_Haze);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(2);
  tft.setTextSize(2);
  tft.drawString("ESP32 Demo", 120, 30);
  
  tft.setTextColor(ORANGE_RED, TFT_BLACK);
  tft.setTextFont(4);
  tft.setTextSize(1);
  tft.drawString("Processing", 120, 210);

  pageInitialized = true;
}

void runPage3_Anim() {
  // Animation: เรดาร์กระจายออก (Pulse Radar) ตรงกลางจอ
  
  // ลบวงเก่า
  tft.drawCircle(120, 120, p3_pulseR, TFT_BLACK);
  tft.drawCircle(120, 120, p3_pulseR - 1, TFT_BLACK); // ลบเส้นหนาหน่อย
  
  // ขยายขนาด
  p3_pulseR += 2;
  
  // ถ้าใหญ่เกินกรอบ ให้เริ่มใหม่จากตรงกลาง
  if (p3_pulseR > 80) p3_pulseR = 10;
  
  // วาดวงใหม่ (เปลี่ยนสีตามขนาดได้ถ้าต้องการ)
  tft.drawCircle(120, 120, p3_pulseR, 0xFFE0); // สีเหลือง
  tft.drawCircle(120, 120, p3_pulseR - 1, 0xFFE0);
}
