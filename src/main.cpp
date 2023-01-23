#include <M5Core2.h>

#define LGFX_M5STACK_CORE2
#define LGFX_USE_V1

#include <SPI.h>
// #include <Wire.h>

#include <PN532/PN532/PN532.h>
#include <PN532/PN532/PN532_debug.h>
// #include <PN532/PN532_SPI/PN532_SPI.h>
#include <PN532/PN532_I2C/PN532_I2C.h>

/* 配線図
  PN532      M5Stack Core2

   SDA        32
   SCL        33
   VCC        3.3V
   GND        GND
   IRQ        19
*/

/* I2Cは sda 32 scl 33 に接続．
   m5stack core2のデフォルトで Wire は 32, 33 が利用される．

    通信速度を50kHzにしないと失敗する．
    Wire.setClock(50000UL);

*/

#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager

#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>

/*
#include "Map.h"
*/
#include "Character.h"
#include "Map.h"
#include "bitmaps.h"
#include "sound.h"

#define CONFIG_I2S_BCK_PIN 12
#define CONFIG_I2S_LRCK_PIN 0
#define CONFIG_I2S_DATA_PIN 2
#define CONFIG_I2S_DATA_IN_PIN 34

#define SPEAKER_I2S_NUMBER I2S_NUM_0

#define MODE_MIC 0
#define MODE_SPK 1

#define PIN_SPI_SS (27)
#define PIN_PN532_IRQ (19)

// static LGFX *lcd = &(M5.Lcd);
static LGFX *lcd = new LGFX();

// PN532_SPI pn532spi(SPI, PIN_SPI_SS);
// PN532 nfc(pn532spi);
PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

uint8_t _prevIDm[8];
unsigned long _prevTime = 0;
unsigned long _prev_keyopen_time = 0;

volatile boolean irq = false;
Character aquatan = Character(lcd, (unsigned char (*)[4][2048])aqua_bmp);
Map bg = Map(lcd, 10, 8, (unsigned char (*)[2048])bgimg);

uint8_t bgmap[8][10] = {
    {14, 15, 15, 15, 15, 15, 15, 15, 15, 15},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {5, 6, 6, 7, 1, 1, 5, 6, 6, 7},
    {4, 4, 4, 4, 0, 0, 4, 4, 4, 4},
    {12, 12, 9, 10, 13, 13, 11, 8, 12, 12},
    {3, 3, 3, 10, 13, 13, 11, 3, 3, 3},
    {3, 3, 3, 10, 13, 13, 11, 3, 3, 3},
    {3, 3, 3, 8, 12, 12, 9, 3, 3, 3}};

// 文字背景色 F3EBD6

HTTPClient http;
WebServer webServer(80);
Preferences prefs;
WiFiManager wifimanager;

/*
Map background = Map(&lcd, &aquatan, (unsigned char (*)[2048])bgimg);
int aquatan_speed = 8;
*/
const unsigned char *wav_list[] = {macstartup, se_pi, se_pu};
const size_t wav_size[] = {sizeof(macstartup), sizeof(se_pi), sizeof(se_pu)};

