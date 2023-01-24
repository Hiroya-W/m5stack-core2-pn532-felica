#ifndef CHARACTER_H
#define CHARACTER_H

/* Character class 
   アニメーションするキャラクタを描画するクラス．

*/

#include "Arduino.h"
#include "bitmaps.h"
//#include "util.h"
#include <pgmspace.h>
//#include <Adafruit_GFX.h>
//#include <Arduino_ST7789.h>
#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>
//#include <M5Core2.h>
#include "Map.h"
//#define LGFX M5Display
//#define LGFX_Sprite TFT_eSprite

//#include "DSEG7Classic-Bold18pt7b.h"
//#include <Fonts/FreeSansBold12pt7b.h>
//#include <Fonts/FreeSansBold18pt7b.h>

// 20 frames per second
#define FPS 24
#define SIZE_QUEUE 30

#define HOME_X 120-32
#define HOME_Y 120-32

#define TIME_BACKHOME 10000
#define TIME_SLEEP    60000
#define TIME_TFTSLEEP 60000

//#define BLACK TFT_BLACK
//#define WHITE TFT_WHITE
#define SCREEN_BGCOLOR TFT_NAVY

void InitI2SSpeakerOrMic(int mode);
void playSound(int type);

enum {
  ORIENT_FRONT = 0,
  ORIENT_LEFT,
  ORIENT_RIGHT,
  ORIENT_BACK,
  ORIENT_JUMP,
  ORIENT_SLEEP
};
enum { STATUS_WAIT = 0, STATUS_STOP, STATUS_MOVE, STATUS_TOUCH };
enum {
  MOVE_UP = 0,
  MOVE_LEFT,
  MOVE_RIGHT,
  MOVE_DOWN,
  MOVE_LEFTBACK,
  MOVE_RIGHTBACK
};

class Character {
public:
//  Character(Arduino_ST7789 *display, unsigned char (*bmp)[4][2048]);
  Character(LGFX *display, unsigned char (*bmp)[4][2048]);
  void start(uint8_t o);
  void start(uint16_t x, uint16_t y, uint8_t o);
  void stop(uint8_t o);
  void sleep();
  void setMap(Map *m);
  int16_t current_x();
  int16_t current_y();
  int current_width();
  int current_height();
  void setSpeed(uint8_t s);
  void incSpeed();
  void decSpeed();
  void setOrient(uint8_t o);
  uint32_t getOrientTimer();
  //void setOrientByAccel(int32_t ax);
  void moveDist(uint8_t d, int16_t dist);
  void moveTo(uint16_t to_x, uint16_t to_y);
  void queueMoveTo(uint16_t to_x, uint16_t to_y, uint16_t s = 4, uint16_t d = 1);
  void queueAction(uint8_t s, uint16_t p1, uint16_t p2, uint16_t p3 = 0, uint16_t p4 = 0);
//  void queueLight(uint16_t light);
  void dequeueMoveTo();
  int dequeueAction();
  void clearActionQueue();
  uint8_t isEmptyQueue();
  void setStatus(uint8_t status);
  uint8_t getStatus();
  uint32_t getSleepTime();
  uint32_t update(); // returns millisecs to wait
protected:
  LGFX *tft;
  LGFX_Sprite *sprite;
  Map *map;
  unsigned char (*image)[4][2048];
  int16_t move_queue[SIZE_QUEUE][5];
  int16_t move_queue_idx;
  int16_t move_queue_last;
  int16_t home_x, home_y;
  int16_t pos_x, pos_y, width, height, target_x, target_y;
  uint8_t status;
  uint8_t speed;   // speed (1-) drawing interval (5 is slow, 1 is fast)
  uint8_t pattern; // character pattern (0-4)
  uint8_t frame;
  uint8_t orient;
  uint8_t direction;
  uint8_t move_diff;
  uint8_t size = 1;
  uint32_t orient_timer[10];
  uint16_t wait_timer;
  uint32_t drawBmpOnSprite(LGFX_Sprite *sp,unsigned char *data, int16_t, int16_t, int16_t, int16_t);
  uint32_t drawSprite(unsigned char *data, uint8_t size = 1);
};

#endif
