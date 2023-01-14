#include <Arduino.h>
#include <M5Core2.h>

#define LGFX_M5STACK_CORE2
#define LGFX_USE_V1

#include <PN532/PN532/PN532.h>
#include <PN532/PN532/PN532_debug.h>
#include <PN532/PN532_SPI/PN532_SPI.h>
#include <SPI.h>

#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>

static LGFX lcd;

PN532_SPI pn532spi(SPI, 27);
PN532 nfc(pn532spi);
uint8_t _prevIDm[8];
unsigned long _prevTime;

void PrintHex8(const uint8_t d) {
    Serial.print(" ");
    Serial.print((d >> 4) & 0x0F, HEX);
    Serial.print(d & 0x0F, HEX);
}

void setup(void) {
    // put your setup code here, to run once:
    M5.begin();
    lcd.init();
    lcd.setFont(&fonts::lgfxJapanGothic_40);
    lcd.setBrightness(128);
    lcd.setCursor(0, 0);
    lcd.println("Hello World");

    Serial.begin(115200);
    Serial.println("Hello!");

    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.print("Didn't find PN53x board");
        while (1) {
            delay(10);
        };  // halt
    }

    // Got ok data, print it out!
    Serial.print("Found chip PN5");
    Serial.println((versiondata >> 24) & 0xFF, HEX);
    Serial.print("Firmware ver. ");
    Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print('.');
    Serial.println((versiondata >> 8) & 0xFF, DEC);

    // Set the max number of retry attempts to read from a card
    // This prevents us from waiting forever for a card, which is
    // the default behaviour of the PN532.
    nfc.setPassiveActivationRetries(0xFF);
    nfc.SAMConfig();

    memset(_prevIDm, 0, 8);
}

void loop(void) {
    int8_t ret;
    uint16_t systemCode = 0xFE00;
    uint8_t requestCode = 0x01;  // System Code request
    uint8_t idm[8];
    uint8_t pmm[8];
    uint16_t systemCodeResponse;

    // Wait for an FeliCa type cards.
    // When one is found, some basic information such as IDm, PMm, and System Code are retrieved.
    Serial.print("Waiting for an FeliCa card...  ");
    ret = nfc.felica_Polling(systemCode, requestCode, idm, pmm, &systemCodeResponse, 5000);

    if (ret != 1) {
        Serial.println("Could not find a card");
        delay(500);
        return;
    }

    if (memcmp(idm, _prevIDm, 8) == 0) {
        if ((millis() - _prevTime) < 3000) {
            Serial.println("Same card");
            delay(500);
            return;
        }
    }

    Serial.println("Found a card!");
    Serial.print("  IDm: ");
    nfc.PrintHex(idm, 8);
    Serial.print("  PMm: ");
    nfc.PrintHex(pmm, 8);
    Serial.print("  System Code: ");
    Serial.print(systemCodeResponse, HEX);
    Serial.print("\n");

    memcpy(_prevIDm, idm, 8);
    _prevTime = millis();

    uint8_t blockData[4][16];
    uint16_t serviceCodeList[1];
    uint16_t blockList[4];

    Serial.print("Read Without Encryption command -> ");
    serviceCodeList[0] = 0x1A8B;
    blockList[0] = 0x8000;
    blockList[1] = 0x8001;
    blockList[2] = 0x8002;
    blockList[3] = 0x8003;
    ret = nfc.felica_ReadWithoutEncryption(1, serviceCodeList, 4, blockList, blockData);
    if (ret != 1) {
        Serial.println("error");
        Serial.printf("  Error code: %d\n", ret);
    } else {
        Serial.println("OK!");
        Serial.print("  Student ID: ");
        for (int i = 0; i < 8; i++) {
            Serial.printf("%d", blockData[0][6 + i] & 0x0F);
        }
        Serial.println();
        Serial.print("  Name: ");
        for (int i = 0; i < 16; i++) {
            Serial.printf("%02X ", blockData[1][i]);
        }
        Serial.println();
        Serial.print("  Period of validity: ");
        for (int i = 0; i < 8; i++) {
            Serial.printf("%d", blockData[2][8 + i] & 0x0F);
        }
        Serial.print(" - ");
        for (int i = 0; i < 8; i++) {
            Serial.printf("%d", blockData[3][i] & 0x0F);
        }
        Serial.println();
    }

    // Wait 1 second before continuing
    Serial.println("Card access completed!\n");
    delay(1000);
}
