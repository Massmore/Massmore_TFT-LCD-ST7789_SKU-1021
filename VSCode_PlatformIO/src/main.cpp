#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "cst816d.h"

// --- Config Pins (SD card + touch controller) ---
#define SPI_MISO 13
#define SPI_MOSI 11
#define SPI_SCLK 12
#define SD_CS 0

#define TP_INT -1
#define TP_RST -1
#define TP_SDA 14
#define TP_SCL 15

SPIClass fspi(FSPI);
TFT_eSPI tft = TFT_eSPI();

volatile int shared_x = 0;
volatile int shared_y = 0;
volatile bool isTouched = false;
volatile bool updateScreen = false;
// Set true only after tft.init() has run inside TaskDisplayCode. loop() must
// not touch the TFT before this, and in fact never touches it at all now --
// TaskDisplayCode is the single owner of the display (TFT_eSPI is not
// thread-safe, so only one task may drive it).
volatile bool displayReady = false;

uint64_t cardSize;
toucht_coords_t toucht_coords;
uint16_t sd_count = 0;

bool sdMounted = false;
String sdStatus;

TaskHandle_t Task1;

// Display task, pinned to core 1
void TaskDisplayCode(void *pvParameters)
{
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("SD & Touch Test", 10, 10);

    tft.setTextColor(sdMounted ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.drawString(sdStatus, 10, 30);

    tft.drawRect(5, 55, 310, 175, TFT_BLUE);

    displayReady = true; // static UI is up; loop() may start feeding coordinates

    for (;;) {
        if (updateScreen) {
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.setTextSize(2);
            String coords = "X: " + String(shared_x) + "  Y: " + String(shared_y) + "   ";
            tft.drawString(coords, 20, 200);

            if (shared_x > 0 && shared_y > 0) {
                tft.fillCircle(shared_x, shared_y, 5, TFT_RED);
            }

            updateScreen = false;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("ESP32 Multi-Tasking: SD + Touch + TFT");

    touch_init(TP_SDA, TP_SCL, TP_RST, TP_INT);

    // SD and TFT share this SPI bus but tft.init() only runs later, inside
    // TaskDisplayCode. Deselect the TFT chip now so it can't drive MISO
    // while the SD card is being probed, or the mount fails intermittently.
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    fspi.begin(SPI_SCLK, SPI_MISO, SPI_MOSI, -1);
    sdMounted = SD.begin(SD_CS, fspi);
    if (!sdMounted) {
        Serial.println("Card Mount Failed");
        sdStatus = "SD: Mount Failed";
    } else {
        cardSize = SD.cardSize() / (1024 * 1024);
        Serial.printf("SD Card Size: %lluMB\n", cardSize);
        sdStatus = "SD: " + String((unsigned long)cardSize) + "MB";
    }

    xTaskCreatePinnedToCore(
        TaskDisplayCode,
        "DisplayTask",
        10000,
        NULL,
        1,
        &Task1,
        1);
}

void loop()
{
    // Wait until TaskDisplayCode has initialised the panel before doing anything.
    if (!displayReady) {
        delay(10);
        return;
    }

    if (get_touch_coords(&toucht_coords)) {
        // Map touch axes to the screen orientation used by setRotation(3)
        int draw_x = map(toucht_coords.y, 300, 0, 0, 320);
        int draw_y = map(toucht_coords.x, 200, 0, 0, 240);

        draw_x = constrain(draw_x, 0, 320);
        draw_y = constrain(draw_y, 0, 240);

        Serial.print("Raw[x,y]: ");
        Serial.print(toucht_coords.x);
        Serial.print(",");
        Serial.print(toucht_coords.y);
        Serial.print("  ->  Screen[x,y]: ");
        Serial.print(draw_x);
        Serial.print(",");
        Serial.println(draw_y);

        shared_x = draw_x;
        shared_y = draw_y;
        isTouched = true;
        updateScreen = true;
        // NOTE: do NOT draw to tft here. TaskDisplayCode renders the touch
        // point from shared_x/shared_y so the display has a single owner.
    }

    if (sd_count++ > 200) {
        sd_count = 0;
        // tft.fillScreen(TFT_BLACK); // uncomment to periodically clear the screen
    }
    delay(10);
}
