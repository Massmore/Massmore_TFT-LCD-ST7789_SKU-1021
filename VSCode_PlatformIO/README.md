# VSCode_PlatformIO — ESP32-S3 + ST7789 + CST816 Touch + SD

โปรเจกต์ VS Code + PlatformIO สำหรับ SKU-1021 (ดูคู่มือฉบับเต็มที่ [`../README.md`](../README.md))
VS Code + PlatformIO project for SKU-1021 (full manual in [`../README.md`](../README.md)).

## เริ่มใช้งาน / Quick start

1. เปิดโฟลเดอร์นี้ใน VS Code ที่มีส่วนขยาย **PlatformIO IDE** / Open this folder in VS Code with the **PlatformIO IDE** extension.
2. PlatformIO ดึง `espressif32` + `TFT_eSPI` ให้อัตโนมัติ / dependencies are fetched automatically.
3. Build & Upload:

```bash
# โปรแกรมหลัก (SD + Touch + TFT) / main app
pio run -t upload -t monitor

# ทดสอบ SD card แยกเดี่ยว / standalone SD bring-up test
pio run -e sd_test -t upload -t monitor
```

- การตั้งค่าจอทั้งหมดอยู่ใน `platformio.ini` (build flags) — **ไม่มี `User_Setup.h` ให้แก้**
- All display config lives in `platformio.ini` build flags — there is **no `User_Setup.h`** to edit.
- Serial Monitor: **115200 baud**. บอร์ดใช้ชิป **CH340** (อาจต้องติดตั้งไดรเวอร์)

ขา / Pins: TFT SPI (MOSI 11, SCLK 12, MISO 13, CS 17, DC 3, RST 18) · SD_CS 0 · Touch I²C (SDA 14, SCL 15, addr 0x15)
