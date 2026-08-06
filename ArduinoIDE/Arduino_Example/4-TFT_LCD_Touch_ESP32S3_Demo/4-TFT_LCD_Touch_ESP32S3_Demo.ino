#include "SD.h"
#include "SPI.h"
#include "cst816d.h"
#include <TFT_eSPI.h> // เรียกใช้ไลบรารีจอภาพ

// --- Config Pins (สำหรับ SD และ Touch) ---
#define SPI_MISO 13 
#define SPI_MOSI 11
#define SPI_SCLK 12
#define SD_CS 0

#define TP_INT -1
#define TP_RST -1
#define TP_SDA 14
#define TP_SCL 15

// สร้างตัวแปร Hardware
SPIClass fspi(FSPI);
TFT_eSPI tft = TFT_eSPI(); // เรียกใช้ Object ของจอ

// ตัวแปร Global สำหรับรับส่งข้อมูลระหว่าง Task
volatile int shared_x = 0;
volatile int shared_y = 0;
volatile bool isTouched = false;
volatile bool updateScreen = false;

uint64_t cardSize;
toucht_coords_t toucht_coords;
uint16_t sd_count = 0;

// สร้าง Task Handle
TaskHandle_t Task1; 

// --- ฟังก์ชันสำหรับ Task จอภาพ (รันบน Core 1) ---
void TaskDisplayCode(void * pvParameters) {
  // 1. Setup จอภาพ (ทำครั้งเดียว)
  tft.init();
  tft.setRotation(3); // แนวนอน
  tft.fillScreen(TFT_BLACK);
  
  // วาด UI พื้นฐาน
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("SD & Touch Test", 10, 10);
  
  tft.drawRect(5, 40, 310, 190, TFT_BLUE); // กรอบพื้นที่วาด

  // Loop ของ Task นี้ (ทำงานตลอดเวลาเหมือน void loop)
  for(;;) {
    if (updateScreen) {
      // แสดงพิกัดเป็นตัวเลข
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.setTextSize(2);
      String coords = "X: " + String(shared_x) + "  Y: " + String(shared_y) + "   ";
      tft.drawString(coords, 20, 200);

      // วาดจุดที่ตำแหน่ง Touch
      // แปลงพิกัด (Map) ถ้าจำเป็น หรือใช้ค่าตรงๆ
      if (shared_x > 0 && shared_y > 0) {
          tft.fillCircle(shared_x, shared_y, 5, TFT_RED);
      }
      
      updateScreen = false; // เคลียร์สถานะเมื่อวาดเสร็จ
    }
    
    // หน่วงเวลาเล็กน้อยเพื่อให้ Watchdog ทำงานและไม่กิน CPU 100%
    vTaskDelay(10 / portTICK_PERIOD_MS); 
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Multi-Tasking: SD + Touch + TFT");

  // 1. เริ่มต้น Hardware อื่นๆ
  touch_init(TP_SDA, TP_SCL, TP_RST, TP_INT);

  fspi.begin(SPI_SCLK, SPI_MISO, SPI_MOSI, -1); 
  if (!SD.begin(SD_CS, fspi)) {
    Serial.println("Card Mount Failed");
  } else {
    cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
  }

  // 2. สร้าง Task สำหรับจอภาพ
  // xTaskCreatePinnedToCore(ฟังก์ชัน, "ชื่อ", StackSize, Parameter, Priority, Handle, CoreID)
  xTaskCreatePinnedToCore(
                    TaskDisplayCode,   /* ฟังก์ชันที่ทำงาน */
                    "DisplayTask",     /* ชื่อ Task */
                    10000,             /* ขนาด Stack (10k น่าจะพอ) */
                    NULL,              /* พารามิเตอร์ */
                    1,                 /* Priority (1 = ปกติ) */
                    &Task1,            /* Task Handle */
                    1);                /* **Core ID** (0 หรือ 1) */ 
                    
  // หมายเหตุ: โดยปกติ Arduino loop() ก็รันบน Core 1 
  // การแยก Task นี้จะช่วยให้เราเขียนโค้ดแยกส่วนกันได้ชัดเจน
}
void loop() {
  // 1. อ่านค่า Touch
  if (get_touch_coords(&toucht_coords))
  {
    // --- ส่วนคำนวณ Map ตำแหน่ง (สำคัญ) ---
    
    // แปลงค่า Y ของ Touch (300-0) ไปเป็น X ของจอ (0-320)
    int draw_x = map(toucht_coords.y, 300, 0, 0, 320);
    
    // แปลงค่า X ของ Touch (200-0) ไปเป็น Y ของจอ (0-240)
    int draw_y = map(toucht_coords.x, 200, 0, 0, 240);

    // ป้องกันค่าเกินขอบจอ (Constrain)
    if (draw_x < 0) draw_x = 0;
    if (draw_x > 320) draw_x = 320;
    if (draw_y < 0) draw_y = 0;
    if (draw_y > 240) draw_y = 240;

    // --- ส่วนแสดงผล ---
    
    // Debug ดูค่าที่แปลงแล้ว
    Serial.print("Raw[x,y]: ");
    Serial.print(toucht_coords.x); Serial.print(","); Serial.print(toucht_coords.y);
    Serial.print("  ->  Screen[x,y]: ");
    Serial.print(draw_x); Serial.print(","); Serial.println(draw_y);

    // วาดจุดลงบนจอ (สีเขียว)
    tft.fillCircle(draw_x, draw_y, 4, TFT_GREEN);
  }
  
  // (ส่วนจัดการ SD Card คงเดิม...)
  if (sd_count++ > 200) {
    sd_count = 0;
    // tft.fillScreen(TFT_BLACK); // ถ้าอยากเคลียร์หน้าจอให้เปิดบรรทัดนี้
  }
  delay(10);
}