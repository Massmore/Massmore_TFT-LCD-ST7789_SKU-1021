# SKU-1021 — TFT LCD ST7789 (2.0" + Touch CST816 + SD Card)

> โมดูลหน้าจอ TFT LCD ST7789 SPI 1.3" / 1.54" / 2.0" — IPS Display + Capacitive Touch (CST816) + SD Card — by **Massmore**
>
> TFT LCD ST7789 SPI display module (1.3" / 1.54" / 2.0") — IPS panel + Capacitive Touch (CST816) + SD card slot — by **Massmore**

- 🛒 หน้าสินค้า / Product page: https://www.massmore.shop/products/5e1f85a3-6174-4861-aef3-dca6186013f2
- 📄 บทความ / Article: https://www.massmore.shop/docs/1
- 📦 GitHub: https://github.com/Massmore/SKU-1021-TFT-LCD-ST7789

---

## 1. โครงสร้างโปรเจกต์ / Repository layout

โปรเจกต์นี้แยกออกเป็น **2 ชุด** อย่างชัดเจน เพื่อให้เลือกใช้ตามเครื่องมือที่ถนัด
This repository is split into **two independent tracks** so you can pick the toolchain you prefer.

```
SKU-1021-TFT-LCD-ST7789/
├── Arduino_Example/        # ← ตัวอย่างสำหรับ Arduino IDE (ของเดิม / original)
│   ├── 1_TFT_LCD_ST7789_Demo/         Basic UI + animation (3 หน้า)
│   ├── 2_TFT_LCD_ST7789_DMA/          แสดงภาพด้วย DMA
│   ├── 3_TFT_LCD_ST7789_PNG/          ถอดรหัสและแสดงภาพ PNG
│   └── 4-TFT_LCD_Touch_ESP32S3_Demo/  Touch + SD + TFT (multitask)
│
├── Arduino_Library/        # ← ไลบรารีที่ต้องติดตั้งสำหรับ Arduino IDE
│   ├── TFT_eSPI/           (มี User_Setup สำเร็จรูปให้แล้ว)
│   ├── PNGdec/
│   ├── JPEGDecoder/
│   └── TJpg_Decoder/
│
├── VSCode_PlatformIO/      # ← โปรเจกต์ VS Code + PlatformIO (อันใหม่)
│   ├── platformio.ini      การตั้งค่าทั้งหมด (ไม่ต้องแก้ User_Setup.h)
│   ├── src/                main.cpp, cst816d.cpp, sd_test.cpp
│   ├── include/            cst816d.h
│   └── lib/  test/
│
└── README.md               ← คู่มือนี้ / this manual
```

> **สรุปสั้น ๆ / TL;DR**
> - ถ้าใช้ **Arduino IDE** → ไปที่โฟลเดอร์ `Arduino_Example/` และติดตั้งไลบรารีจาก `Arduino_Library/`
> - ถ้าใช้ **VS Code + PlatformIO** → เปิดโฟลเดอร์ `VSCode_PlatformIO/` ได้เลย ไลบรารีจะถูกดึงอัตโนมัติ
> - Using **Arduino IDE** → open `Arduino_Example/`, install libraries from `Arduino_Library/`.
> - Using **VS Code + PlatformIO** → just open `VSCode_PlatformIO/`; dependencies are fetched automatically.

---

## 2. ฮาร์ดแวร์ / Hardware

| รายการ / Item | รายละเอียด / Detail |
|---|---|
| จอ / Display | ST7789V IPS, 2.0" 240×320 (รุ่น 1.3"/1.54" = 240×240) |
| Touch | CST816 (Capacitive), I²C, address `0x15` |
| Storage | Micro SD card (FAT/FAT32), บัส SPI ร่วมกับจอ |
| MCU (ตัวอย่างนี้) | ESP32-S3 DevKitC-1, Flash 16MB, Octal PSRAM 8MB |
| USB-UART | CH340 (เป็น UART bridge ธรรมดา ไม่ใช่ USB native) |
| ไฟเลี้ยง / Power | 3.3V (แนะนำ) ถึง 5V ผ่าน regulator บนบอร์ด, logic 3.3V |

> ⚠️ ระดับสัญญาณโลจิกเป็น **3.3V** หากต่อกับบอร์ด 5V (เช่น Arduino UNO) ควรใช้ level shifter
> Logic level is **3.3V**. If you drive it from a 5V board (e.g. Arduino UNO), use a level shifter.

### 2.1 ตารางขา / Pin map (ตามตัวอย่าง ESP32-S3 / as used in these examples)

**จอ ST7789 — SPI (HSPI):**

| สัญญาณ / Signal | ESP32-S3 GPIO |
|---|---|
| MOSI / SDA | **11** |
| SCLK / SCL | **12** |
| MISO | **13** |
| CS | **17** |
| DC | **3** |
| RST | **18** |
| BL (Backlight) | ต่อ 3.3V ตรง (ไม่ควบคุมด้วย GPIO / `TFT_BL = -1`) |

**SD card — ใช้บัส SPI เดียวกับจอ / shares the same SPI bus:**

| สัญญาณ / Signal | ESP32-S3 GPIO |
|---|---|
| MISO | 13 |
| MOSI | 11 |
| SCLK | 12 |
| CS | **0** |

**Touch CST816 — I²C:**

| สัญญาณ / Signal | ESP32-S3 GPIO |
|---|---|
| SDA | **14** |
| SCL | **15** |
| RST | ไม่ต่อ / not connected (`-1`) |
| INT | ไม่ต่อ / not connected (`-1`) |
| I²C address | `0x15`, 400 kHz |

> 💡 จอกับ SD card **ใช้บัส SPI เส้นเดียวกัน** แยกกันด้วยขา CS คนละขา (จอ = 17, SD = 0)
> The TFT and SD card **share one SPI bus**, separated only by their chip-select pins (TFT = 17, SD = 0).
>
> ⚠️ GPIO0 เป็น **strapping pin** ของ ESP32-S3 (ใช้เข้าโหมด boot) — ปกติใช้เป็น SD_CS ได้ แต่ถ้า SD ดึงขานี้ลง LOW ตอนรีเซ็ต อาจ boot ไม่ขึ้น ให้ตรวจสอบจุดนี้หากบอร์ดค้าง
> GPIO0 is an ESP32-S3 strapping pin. It works as SD_CS, but if the card module pulls it LOW at reset the board may fail to boot — check this if you see boot hangs.

---

## 3. เริ่มต้นแบบ Arduino IDE / Getting started — Arduino IDE

1. **ติดตั้ง ESP32 board package**
   `File → Preferences → Additional Boards Manager URLs` ใส่:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   จากนั้น `Tools → Board → Boards Manager` ค้นหา **esp32** แล้วติดตั้ง

2. **ติดตั้งไลบรารี** — คัดลอกทุกโฟลเดอร์ใน `Arduino_Library/` ไปไว้ที่โฟลเดอร์ `Documents/Arduino/libraries/`
   Copy every folder inside `Arduino_Library/` into your `Documents/Arduino/libraries/` folder.
   - `TFT_eSPI`, `PNGdec`, `JPEGDecoder`, `TJpg_Decoder`

3. **ตั้งค่า TFT_eSPI** — ไลบรารี `TFT_eSPI` ในชุดนี้มีไฟล์ตั้งค่าสำเร็จรูปให้แล้ว:
   `Arduino_Library/TFT_eSPI/User_Setup_ST7789_ESP32S3.h`
   ตรวจสอบว่าไฟล์ `User_Setup_Select.h` ชี้มาที่ setup ของ ST7789 (ขาตรงกับตารางในข้อ 2.1)
   The bundled `TFT_eSPI` already contains a ready-made setup; make sure `User_Setup_Select.h` points to the ST7789 setup whose pins match §2.1.

4. **เลือกบอร์ด** — `Tools → Board → ESP32S3 Dev Module`
   - Flash Size: **16MB**
   - PSRAM: **OPI PSRAM**
   - Upload Speed: 921600

5. เปิดตัวอย่างในโฟลเดอร์ `Arduino_Example/` แล้วกด **Upload**

### ตัวอย่างที่มี / Included examples

| # | โฟลเดอร์ / Folder | ทำอะไร / What it does |
|---|---|---|
| 1 | `1_TFT_LCD_ST7789_Demo` | เดโมพื้นฐาน: UI + แอนิเมชัน 3 หน้า (วงกลม, ลูกบอลเด้ง, เรดาร์) |
| 2 | `2_TFT_LCD_ST7789_DMA` | แสดงภาพด้วย DMA (เร็วขึ้น) |
| 3 | `3_TFT_LCD_ST7789_PNG` | ถอดรหัสและแสดงภาพ PNG (`panda.h`) |
| 4 | `4-TFT_LCD_Touch_ESP32S3_Demo` | รวม Touch + SD + TFT ทำงานแบบ multitask (FreeRTOS) |

---

## 4. เริ่มต้นแบบ VS Code + PlatformIO / Getting started — VS Code + PlatformIO

นี่คือโปรเจกต์อันใหม่ในโฟลเดอร์ `VSCode_PlatformIO/` — **ไม่ต้องแก้ `User_Setup.h`** เพราะตั้งค่าจอทั้งหมดผ่าน `build_flags` ใน `platformio.ini` แล้ว
This is the new project in `VSCode_PlatformIO/`. There is **no `User_Setup.h` to edit** — the entire display configuration is passed as `build_flags` in `platformio.ini`.

1. ติดตั้ง **VS Code** + ส่วนขยาย **PlatformIO IDE**
2. `File → Open Folder…` แล้วเลือกโฟลเดอร์ `VSCode_PlatformIO/`
3. PlatformIO จะดาวน์โหลด `platform espressif32` และไลบรารี `TFT_eSPI` ให้อัตโนมัติ
4. กด **Build** (✓) แล้ว **Upload** (→) จาก status bar ด้านล่าง

### 4.1 Environments ที่กำหนดไว้ / Defined environments

| Environment | ใช้ทำอะไร / Purpose | คำสั่ง / Command |
|---|---|---|
| `esp32-s3-devkitc-1` | โปรแกรมหลัก: SD + Touch + TFT | `pio run -t upload -t monitor` |
| `sd_test` | ทดสอบ SD card แยกเดี่ยว ๆ เพื่อไล่ปัญหา | `pio run -e sd_test -t upload -t monitor` |

> `build_src_filter` แยกไฟล์ให้ `sd_test.cpp` คอมไพล์เฉพาะใน env `sd_test` และตัดออกจากโปรแกรมหลัก จึงไม่มีปัญหา `setup()`/`loop()` ซ้ำกัน
> `build_src_filter` compiles `sd_test.cpp` only in the `sd_test` env and excludes it from the main build, so there is no duplicate `setup()`/`loop()`.

### 4.2 การตั้งค่าสำคัญใน platformio.ini / Key settings

```ini
board = esp32-s3-devkitc-1
board_upload.flash_size = 16MB          ; override เป็น 16MB
board_build.partitions  = default_16MB.csv
board_build.psram_type  = opi           ; Octal PSRAM 8MB
lib_deps = bodmer/TFT_eSPI @ ^2.5.43
monitor_speed = 115200
upload_speed  = 921600
```

ค่าจอ (ST7789, ขา, ฟอนต์, ความเร็ว SPI) ถูกส่งเป็น `-D...` build flags ทั้งหมด ตรงกับไฟล์อ้างอิง `Arduino_Library/TFT_eSPI/User_Setup_ST7789_ESP32S3.h`
All display settings (driver, pins, fonts, SPI speed) are `-D...` build flags that mirror the reference `User_Setup_ST7789_ESP32S3.h`.

### 4.3 เลือกพอร์ต (CH340) / Selecting the port

บอร์ดใช้ชิป **CH340** โดยปกติ PlatformIO ตรวจจับพอร์ตอัตโนมัติ ถ้าไม่เจอ ให้เปิดคอมเมนต์ใน `platformio.ini`:
The board uses a **CH340** bridge. Auto-detect usually works; if not, uncomment in `platformio.ini`:

```ini
;upload_port = /dev/cu.wchusbserialXXXX   ; macOS/Linux
;upload_port = COM5                        ; Windows
;monitor_port = /dev/cu.wchusbserialXXXX
```

> macOS/Windows อาจต้องติดตั้งไดรเวอร์ CH340 ก่อน หากไม่เห็นพอร์ต
> You may need the CH340 driver on macOS/Windows if the port doesn't appear.

---

## 5. อธิบายโค้ด / Code walkthrough (`VSCode_PlatformIO/src`)

**`main.cpp`** — โปรแกรมหลัก ทำงานแบบ multitask ด้วย FreeRTOS:
- `TaskDisplayCode` (ปักไว้ที่ core 1) เป็น **เจ้าของจอเพียงผู้เดียว** — เรียก `tft.init()`, วาด UI และจุดสัมผัส
- `loop()` อ่านพิกัดสัมผัสจาก CST816 แล้วเก็บใส่ตัวแปร `shared_x/shared_y` (ไม่แตะจอโดยตรง)

`main.cpp` runs multitasked on FreeRTOS. `TaskDisplayCode` (pinned to core 1) is the **single owner of the TFT** — it calls `tft.init()`, draws the UI, and renders touch points. `loop()` only reads the CST816 and publishes coordinates via `shared_x/shared_y`.

**`cst816d.cpp` / `cst816d.h`** — ไดรเวอร์ touch CST816 ผ่าน I²C (`touch_init`, `get_touch_coords`)

**`sd_test.cpp`** — โปรแกรมทดสอบ SD card แบบแยกเดี่ยว ลองmount ที่ 4MHz → 1MHz → 400kHz พร้อม log บอกจุดตรวจสอบเมื่อ mount ไม่ผ่าน

### 5.1 สิ่งที่แก้ไข / ปรับปรุงในเวอร์ชัน PlatformIO นี้ / Fixes applied in this PlatformIO version

ในการย้ายจาก Arduino IDE มาเป็น PlatformIO ได้ตรวจและแก้ไขจุดต่อไปนี้:
When porting from Arduino IDE to PlatformIO the following were reviewed and fixed:

1. **แก้ race condition ในการวาดจอ / Fixed a TFT race condition.**
   เดิม `loop()` เรียก `tft.fillCircle()` โดยตรง ขณะที่ `TaskDisplayCode` ก็วาดจอด้วย — TFT_eSPI ไม่ thread-safe การวาดจาก 2 task พร้อมกันบนบัส SPI เดียวกันทำให้ภาพเพี้ยนได้ ตอนนี้ให้ `TaskDisplayCode` เป็นผู้วาดจอคนเดียว ส่วน `loop()` แค่ส่งพิกัดผ่านตัวแปร
   Previously `loop()` drew to the TFT directly while `TaskDisplayCode` also drew — TFT_eSPI is not thread-safe, so two tasks sharing the SPI bus could corrupt the display. Now `TaskDisplayCode` is the sole drawer; `loop()` only publishes coordinates.

2. **แก้ลำดับการเริ่มต้น / Fixed init ordering.**
   เพิ่มธง `displayReady` เพื่อกัน `loop()` ไม่ให้ทำงานก่อน `tft.init()` จะเสร็จ
   Added a `displayReady` flag so `loop()` cannot run before `tft.init()` completes.

3. **แก้ชนิดตัวแปร I²C / I²C type correctness.**
   `requestFrom()` cast อาร์กิวเมนต์เป็น `(uint16_t, uint8_t)` ให้ตรง overload บน ESP32 ลด warning ตอนคอมไพล์
   `requestFrom()` now casts its arguments to `(uint16_t, uint8_t)` to match the ESP32 overload and avoid compiler warnings.

> หมายเหตุ / Note: โค้ดถูกตรวจแบบ static review ตามการตั้งค่า PlatformIO (ยังไม่ได้ compile จริงในเครื่องนี้ตามที่ตกลง) แนะนำให้กด Build หนึ่งครั้งบนเครื่องคุณก่อนใช้งานจริง
> The code was checked by static review against the PlatformIO config (not compiled here, as agreed). Please run **Build** once on your machine before deploying.

---

## 6. การแก้ปัญหาเบื้องต้น / Troubleshooting

| อาการ / Symptom | สาเหตุ / แนวทางแก้ / Cause & fix |
|---|---|
| จอขาว/ดำ ไม่แสดงอะไร | ตรวจขา DC/RST/CS และ backlight (BL ต่อ 3.3V), ยืนยันไดรเวอร์ ST7789 และขาตรงกับ §2.1 |
| สีเพี้ยน / กลับสี | ปรับ `TFT_RGB_ORDER` และ `TFT_INVERSION_ON/OFF` |
| ภาพเลื่อน/ขอบเพี้ยน | ตรวจ `TFT_WIDTH`/`TFT_HEIGHT` และ `setRotation()` |
| `Card Mount Failed` | ฟอร์แมต SD เป็น **FAT32** (ไม่ใช่ exFAT), จ่ายไฟ 3.3V ให้พอ, ตรวจบัดกรี MISO/MOSI/SCLK/CS — ใช้ env `sd_test` เพื่อไล่ทีละความเร็ว |
| Touch ไม่ตอบสนอง | ตรวจ SDA=14 / SCL=15, address `0x15`, ดู Serial ว่าเจอ `Find CST816T!` หรือไม่ |
| อัปโหลดไม่ได้ / ไม่เห็นพอร์ต | ติดตั้งไดรเวอร์ **CH340**, กำหนด `upload_port` เอง, ลองกดปุ่ม BOOT ค้างขณะเริ่มอัปโหลด |
| บอร์ด boot ไม่ขึ้น | GPIO0 เป็น strapping pin — ตรวจว่าโมดูล SD ไม่ดึง GPIO0 ลง LOW ตอนรีเซ็ต |

การดู log ให้เปิด Serial Monitor ที่ **115200 baud** / Open the Serial Monitor at **115200 baud** for logs.

---

## 7. ต่อยอด / Extending the project

- เพิ่มปุ่มบนจอด้วยการอ่านพิกัดสัมผัสจาก `get_touch_coords()` แล้วเทียบกับกรอบพื้นที่ปุ่ม
- แสดงรูปจาก SD card ด้วย `TJpg_Decoder` (JPEG) หรือ `PNGdec` (PNG)
- ทำ UI dashboard / นาฬิกา / เครื่องเล่นสื่อ ตามตัวอย่างการใช้งานในหน้าสินค้า
- Read touch coordinates from `get_touch_coords()` to build on-screen buttons; display images from SD via `TJpg_Decoder` (JPEG) or `PNGdec` (PNG); build dashboards, watches or media players as suggested on the product page.

---

## 8. เครดิตและลิขสิทธิ์ / Credits & license

- ไลบรารี `TFT_eSPI` โดย Bodmer — https://github.com/Bodmer/TFT_eSPI
- `PNGdec`, `JPEGDecoder`, `TJpg_Decoder` — ตามลิขสิทธิ์ของแต่ละไลบรารี (ดูไฟล์ LICENSE ในแต่ละโฟลเดอร์)
- โค้ดตัวอย่างและคู่มือนี้ — © MASSMORE BIZ CO., LTD.

รับออกแบบและผลิต PCB, 3D Printing, IoT Programming และอุปกรณ์อิเล็กทรอนิกส์ — Massmore
For custom PCB design & manufacturing, 3D printing, and IoT programming — contact Massmore: https://www.massmore.shop
