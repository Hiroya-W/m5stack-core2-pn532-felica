#include "Map.h"

Map::Map(LGFX *display,  int w, int h, unsigned char (*bmp)[2048]) {
    tft = display;
    width = w;
    height = h;
    image = bmp;
}

void Map::setMapData(uint8_t (*m)[10]) {
    map = m;
}

uint32_t Map::drawEntireMap() {
    uint32_t startTime = millis();
    drawMap(0, 0, width, height);
    return millis() - startTime;
}

uint32_t Map::drawMap(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint32_t startTime = millis();
    if (x + w > width || y + h > height) return 0;

    tft->startWrite();
    for (int i = y; i < y + h; i++) {
        for (int j = x; j < x + w; j++) {
            _drawBmp(tft,(unsigned char *)image[map[i][j]], j * 32, i * 32, 32, 32);
        }
    }
    tft->endWrite();
    return millis() - startTime;
}

uint32_t Map::drawSpriteMap(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint32_t startTime = millis();
    if (x + w > width || y + h > height) return 0;
    LGFX_Sprite *sp = new LGFX_Sprite(tft);
    sp->createSprite(w*32,h*32);
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            _drawBmp(sp,(unsigned char *)image[map[i+y][j+x]], j * 32, i * 32, 32, 32);
        }
    }
    sp->pushSprite(x*32,y*32);
    sp->deleteSprite();
    return millis() - startTime;
}

uint32_t Map::_drawBmp(LGFX *device, unsigned char *data, int16_t x, int16_t y, int16_t w,
                      int16_t h) {
    uint16_t row, col, buffidx = 0;
    device->startWrite();
    for (col = 0; col < w; col++) {  // For each scanline...
        for (row = 0; row < h; row++) {
            uint16_t c = pgm_read_word(data + buffidx);
            c = ((c >> 8) & 0x00ff) | ((c << 8) & 0xff00);  // swap back and fore
            device->drawPixel(col + x, row + y, c);
            buffidx += 2;
        }  // end pixel
    }
    device->endWrite();
    return 0;
}

uint32_t Map::_drawBmp(LGFX_Sprite *device, unsigned char *data, int16_t x, int16_t y, int16_t w,
                      int16_t h) {
    uint16_t row, col, buffidx = 0;
    for (col = 0; col < w; col++) {  // For each scanline...
        for (row = 0; row < h; row++) {
            uint16_t c = pgm_read_word(data + buffidx);
            c = ((c >> 8) & 0x00ff) | ((c << 8) & 0xff00);  // swap back and fore
            device->drawPixel(col + x, row + y, c);
            buffidx += 2;
        }  // end pixel
    }
    return 0;
}

int Map::update() { 
    return 1;
}