void InitI2SSpeakerOrMic(int mode) {
    esp_err_t err = ESP_OK;

    i2s_driver_uninstall(SPEAKER_I2S_NUMBER);
    i2s_config_t i2s_config = {.mode = (i2s_mode_t)(I2S_MODE_MASTER),
                               .sample_rate = 16000,
                               .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
                               .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
                               .communication_format = I2S_COMM_FORMAT_STAND_I2S,
                               .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
                               .dma_buf_count = 6,
                               .dma_buf_len = 60,
                               .use_apll = false,
                               .tx_desc_auto_clear = true,
                               .fixed_mclk = 0};
    if (mode == MODE_MIC) {
        i2s_config.mode =
            (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    } else {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    }

    err += i2s_driver_install(SPEAKER_I2S_NUMBER, &i2s_config, 0, NULL);

    i2s_pin_config_t tx_pin_config = {
        .bck_io_num = CONFIG_I2S_BCK_PIN,
        .ws_io_num = CONFIG_I2S_LRCK_PIN,
        .data_out_num = CONFIG_I2S_DATA_PIN,
        .data_in_num = CONFIG_I2S_DATA_IN_PIN,
    };
    err += i2s_set_pin(SPEAKER_I2S_NUMBER, &tx_pin_config);

    if (mode != MODE_MIC) {
        err += i2s_set_clk(SPEAKER_I2S_NUMBER, 16000, I2S_BITS_PER_SAMPLE_16BIT,
                           I2S_CHANNEL_MONO);
    }

    i2s_zero_dma_buffer(SPEAKER_I2S_NUMBER);
}

void playSound(int type) {
    size_t bytes_written;
    M5.Axp.SetSpkEnable(true);
    InitI2SSpeakerOrMic(MODE_SPK);

    // Write Speaker
    i2s_write(SPEAKER_I2S_NUMBER, wav_list[type], wav_size[type], &bytes_written,
              portMAX_DELAY);
    i2s_zero_dma_buffer(SPEAKER_I2S_NUMBER);

    // Set Mic Mode
    InitI2SSpeakerOrMic(MODE_MIC);
    M5.Axp.SetSpkEnable(false);
}

uint32_t drawBitmap16(unsigned char *data, int16_t x, int16_t y, int16_t w,
                      int16_t h) {
    uint16_t row, col, buffidx = 0;
    lcd->startWrite();
    for (col = 0; col < w; col++) {  // For each scanline...
        for (row = 0; row < h; row++) {
            uint16_t c = pgm_read_word(data + buffidx);
            c = ((c >> 8) & 0x00ff) | ((c << 8) & 0xff00);  // swap back and fore
            lcd->drawPixel(col + x, row + y, c);
            buffidx += 2;
        }  // end pixel
    }
    lcd->endWrite();
    return 0;
}

void PrintHex8(const uint8_t d) {
    Serial.print(" ");
    Serial.print((d >> 4) & 0x0F, HEX);
    Serial.print(d & 0x0F, HEX);
}

// 指定した範囲のmapを描画する．
void drawMap(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    lcd->startWrite();
    for (int i = y; i < y + h; i++) {
        for (int j = x; j < x + w; j++) {
            drawBitmap16((unsigned char *)bgimg[bgmap[i][j]], j * 32, i * 32, 32, 32);
        }
    }
    lcd->endWrite();
}

void messageBox(int x, int y, String message) {
    LGFX_Sprite *img;
    img = new LGFX_Sprite(lcd);
    img->setColorDepth(16);
    img->createSprite(320, 64);
    img->fillSprite(TFT_TRANSPARENT);

    img->fillRoundRect(0, 0, 320, 64, 5, TFT_NAVY);
    img->drawRoundRect(0, 0, 320, 64, 5, TFT_WHITE);
    img->drawRoundRect(1, 1, 318, 62, 5, TFT_WHITE);
    img->setFont(&fonts::Font4);
    img->setTextColor(TFT_WHITE);

    img->drawString(message, img->width() / 2 - img->textWidth(message) / 2, 2);

    img->pushSprite(x, y, TFT_TRANSPARENT);
    img->deleteSprite();
}


void cardreading() {
    Serial.println("INT");
    //    if ((millis() - _prevTime) > 5000) {
    irq = true;
    //    }
}

void setup(void) {
    // put your setup code here, to run once:
    pinMode(PIN_PN532_IRQ, INPUT_PULLUP);
    M5.begin();
    //    Wire1.begin(21,22);
    prefs.begin("aquatan", false);

    lcd->init();
    lcd->setFont(&fonts::Font2);
    lcd->setBrightness(128);
    lcd->setCursor(10, 0);
    //    lcd->println("Hello World");

    bg.setMapData(bgmap);
    //    drawBitmap16((unsigned char *)bgimg[0], 100, 100, 32, 32);
    // drawMap(0,0,10,8);
    bg.drawEntireMap();


    aquatan.start(160 - 32, 120 + 64, ORIENT_FRONT);
    aquatan.setMap(&bg);

    Serial.begin(115200);
    Serial.println("Hello!");
    nfc.begin();
    Wire.setClock(50000UL);

    messageBox(0,0,"SSID: nfclock");

    wifimanager.autoConnect("nfclock");
    WiFi.setSleep(false);

    bg.drawMap(0,0,10,2);

    ArduinoOTA.setHostname("nfclock1");
    ArduinoOTA
        .onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else  // U_SPIFFS
                type = "filesystem";
            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS
            // using SPIFFS.end()
            Serial.println("Start updating " + type);
        })
        .onEnd([]() {
            Serial.println("\nEnd");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR)
                Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR)
                Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR)
                Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR)
                Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR)
                Serial.println("End Failed");
        });
    ArduinoOTA.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.print("Didn't find PN53x board");
        lcd->setCursor(10, 0);
        lcd->println("Didn't find PN53x board");
        while (1) {
            delay(10);
        };  // halt
    }

    //    lcd->setCursor(100, 0);
    //    lcd->println("Hello World2");

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
    uint16_t systemCode = 0xFE00;
    uint8_t requestCode = 0x01;  // System Code request
    uint8_t idm[8];
    uint8_t pmm[8];
    uint16_t systemCodeResponse;
    nfc.felica_WaitingCard(systemCode, requestCode, idm, pmm, &systemCodeResponse);
    attachInterrupt(PIN_PN532_IRQ, cardreading, FALLING);

    InitI2SSpeakerOrMic(MODE_MIC);
}

uint32_t move_prev_millis = 0;
boolean touch = false;

