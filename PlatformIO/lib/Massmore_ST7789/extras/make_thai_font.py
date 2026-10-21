#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
make_thai_font.py — Massmore_ST7789 Thai Smooth Font generator
--------------------------------------------------------------------------
สร้างไฟล์ header (C array ในรูปแบบ .vlw ของ Processing / TFT_eSPI Smooth Font)
จากไฟล์ TrueType/OpenType เพื่อใช้กับ Massmore_ST7789::loadFont() / drawThaiString()

  python3 make_thai_font.py <font.ttf> <size_px> <ArrayName> <out.h> [--license "..."]

ต้องมี Pillow:  python3 -m pip install pillow

สิ่งที่สคริปต์ทำเพิ่มจากตัวสร้างฟอนต์ทั่วไป (จำเป็นสำหรับภาษาไทย)
  * ใส่ glyph ทั้ง ASCII (0x21-0x7E), ° µ, และอักษรไทยทั้งหมด U+0E01-U+0E5B
  * สร้าง "glyph สำรอง" ใน Private Use Area (U+F700-U+F71A) สำหรับ
      - วรรณยุกต์ที่ต้องยกสูงเมื่ออยู่บนสระบน (เช่น ที่ นี้ น้ำ)
      - วรรณยุกต์ / สระบนที่ต้องเลื่อนซ้ายเมื่ออยู่บน ป ฝ ฟ (หางยาวด้านขวา)
      - สระล่างที่ต้องเลื่อนลงเมื่ออยู่ใต้ ญ ฐ (มีหางด้านล่าง)
    Massmore_ST7789::shapeThai() จะเลือก glyph เหล่านี้ให้อัตโนมัติขณะวาดข้อความ
  * คำนวณ ascent ของ header ให้ครอบคลุมวรรณยุกต์ที่ยกสูง เพื่อไม่ให้ทับบรรทัดบน

VLW format (big-endian int32):
  header  : gCount, version(11), fontSize, mboxY(0), ascent, descent
  per glyph: unicode, height, width, xAdvance, dY(top above baseline), dX(left of cursor), 0
  bitmaps  : 8-bit alpha, width*height bytes per glyph, same order
