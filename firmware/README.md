# Massmore_ST7789 — Factory Test Firmware

เฟิร์มแวร์ที่ compile แล้วของตัวอย่าง [`07_Factory_Test`](../ArduinoIDE/Massmore_ST7789/examples/07_Factory_Test/)
สำหรับ **ESP32 (Classic) DevKit — Primary Factory Test MCU** และ **ESP32-S3 DevKitC-1** ใช้ตรวจโมดูลจอ
Massmore ST7789 SKU-1021 ก่อนส่งลูกค้า (Outgoing QA/QC) และให้ **Massmore Web Serial Monitor** อ่านผลอัตโนมัติ

## Files

| File | Flash offset | Use |
|---|---|---|
| `bin/Massmore_ST7789_FactoryTest_ESP32.bin` | `0x0` | **Merged image** ESP32 Classic (bootloader + partitions + app) — ไฟล์เดียวจบ |
| `bin/Massmore_ST7789_FactoryTest_ESP32_app.bin` | `0x10000` | Application เท่านั้น (ESP32 Classic) |
| `bin/Massmore_ST7789_FactoryTest_ESP32S3.bin` | `0x0` | **Merged image** ESP32-S3 |
| `bin/Massmore_ST7789_FactoryTest_ESP32S3_app.bin` | `0x10000` | Application เท่านั้น (ESP32-S3) |
| `manifest.json` | — | Manifest สำหรับ ESP Web Tools (เลือก chip family ให้อัตโนมัติ) |

Build: pioarduino `55.03.311` (Arduino-ESP32 Core 3.3.11), env `esp32dev` / `esp32-s3-devkitc-1`, partition default, variant ใน binary = **2.0" Touch + SD** (`FT_HAS_TOUCH 1`, `FT_HAS_SD 1`, panel 240x320)

> จอ 1.3" / 1.54" (240x240, ไม่มี SD / Touch) ต้อง build เองด้วย `-DMASSMORE_ST7789_PANEL=240` และตั้ง `FT_HAS_TOUCH 0`, `FT_HAS_SD 0` ใน sketch

## Wiring (Primary MCU = ESP32 Classic)

| Module pin | ESP32 DevKit | ESP32-S3 DevKitC-1 | Note |
|---|---|---|---|
| `VCC` | 3V3 | 3V3 | โมดูลเป็น logic 3.3 V |
| `GND` | GND | GND | |
| `SCL` (SPI clock) | GPIO 18 | GPIO 12 | |
| `SDA` (SPI MOSI) | GPIO 23 | GPIO 11 | bidirectional — ใช้อ่าน ID / GRAM |
| `RES` | GPIO 4 | GPIO 18 | |
| `DC` | GPIO 2 | GPIO 3 | |
| `CS` | GPIO 16 | GPIO 17 | |
| `BLK` | 3V3 | 3V3 | backlight เปิดตลอด |
| `SD_MISO` | GPIO 19 | GPIO 13 | รุ่น 2.0" มี SD |
| `SD_MOSI` / `SD_SCK` | ร่วมกับ SDA / SCL | ร่วมกับ SDA / SCL | bus เดียวกับจอ |
| `SD_CS` | GPIO 5 | GPIO 0 | S3: GPIO0 เป็น strapping pin |
| `TP_SDA` / `TP_SCL` | GPIO 21 / 22 | GPIO 14 / 15 | รุ่น Touch (I2C 0x15) |

**เสียบ Micro SD (FAT32) ก่อนทดสอบ** — ถ้าไม่มีการ์ด `SD_MOUNT` จะ FAIL

## Flashing

**1. esptool (command line)** — ไฟล์เดียวจบ

```bash
# ESP32 Classic
esptool.py --chip esp32 --port /dev/cu.usbserial-0001 --baud 921600 \
  write_flash -z 0x0 bin/Massmore_ST7789_FactoryTest_ESP32.bin

# ESP32-S3
esptool.py --chip esp32s3 --port /dev/cu.usbmodem1101 --baud 921600 \
  write_flash -z 0x0 bin/Massmore_ST7789_FactoryTest_ESP32S3.bin
```

ถ้าอัพโหลดไม่นิ่งให้ลด `--baud` เป็น `460800` หรือ `115200` · Windows ใช้ `--port COM5`

**2. PlatformIO** — เปิดโฟลเดอร์ `PlatformIO/` แล้ว `pio run -e esp32dev -t upload` (main.cpp คือ Factory Test อยู่แล้ว)

**3. Web flasher (ESP Web Tools)** — วางโฟลเดอร์ `firmware/` บนเว็บ HTTPS แล้วใช้

```html
<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js"></script>
<esp-web-install-button manifest="firmware/manifest.json"></esp-web-install-button>
```