void loop(void) {
    int8_t ret;
    uint16_t systemCode = 0xFE00;
    uint8_t requestCode = 0x01;  // System Code request
    uint8_t idm[8];
    uint8_t pmm[8];
    uint16_t systemCodeResponse;

    ArduinoOTA.handle();

    TouchPoint_t pos = M5.Touch.getPressPoint();
    if (pos.x != -1) {
        if (!touch) {
            touch = true;
            if (pos.x >= 0 && pos.x < 320 && pos.y < 240 && pos.y > 96) {
                int to_x = pos.x - pos.x % 32;
                int to_y = pos.y - pos.y % 32;
                aquatan.queueMoveTo(to_x, to_y, 4, 4);
                playSound(1);
            }
        }
    } else {
        if (touch) {
            touch = false;
        }
    }

    if (irq) {
        // Wait for an FeliCa type cards.
        // When one is found, some basic information such as IDm, PMm, and System Code are retrieved.
        Serial.print("Waiting for an FeliCa card...  ");
        ret = nfc.felica_ReadingCard(systemCode, requestCode, idm, pmm, &systemCodeResponse);

        if (ret != 1) {
            Serial.println("Could not find a card");
            delay(1);
            nfc.felica_WaitingCard(systemCode, requestCode, idm, pmm, &systemCodeResponse);
            irq = false;
            return;
        }

        if (memcmp(idm, _prevIDm, 8) == 0) {
            if ((millis() - _prevTime) < 10000) {
                Serial.println("Same card");
                delay(1);
                nfc.felica_WaitingCard(systemCode, requestCode, idm, pmm, &systemCodeResponse);
                irq = false;
                return;
            }
        }

        _prevTime = millis();

        Serial.println("Found a card!");
        Serial.print("  IDm: ");
        nfc.PrintHex(idm, 8);
        Serial.print("  PMm: ");
        nfc.PrintHex(pmm, 8);
        Serial.print("  System Code: ");
        Serial.print(systemCodeResponse, HEX);
        Serial.print("\n");
        //        lcd->setCursor(100, 100);
        //        lcd->println("Hello World");

        memcpy(_prevIDm, idm, 8);

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
            uint32_t student_id = 0;
            String str_student_id = "";
            uint32_t keta = 10000000;
            for (int i = 0; i < 8; i++) {
                student_id += (blockData[0][6 + i] & 0x0F) * keta;
                keta /= 10;
                Serial.printf("%d", blockData[0][6 + i] & 0x0F);
                str_student_id += (char)blockData[0][6 + i] ;
            }
            Serial.println();

            messageBox(0,0,str_student_id);

//            lcd->printf("%08d", student_id);
            playSound(2);

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

            /* student_id が valid かどうかを labapi に問い合わせ */

            /* もし idm が返ってきたら idm も比較して不一致なら NG */

            /* idmが返ってこない場合にはidmを登録 */

            /* student id が valid なら switchbot apiに解錠を送る．*/

            // use switchbot lock api here
            // getlockstate()
            //

            //            playSound(0);
            aquatan.clearActionQueue();
            aquatan.queueMoveTo(128, 96, 2, 4);
            aquatan.queueAction(STATUS_TOUCH, 0, 0);
        }

        // Wait 1 second before continuing
        Serial.println("Card access completed!\n");
        nfc.felica_WaitingCard(systemCode, requestCode, idm, pmm, &systemCodeResponse);
        irq = false;
    }

    if (millis() > move_prev_millis + 2500) {
        move_prev_millis = millis();
        // ランダムに移動 未解錠時 0, 96, (240-64),(240-64) の範囲内
        if (aquatan.isEmptyQueue()) {
            if (random(100) > 50) {
                int dir = random(100);
                int16_t target_x, target_y;
                if (dir < 25) {  // 上
                    target_x = aquatan.current_x();
                    target_y = aquatan.current_y() - 32;
                } else if (dir < 50) {  // 下
                    target_x = aquatan.current_x();
                    target_y = aquatan.current_y() + 32;
                } else if (dir < 75) {  // 左
                    target_x = aquatan.current_x() - 32;
                    target_y = aquatan.current_y();
                } else {  // 右
                    target_x = aquatan.current_x() + 32;
                    target_y = aquatan.current_y();
                }
                aquatan.queueMoveTo(constrain(target_x, 0, 240 - 64),
                                    constrain(target_y, 96, 240 - 64));
            }
        }
    }
    if (aquatan.getStatus() == STATUS_WAIT) {
        aquatan.sleep();
        int retval = aquatan.dequeueAction();
        // 鍵を開けた場合は，扉のmapを床で置き換える
        if (retval == STATUS_TOUCH) {
            bgmap[2][4] = bgmap[2][5] = bgmap[3][4] = bgmap[3][5] = 2;
            bg.drawMap(4, 2, 2, 2);
            _prev_keyopen_time = millis();
        }
    }

    // 一定時間後に扉を復活
    if (_prev_keyopen_time > 0 && millis() - _prev_keyopen_time > 10000) {
        _prev_keyopen_time = 0;
        bgmap[2][4] = bgmap[2][5] = 1;
        bgmap[3][4] = bgmap[3][5] = 0;
        bg.drawMap(4, 2, 2, 2);
        bg.drawMap(0, 0, 10, 2);
    }

    // delay(10);
    int d1 = 0;
    //    d1 += bg.update();
    d1 += aquatan.update();
    //    d1 += background.update();
    int dd = (1000 / FPS) - d1 > 0 ? (1000 / FPS) - d1 : 1;
    delay(dd);
}
