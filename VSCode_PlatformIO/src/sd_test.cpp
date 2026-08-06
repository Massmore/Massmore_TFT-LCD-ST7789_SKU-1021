#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// --- Pins per ESP32S3_LCD2_Touch board (same as main.cpp) ---
#define SPI_MISO 13
#define SPI_MOSI 11
#define SPI_SCLK 12
#define SD_CS    0

// TFT shares this same physical SPI bus (MISO13/MOSI11/SCLK12) via its own
// CS on pin 17 (see TFT_CS in platformio.ini build_flags). In the combined
// sketch SD.begin() runs before tft.init(), so TFT_CS is left floating
// while the SD card is probed -- the ST7789 can then drive MISO at the
// same time as the card and corrupt the init handshake. Deselect it here
// to isolate whether that is the cause.
#define TFT_CS_PIN 17

SPIClass fspi(FSPI);

void listDir(File dir, int depth = 0)
{
    while (File entry = dir.openNextFile()) {
        for (int i = 0; i < depth; i++) Serial.print("  ");
        if (entry.isDirectory()) {
            Serial.printf("[DIR]  %s\n", entry.name());
            listDir(entry, depth + 1);
        } else {
            Serial.printf("       %-24s %8u bytes\n", entry.name(), (unsigned)entry.size());
        }
        entry.close();
    }
}

bool tryMount(uint32_t freqHz)
{
    Serial.printf("SD.begin() at %lu Hz... ", (unsigned long)freqHz);
    if (SD.begin(SD_CS, fspi, freqHz)) {
        Serial.println("OK");
        return true;
    }
    Serial.println("failed");
    SD.end();
    return false;
}

void setup()
{
    Serial.begin(115200);
    delay(1500); // give USB-serial time to enumerate
    Serial.println("\n=== SD Card Standalone Test (ESP32S3_LCD2_Touch) ===");
    Serial.printf("MISO=%d  MOSI=%d  SCLK=%d  SD_CS=%d\n", SPI_MISO, SPI_MOSI, SPI_SCLK, SD_CS);

    // Deselect the TFT chip so it can't contend on the shared SPI bus.
    pinMode(TFT_CS_PIN, OUTPUT);
    digitalWrite(TFT_CS_PIN, HIGH);
    Serial.printf("Held TFT_CS (pin %d) HIGH to keep it off the bus.\n", TFT_CS_PIN);

    // Deselect the card too before bringing the bus up.
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    fspi.begin(SPI_SCLK, SPI_MISO, SPI_MOSI, -1);

    bool mounted = tryMount(4000000);
    if (!mounted) mounted = tryMount(1000000);
    if (!mounted) mounted = tryMount(400000); // SD spec init speed

    if (!mounted) {
        Serial.println("\nCard Mount Failed at all speeds.");
        Serial.println("Things to check next:");
        Serial.println("  - Card is FAT32-formatted (SD library requires FAT/FAT32, not exFAT)");
        Serial.println("  - 3.3V at the card's VDD pin under load (some modules brown out under SD current spikes)");
        Serial.println("  - Continuity/solder joints on MISO/MOSI/SCLK/CS traces to the card holder");
        Serial.println("  - GPIO0 is a boot-strap pin; confirm the card module doesn't pull it low at reset");
        Serial.println("  - Try a different SD card (some cards are picky about init timing)");
        return;
    }

    Serial.println("\nCard mounted OK.");

    uint8_t cardType = SD.cardType();
    Serial.print("Card Type: ");
    switch (cardType) {
        case CARD_MMC:  Serial.println("MMC");  break;
        case CARD_SD:   Serial.println("SDSC"); break;
        case CARD_SDHC: Serial.println("SDHC"); break;
        default:        Serial.println("UNKNOWN"); break;
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("Card Size: %lluMB\n", cardSize);
    Serial.printf("Total: %lluMB  Used: %lluMB\n",
                   SD.totalBytes() / (1024 * 1024), SD.usedBytes() / (1024 * 1024));

    Serial.println("\nRoot directory listing:");
    File root = SD.open("/");
    listDir(root);
    root.close();

    Serial.println("\nWrite/read smoke test:");
    File f = SD.open("/sd_test.txt", FILE_WRITE);
    if (f) {
        f.println("hello from sd_test");
        f.close();
        Serial.println("Wrote /sd_test.txt");
    } else {
        Serial.println("Failed to open /sd_test.txt for write");
    }

    f = SD.open("/sd_test.txt");
    if (f) {
        Serial.print("Read back: ");
        while (f.available()) Serial.write(f.read());
        f.close();
    } else {
        Serial.println("Failed to open /sd_test.txt for read");
    }
}

void loop()
{
    delay(1000);
}