"""
import struct
import sys
import argparse
from PIL import Image, ImageDraw, ImageFont

# ---- Thai code point classes ---------------------------------------------------------
TONE   = [0x0E48, 0x0E49, 0x0E4A, 0x0E4B, 0x0E4C]                 # ่ ้ ๊ ๋ ์
UPPER  = [0x0E31, 0x0E34, 0x0E35, 0x0E36, 0x0E37, 0x0E47, 0x0E4D, 0x0E4E]  # ั ิ ี ึ ื ็ ํ ๎
LOWER  = [0x0E38, 0x0E39, 0x0E3A]                                 # ุ ู ฺ
TALL   = [0x0E1B, 0x0E1D, 0x0E1F]                                 # ป ฝ ฟ
DESC   = [0x0E0D, 0x0E10]                                         # ญ ฐ

# ---- Private Use Area scheme (must match Massmore_ST7789.cpp shapeThai()) --------------
PUA_TONE_LEFT        = 0xF700   # +k  : tone mark shifted left      (on ป ฝ ฟ)
PUA_TONE_RAISED      = 0xF705   # +k  : tone mark raised            (after upper vowel / before ำ)
PUA_TONE_RAISED_LEFT = 0xF70A   # +k  : raised + shifted left       (ป ฝ ฟ + upper vowel)
PUA_UPPER_LEFT       = 0xF710   # +j  : upper vowel shifted left    (on ป ฝ ฟ)
PUA_LOWER_DOWN       = 0xF718   # +m  : lower vowel shifted down    (under ญ ฐ)


def ink_box(font, ch, size):
    """Render one code point and return (x0, y0, x1, y1) of the ink relative to the
    pen origin (x right, y down, baseline = 0), plus the alpha bitmap. None if blank."""
    pad = size * 2
    W = H = size * 4
    im = Image.new("L", (W, H), 0)
    ImageDraw.Draw(im).text((pad, pad), ch, font=font, fill=255, anchor="ls")
    bb = im.getbbox()
    if bb is None:
        return None, None
    x0, y0, x1, y1 = bb
    return (x0 - pad, y0 - pad, x1 - pad, y1 - pad), im.crop(bb)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("ttf")
    ap.add_argument("size", type=int)
    ap.add_argument("name")
    ap.add_argument("out")
    ap.add_argument("--license", default="")
    ap.add_argument("--no-thai", action="store_true", help="ASCII only")
    a = ap.parse_args()

    size = a.size
    font = ImageFont.truetype(a.ttf, size)
    gap = max(1, round(size * 0.04))

    cps = list(range(0x21, 0x7F)) + [0x00B0, 0x00B5]
    if not a.no_thai:
        cps += list(range(0x0E01, 0x0E3B)) + list(range(0x0E3F, 0x0E5C))

    glyphs = {}   # cp -> dict(w,h,adv,dx,dy,bitmap)
    for cp in cps:
        ch = chr(cp)
        box, bmp = ink_box(font, ch, size)
        adv = int(round(font.getlength(ch)))
        if box is None:
            continue
        x0, y0, x1, y1 = box
        glyphs[cp] = dict(w=x1 - x0, h=y1 - y0, adv=adv, dx=x0, dy=-y0, bmp=bmp, x1=x1, y1=y1, y0=y0)

    extra = {}
    if not a.no_thai:
        # ---- measurements for the PUA variants ---------------------------------------
        upper_top = min(glyphs[c]["y0"] for c in UPPER if c in glyphs)          # most negative
        tall_asc_x0 = None
        tall_adv = 0
        cons_top = glyphs[0x0E01]["y0"] if 0x0E01 in glyphs else 0   # top of ก = normal consonant height
        for c in TALL:
            if c not in glyphs:
                continue
            g = glyphs[c]
            bmp = g["bmp"]
            rows_above = cons_top - g["y0"]                # rows the ascender pokes above a normal consonant
            if rows_above < 2:
                continue
            rows = max(1, rows_above - 1)
            top = bmp.crop((0, 0, g["w"], rows)).getbbox()  # only the ascender is in these rows
            if top is None:
                continue
            asc_x0 = g["dx"] + top[0]                     # ascender left edge, relative to consonant origin
            if tall_asc_x0 is None or asc_x0 < tall_asc_x0:
                tall_asc_x0 = asc_x0
            tall_adv = max(tall_adv, g["adv"])
        desc_bottom = max(glyphs[c]["y1"] for c in DESC if c in glyphs)

        def left_shift(g):
            if tall_asc_x0 is None:
                return 0
            right_edge = tall_adv + g["x1"]               # mark right edge inside the consonant cell
            return max(0, right_edge - (tall_asc_x0 - gap))

        for k, cp in enumerate(TONE):
            if cp not in glyphs:
                continue
            g = glyphs[cp]
            raise_by = max(0, g["y1"] - upper_top + gap)
            ls = left_shift(g)
            extra[PUA_TONE_LEFT + k]        = dict(g, dx=g["dx"] - ls)
            extra[PUA_TONE_RAISED + k]      = dict(g, dy=g["dy"] + raise_by)
            extra[PUA_TONE_RAISED_LEFT + k] = dict(g, dx=g["dx"] - ls, dy=g["dy"] + raise_by)
        for j, cp in enumerate(UPPER):
            if cp not in glyphs:
                continue
            g = glyphs[cp]
            extra[PUA_UPPER_LEFT + j] = dict(g, dx=g["dx"] - left_shift(g))
        for m, cp in enumerate(LOWER):
            if cp not in glyphs:
                continue
            g = glyphs[cp]
            down = max(0, desc_bottom + gap - g["y0"])
            extra[PUA_LOWER_DOWN + m] = dict(g, dy=g["dy"] - down)

    allg = dict(glyphs)
    allg.update(extra)
    order = sorted(allg.keys())

    ascent  = max(g["dy"] for g in allg.values())
    descent = max(g["h"] - g["dy"] for g in allg.values())

    out = bytearray()
    out += struct.pack(">6i", len(order), 11, size, 0, ascent, descent)
    for cp in order:
        g = allg[cp]
        out += struct.pack(">7i", cp, g["h"], g["w"], g["adv"], g["dy"], g["dx"], 0)
    for cp in order:
        out += allg[cp]["bmp"].tobytes()
    # trailer (not used by TFT_eSPI, kept for .vlw compatibility)
    nm = a.name.encode()
    out += bytes([len(nm)]) + nm + b"\0" + bytes([len(nm)]) + nm + b"\0" + b"\1"

    with open(a.out, "w", encoding="utf-8") as f:
        f.write("// %s — generated by extras/make_thai_font.py (Massmore_ST7789)\n" % a.name)
        f.write("// Source font: %s @ %d px — %d glyphs (ASCII + Thai + %d Thai shaping variants) — %d bytes\n"
                % (a.ttf.split("/")[-1], size, len(order), len(extra), len(out)))
        if a.license:
            f.write("// Licence: %s\n" % a.license)
        f.write("// Usage:  tft.loadFont(%s);  tft.drawThaiString(\"สวัสดี\", x, y);  tft.unloadFont();\n" % a.name)
        f.write("#ifndef MASSMORE_FONT_%s_H\n#define MASSMORE_FONT_%s_H\n#include <Arduino.h>\n\n" % (a.name.upper(), a.name.upper()))
        f.write("const uint8_t %s[] PROGMEM = {\n" % a.name)
        for i in range(0, len(out), 16):
            f.write("  " + ", ".join("0x%02X" % b for b in out[i:i + 16]) + ",\n")
        f.write("};\n\n#endif\n")
    print("%-28s %3d px  glyphs=%3d  ascent=%2d descent=%2d  bytes=%6d" % (a.name, size, len(order), ascent, descent, len(out)))


if __name__ == "__main__":
    main()
