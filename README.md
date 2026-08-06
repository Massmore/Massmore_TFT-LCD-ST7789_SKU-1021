<div align="center">

# 🖥️ SKU-1021 — TFT LCD ST7789 + Touch + SD

**โมดูลหน้าจอ IPS ST7789 · Capacitive Touch (CST816) · SD Card — by Massmore**

![ESP32-S3](https://img.shields.io/badge/MCU-ESP32--S3-red) ![ST7789](https://img.shields.io/badge/Display-ST7789-blue) ![Touch](https://img.shields.io/badge/Touch-CST816-green) ![License](https://img.shields.io/badge/by-Massmore-orange)

🛒 [หน้าสินค้า / Product](https://www.massmore.shop/products/5e1f85a3-6174-4861-aef3-dca6186013f2) · 📄 [บทความ / Article](https://www.massmore.shop/docs/1) · 📦 [GitHub](https://github.com/Massmore/SKU-1021-TFT-LCD-ST7789)

</div>

---

## 📂 เลือกชุดที่จะใช้ / Pick your toolchain

| โฟลเดอร์ | สำหรับ | เริ่มยังไง |
|---|---|---|
| 🟦 **`Arduino_Example/`** | Arduino IDE (ของเดิม) | ติดตั้งไลบรารีจาก `Arduino_Library/` แล้วเปิดตัวอย่าง |
| 🟩 **`VSCode_PlatformIO/`** | VS Code + PlatformIO (ใหม่) | เปิดโฟลเดอร์ → Build → Upload (ไลบรารีมาเอง) |

---

## 🔌 ฮาร์ดแวร์ & ขา / Hardware & Pins

| รายการ | รายละเอียด |
|---|---|
| 🖼️ จอ / Display | ST7789V IPS · 2.0" = 240×320 (1.3"/1.54" = 240×240) |
| ✋ Touch | CST816 · I²C · address `0x15` |
| 💾 SD Card | Micro SD · **FAT32** · ใช้บัส SPI ร่วมกับจอ |
| 🔧 MCU | ESP32-S3 · Flash 16MB · PSRAM 8MB · USB-UART **CH340** |
| ⚡ ไฟเลี้ยง | 3.3V (แนะนำ) · logic **3.3V** |

**ขา ESP32-S3 / Pin map**

```
🖼️  TFT (SPI/HSPI)   MOSI 11 · SCLK 12 · MISO 13 · CS 17 · DC 3 · RST 18 · BL→3.3V
💾  SD Card          MISO 13 · MOSI 11 · SCLK 12 · CS 0        (บัสเดียวกับจอ)
✋  Touch (I²C)       SDA 14 · SCL 15 · addr 0x15
```

> ⚠️ จอ + SD ใช้บัส SPI เส้นเดียวกัน แยกด้วยขา CS (จอ=17, SD=0) · GPIO0 เป็น strapping pin ถ้าบอร์ด boot ไม่ขึ้นให้เช็กจุดนี้
> The TFT & SD share one SPI bus (CS: TFT=17, SD=0). GPIO0 is a strapping pin — check it if the board won't boot.

---

## 🟦 ใช้กับ Arduino IDE

1. 📥 ติดตั้ง **ESP32 board package** (Boards Manager → ค้นหา `esp32`)
2. 📚 คัดลอกทุกโฟลเดอร์ใน `Arduino_Library/` → `Documents/Arduino/libraries/`
3. ⚙️ บอร์ด: **ESP32S3 Dev Module** · Flash **16MB** · PSRAM **OPI** · Upload **921600**
4. ▶️ เปิดตัวอย่างใน `Arduino_Example/` แล้วกด **Upload**

**ตัวอย่างที่ให้มา / Examples**

| # | โฟลเดอร์ | ทำอะไร |
|---|---|---|
| 1️⃣ | `1_TFT_LCD_ST7789_Demo` | UI + แอนิเมชัน 3 หน้า |
| 2️⃣ | `2_TFT_LCD_ST7789_DMA` | แสดงภาพด้วย DMA |
| 3️⃣ | `3_TFT_LCD_ST7789_PNG` | ถอดรหัส + แสดงภาพ PNG |
| 4️⃣ | `4-TFT_LCD_Touch_ESP32S3_Demo` | Touch + SD + TFT (multitask) |

---

## 🟩 ใช้กับ VS Code + PlatformIO

เปิดโฟลเดอร์ `VSCode_PlatformIO/` — ตั้งค่าจอทั้งหมดอยู่ใน `platformio.ini` แล้ว **ไม่ต้องแก้ `User_Setup.h`**
Open `VSCode_PlatformIO/` — all display config is in `platformio.ini`, **no `User_Setup.h` to edit**.

1. 🧩 ติดตั้ง **VS Code** + ส่วนขยาย **PlatformIO IDE**
2. 📂 `Open Folder…` → เลือก `VSCode_PlatformIO/`
3. ⏬ PlatformIO ดึง `espressif32` + `TFT_eSPI` อัตโนมัติ
4. ✅ กด **Build** → ➡️ **Upload**

```bash
# โปรแกรมหลัก (SD + Touch + TFT)
pio run -t upload -t monitor

# ทดสอบ SD card แยกเดี่ยว
pio run -e sd_test -t upload -t monitor
```

> 🔌 บอร์ดใช้ชิป **CH340** — ถ้าไม่เห็นพอร์ตให้ติดตั้งไดรเวอร์ หรือกำหนด `upload_port` เองใน `platformio.ini` · Serial Monitor: **115200 baud**

---

## 🆘 แก้ปัญหาเบื้องต้น / Troubleshooting

| อาการ | แก้ยังไง |
|---|---|
| ⬜ จอไม่แสดงผล | เช็กขา DC/RST/CS + backlight ต่อ 3.3V · ยืนยันขาตรงตาราง |
| 🎨 สีเพี้ยน / กลับสี | ปรับ `TFT_RGB_ORDER` และ `TFT_INVERSION_ON/OFF` |
| 💾 `Card Mount Failed` | ฟอร์แมต SD เป็น **FAT32** · จ่ายไฟ 3.3V ให้พอ · ใช้ env `sd_test` ไล่ทีละความเร็ว |
| ✋ Touch ไม่ตอบสนอง | เช็ก SDA 14 / SCL 15 · address `0x15` · ดู Serial ว่าเจอ `Find CST816T!` |
| ⬆️ อัปโหลดไม่ได้ | ติดตั้งไดรเวอร์ **CH340** · กด **BOOT** ค้างขณะเริ่มอัปโหลด |

---

## 🚀 ต่อยอด / Extend

✋ อ่านพิกัดสัมผัสจาก `get_touch_coords()` เพื่อทำปุ่มบนจอ · 🖼️ แสดงรูปจาก SD ด้วย `TJpg_Decoder` (JPEG) / `PNGdec` (PNG) · 📊 ทำ dashboard, นาฬิกา, เครื่องเล่นสื่อ

---

<div align="center">

**© MASSMORE BIZ CO., LTD.** — รับออกแบบและผลิต PCB · 3D Printing · IoT Programming
🌐 [massmore.shop](https://www.massmore.shop) · TFT_eSPI by [Bodmer](https://github.com/Bodmer/TFT_eSPI)

</div>
