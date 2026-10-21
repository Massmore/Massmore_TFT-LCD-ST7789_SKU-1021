# Massmore Library Request

> กรอกแค่นี้ ที่เหลือให้ศึกษาจาก Datasheet เอง — ช่องไหนไม่มีใส่ `-`

**Chip:** ST7789
**Interface:** I2C / SPI / UART?
**Default addr / alt addr:** 
**Board pins / extra (INT, RST, ADDR, Qwiic ...):** SDA=14 / SCL=15
// Generic ESP32S3 setup
#define TFT_MISO 13
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS 17 // Not connected
#define TFT_DC 3
#define TFT_RST 18 // Connect reset to ensure display initialises

**Note:** 
- massmore.shop (บทความสินค้า ใช้แค่รูปการต่อวงจรดึงมาใส่ Folder docs ได้เลย): https://www.massmore.shop/docs/32
- Base on TFT_eSPI Library clone and add font thai 2 font TH_sarabun/Kanit ให้โหลดใช้งานได้เลยไม่ต้องติดตั้ง library อื่นเพิ่ม

**Links**
- massmore.shop: https://www.massmore.shop/products/5e1f85a3-6174-4861-aef3-dca6186013f2
- Shopee: https://shopee.co.th/%E0%B9%82%E0%B8%A1%E0%B8%94%E0%B8%B9%E0%B8%A5%E0%B8%AB%E0%B8%99%E0%B9%89%E0%B8%B2%E0%B8%88%E0%B8%AD-TFT-LCD-ST7789-SPI-1.3-1.54-2.0-inch-Touch-IPS-Display-Module-SDcard-Massmore-i.5641091.49704750884?extraParams=%7B%22display_model_id%22%3A186356705895%2C%22model_selection_logic%22%3A3%7D
- Lazada: https://www.lazada.co.th/products/pdp-i6042050290-s26180353038.html?c=&channelLpJumpArgs=&clickTrackInfo=query%253AST7789%252Bmassmore%253Bnid%253A6042050290%253Bsrc%253ALazadaMainSrp%253Brn%253A4eb8317da9677dd7ebb6e75828de6ad7%253Bregion%253Ath%253Bsku%253A6042050290_TH%253Bprice%253A420%253Bclient%253Adesktop%253Bsupplier_id%253A100453040804%253Bsession_id%253A%253Bbiz_source%253Ahttps%253A%252F%252Fwww.lazada.co.th%252F%253Bslot%253A0%253Butlog_bucket_id%253A470687%253Basc_category_id%253A13032%253Bitem_id%253A6042050290%253Bsku_id%253A26180353038%253Bshop_id%253A4396003%253BtemplateInfo%253A107882_E%2523-1_A3_C%2523&freeshipping=0&fs_ab=2&fuse_fs=&lang=th&location=%E0%B9%80%E0%B8%8A%E0%B8%B5%E0%B8%A2%E0%B8%87%E0%B9%83%E0%B8%AB%E0%B8%A1%E0%B9%88&price=4.2E%202&priceCompare=skuId%3A26180353038%3Bsource%3Alazada-search-voucher%3Bsn%3A4eb8317da9677dd7ebb6e75828de6ad7%3BoriginPrice%3A42000%3BdisplayPrice%3A42000%3BisGray%3Afalse%3BsinglePromotionId%3A-1%3BsingleToolCode%3A-1%3BvoucherPricePlugin%3A0%3Btimestamp%3A1789555514935&ratingscore=5.0&request_id=4eb8317da9677dd7ebb6e75828de6ad7&review=2&sale=5&search=1&source=search&spm=a2o4m.searchlist.list.0&stock=1

**Attached files**
- [ ] https://www.buydisplay.com/download/ic/ST7789.pdf?srsltid=AU7gw4Vgxpbw5n1uosjUHFbFZumYNPlxt4KbiqNyiZDnNsuVbUAZ36uz
Datasheet (.pdf)

- [ ] /Users/wooddy/Documents/PlatformIO/Projects/Massmore_BNO08x_SKU-1010/docs  (รูปสินค้าหลัก) (ผังขา) (ตัวอย่างการต่อ)