## Serial Output Format (115200 8N1, English only)

ทุกบรรทัดที่เว็บ parse ขึ้นต้นด้วย `#` · บรรทัดอื่นเป็น human-readable

```text
#MASSMORE_FACTORY_TEST v1.0
#PRODUCT Massmore_ST7789
#MCU ESP32
#RESULT <TEST_NAME> <PASS|FAIL> <value>      (one line per sub-test)
#VERDICT <PASS|FAIL> [<REASON>]              (exactly once, last # line)
[PASS] SENSOR QA PASSED - READY TO SHIP   |   [FAIL] QA CHECK FAILED: <REASON>
```

| TEST_NAME | PASS criteria | Value |
|---|---|---|
| `DISPLAY_INIT` | reset + init sequence เสร็จ | `320x240` |
| `CHIP_ID` | RDID1/2/3 = `0x85 0x85 0x52` (datasheet ST7789VW §9.1.3) | `0x858552` |
| `POWER_MODE` | RDDPM (0Ah) = `0x9C` (booster on, sleep out, normal, display on) | hex |
| `PIXEL_FORMAT` | RDDCOLMOD (0Ch) = `0x55` (16-bit) | hex |
| `SELF_DIAG` | RDDSDR (0Fh) bit7 + bit6 = `0xC0` | hex |
| `FRAME_MEMORY` | เขียน 16 pixel อ่านกลับตรงทุกจุด | `n/16` |
| `AUTHENTICITY` | `isGenuine()` | `GENUINE` / `SUSPECT` |
| `TOUCH_I2C` | CST816 ACK ที่ 0x15 (รุ่น Touch) | `0x15` |
| `TOUCH_ID` | CHIP_ID (0xA7) ไม่ใช่ 0x00 / 0xFF | hex |
| `SD_MOUNT` | mount การ์ดสำเร็จ (รุ่น SD) | ขนาด MB |
| `SD_RW` | เขียน + อ่านไฟล์ทดสอบกลับได้ | `OK` |
| `CONTINUOUS` | 20 รอบ: ID ตรง + GRAM readback ตรง ทุกรอบ | `n/20` |

พิมพ์ `r` + Enter เพื่อทดสอบซ้ำโดยไม่ต้อง reset · หน้าจอจะแสดงแถบสี 8 สี, กรอบขอบจอ, ข้อความไทย และ PASS / FAIL ให้พนักงานตรวจด้วยตา

## Expected Report (real passing board)

<!-- TODO: [MASSMORE_INPUT_REQUIRED: paste the verbatim Serial output of a real board that printed #VERDICT PASS] -->

```text
Massmore_ST7789 - 07_Factory_Test

#MASSMORE_FACTORY_TEST v1.0
#PRODUCT Massmore_ST7789
#MCU ESP32
Library v1.0.0
Massmore_ST7789 v1.0.0 (TFT_eSPI 2.5.43)
  MCU        : ESP32 Classic (VSPI)
  Panel      : 240x320 (2.0")
  ...
#RESULT DISPLAY_INIT PASS 320x240
#RESULT CHIP_ID PASS 0x858552
#RESULT POWER_MODE PASS 0x9C
#RESULT PIXEL_FORMAT PASS 0x55
#RESULT SELF_DIAG PASS 0xC0
#RESULT FRAME_MEMORY PASS 16/16
#RESULT AUTHENTICITY PASS GENUINE
#RESULT TOUCH_I2C PASS 0x15
#RESULT TOUCH_ID PASS 0xB6 (CST816 family)
#RESULT SD_MOUNT PASS 7580MB
#RESULT SD_RW PASS OK
#RESULT CONTINUOUS PASS 20/20

#VERDICT PASS
[PASS] SENSOR QA PASSED - READY TO SHIP
Type 'r' + Enter to run the test again.
```

> ตัวอย่างข้างบนคือรูปแบบที่คาดหวัง (template) — ค่าจริงจะถูกแทนที่หลังทดสอบกับบอร์ดจริงตาม Massmore Physical Testing Protocol

## Rebuild

```bash
cd PlatformIO
pio run -e esp32dev -e esp32-s3-devkitc-1
cp .pio/build/esp32dev/firmware.factory.bin            ../firmware/bin/Massmore_ST7789_FactoryTest_ESP32.bin
cp .pio/build/esp32dev/firmware.bin                    ../firmware/bin/Massmore_ST7789_FactoryTest_ESP32_app.bin
cp .pio/build/esp32-s3-devkitc-1/firmware.factory.bin  ../firmware/bin/Massmore_ST7789_FactoryTest_ESP32S3.bin
cp .pio/build/esp32-s3-devkitc-1/firmware.bin          ../firmware/bin/Massmore_ST7789_FactoryTest_ESP32S3_app.bin
```
