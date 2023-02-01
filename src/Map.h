#ifndef MAP_H
#define MAP_H

#include "Arduino.h"
#include "bitmaps.h"
#include <pgmspace.h>

#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>

class Map {
 public:
  Map(LGFX *display, int width, int height, unsigned char (*bmp)[2048]);
  void setMapData(uint8_t (*m)[10]);
  uint32_t drawEntireMap();
  uint32_t drawMap(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
  uint32_t drawSpriteMap(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
  int update();
 private:
  LGFX *tft;
  uint8_t (*map)[10];
  int width;
  int height;
  unsigned char (*image)[2048];
  uint32_t _drawBmp(LGFX *dev, unsigned char *data, int16_t x, int16_t y, int16_t w, int16_t h);
  uint32_t _drawBmp(LGFX_Sprite *dev, unsigned char *data, int16_t x, int16_t y, int16_t w, int16_t h);
};

#endif