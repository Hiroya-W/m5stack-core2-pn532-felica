#include "Character.h"
//#include "util.h"

Character::Character(LGFX *display, unsigned char (*bmp)[4][2048]) {
  tft = display;
  sprite = new LGFX_Sprite(tft);
  width = 32;
  height = 32;
  pos_x = home_x = HOME_X;
  pos_y = home_y = HOME_Y;
  pattern = 0;
  frame = 0;
  speed = 4;
  orient = ORIENT_FRONT;
  status = STATUS_STOP;
  move_diff = 1;
  move_queue_idx = 0;
  move_queue_last = 0;
  image = bmp;
  size = 2;
}

void Character::start(uint8_t o) {
  setOrient(o);
  setStatus(STATUS_WAIT);
  drawSprite(image[o][pattern], size);
}

void Character::start(uint16_t x, uint16_t y, uint8_t o) {
  pos_x = x;
  pos_y = y;
  setOrient(o);
  setStatus(STATUS_WAIT);
  drawSprite(image[o][pattern], size);
}

void Character::stop(uint8_t o) {
  setOrient(o);
  setStatus(STATUS_STOP);
}

void Character::sleep() {
  if (getOrientTimer() > TIME_SLEEP) {
    setOrient(ORIENT_SLEEP);
    setSpeed(8);
  }
}

void Character::setMap(Map *m) {
  map = m;
}

int16_t Character::current_x() { return pos_x; }
int16_t Character::current_y() { return pos_y; }

int Character::current_width() { return width; }
int Character::current_height() { return height; }

void Character::setSpeed(uint8_t s) {
  if (s > 0) {
    speed = s;
  }
}

void Character::incSpeed() { speed++; }

void Character::decSpeed() {
  if (speed > 1) {
    speed--;
  }
}

void Character::setOrient(uint8_t o) {
  if (orient != o) {
    orient = o;
    orient_timer[orient] = millis();
  }
}

uint32_t Character::getSleepTime() {
  if (orient == ORIENT_SLEEP) {
    return getOrientTimer();
  } else {
    return 0;
  }
}

uint32_t Character::getOrientTimer() {
  return (millis() - orient_timer[orient]);
}

uint8_t Character::getStatus() { return status; }

void Character::setStatus(uint8_t s) {
  if (status != s) {
    status = s;
  }
}

void Character::moveDist(uint8_t d, int16_t distance) {
  if (status == STATUS_MOVE) {
    return;
  }
  status = STATUS_MOVE;
  direction = d;
  if (direction == MOVE_UP) {
    target_x = pos_x;
    target_y =
        pos_y - distance >= -height * size ? pos_y - distance : -height * size;
  } else if (direction == MOVE_DOWN) {
    target_x = pos_x;
    target_y = pos_y + distance < tft->height() - 1 ? (pos_y + distance)
                                                    : (tft->height() - 1);
  } else if (direction == MOVE_LEFT) {
    target_x =
        pos_x - distance >= -width * size ? pos_x - distance : -width * size;
    target_y = pos_y;
  } else if (direction == MOVE_RIGHT) {
    target_x = pos_x + distance < tft->width() - 1 ? (pos_x + distance)
                                                   : (tft->width() - 1);
    target_y = pos_y;
  } else if (direction == MOVE_LEFTBACK) {
    target_x =
        pos_x - distance >= -width * size ? pos_x - distance : -width * size;
    target_y = pos_y;
  } else if (direction == MOVE_RIGHTBACK) {
    target_x = pos_x + distance < tft->width() - 1 ? (pos_x + distance)
                                                   : (tft->width() - 1);
    target_y = pos_y;
  }
}

void Character::moveTo(uint16_t to_x, uint16_t to_y) {
  if (status == STATUS_MOVE) {
    return;
  }
  status = STATUS_MOVE;
  //  direction = d;
  target_x = to_x >= 0 ? (to_x < tft->width() - width * size
                              ? to_x
                              : (tft->width() - width * size))
                       : 0;
  target_y = to_y >= 0 ? (to_y < tft->height() - height * size
                              ? to_y
                              : (tft->height() - height * size))
                       : 0;
  if (target_x > pos_x) {
    direction = MOVE_RIGHT;
  } else if (target_x < pos_x) {
    direction = MOVE_LEFT;
  } else if (target_y > pos_y) {
    direction = MOVE_DOWN;
  } else if (target_y < pos_y) {
    direction = MOVE_UP;
  }
  /*
  if (direction == MOVE_UP) {
    target_x = to_x;
    target_y = to_y >= 0 ? to_y : 0;
  } else if (direction == MOVE_DOWN) {
    target_x = to_x;
    target_y = to_y < tft->height() - 32 ? to_y : (tft->height() - 32);
  } else if (direction == MOVE_LEFT) {
    target_x = to_x >= 0 ? to_x : 0;
    target_y = to_y;
  } else if (direction == MOVE_RIGHT) {
    target_x = to_x < tft->width() - 32 ? to_x : (tft->width() - 32);
    target_y = to_y;
  } else if (direction == MOVE_LEFTBACK) {
    target_x = to_x >= 0 ? to_x : 0;
    target_y = to_y;
  } else if (direction == MOVE_RIGHTBACK) {
    target_x = to_x < tft->width() - 32 ? to_x : (tft->width() - 32);
    target_y = to_y;
  }
  */
}

void Character::queueAction(uint8_t s, uint16_t p1, uint16_t p2, uint16_t p3,
                            uint16_t p4) {
  move_queue[move_queue_last][0] = s;
  move_queue[move_queue_last][1] = p1;
  move_queue[move_queue_last][2] = p2;
  move_queue[move_queue_last][3] = p3;
  move_queue[move_queue_last][4] = p4;
  move_queue_last++;
  move_queue_last %= SIZE_QUEUE;
}

void Character::queueMoveTo(uint16_t to_x, uint16_t to_y, uint16_t speed,
                            uint16_t hurry) {
  move_queue[move_queue_last][0] = STATUS_MOVE;
  move_queue[move_queue_last][1] = to_x;
  move_queue[move_queue_last][2] = to_y;
  move_queue[move_queue_last][3] = speed;
  move_queue[move_queue_last][4] = hurry;
  move_queue_last++;
  move_queue_last %= SIZE_QUEUE;
}

int Character::dequeueAction() {
  int retval = -1;
  if (move_queue_idx == move_queue_last)
    return -1;
  if (getOrientTimer() < wait_timer) {
    return -1;
  } else {
    wait_timer = 0;
  }
  if (move_queue[move_queue_idx][0] == STATUS_MOVE) {
    setSpeed(move_queue[move_queue_idx][3]);
    move_diff = move_queue[move_queue_idx][4];
    moveTo(move_queue[move_queue_idx][1], move_queue[move_queue_idx][2]);
    //    Serial.println("move:" + String(move_queue[move_queue_idx][1]) + "," +
    //    String(move_queue[move_queue_idx][2]) );
    retval = move_queue[move_queue_idx][0] ;
  } else if (move_queue[move_queue_idx][0] == STATUS_WAIT) {
    setOrient(move_queue[move_queue_idx][1]);
    wait_timer = move_queue[move_queue_idx][2];
    //    Serial.println("wait:" + String(move_queue[move_queue_idx][1]) + "," +
    //    String(move_queue[move_queue_idx][2]) );
    retval = move_queue[move_queue_idx][0] ;
  } else if (move_queue[move_queue_idx][0] == STATUS_TOUCH) {
    setOrient(ORIENT_JUMP);
    wait_timer = 2500;
    setSpeed(4);
    move_diff = 1;
    update();
    retval = move_queue[move_queue_idx][0] ;
  }
  move_queue_idx++;
  move_queue_idx %= SIZE_QUEUE;
  return retval;
}

void Character::dequeueMoveTo() {
  if (move_queue_idx == move_queue_last)
    return;
  moveTo(move_queue[move_queue_idx][0], move_queue[move_queue_idx][1]);
  move_queue_idx++;
  move_queue_idx %= SIZE_QUEUE;
}

void Character::clearActionQueue() { move_queue_idx = move_queue_last = 0; }

uint8_t Character::isEmptyQueue() {
  return (move_queue_idx == move_queue_last);
}

uint32_t Character::update() {
  uint32_t e = 0;
  if (status != STATUS_STOP) {
    if (status == STATUS_MOVE) {
      if (direction == MOVE_UP) {
        //tft->fillRect(pos_x, pos_y + height * size - move_diff, width * size,
        //              move_diff, SCREEN_BGCOLOR);
        pos_y -= move_diff;
        pos_y = pos_y < target_y ? target_y : pos_y;
        setOrient(ORIENT_BACK);
      } else if (direction == MOVE_DOWN) {
        //tft->fillRect(pos_x, pos_y, width * size, move_diff, SCREEN_BGCOLOR);
        pos_y += move_diff;
        pos_y = pos_y > target_y ? target_y : pos_y;
        setOrient(ORIENT_FRONT);
      } else if (direction == MOVE_LEFT) {
        //tft->fillRect(pos_x + width * size - move_diff, pos_y, move_diff,
        //              height * size, SCREEN_BGCOLOR);
        pos_x -= move_diff;
        pos_x = pos_x < target_x ? target_x : pos_x;
        setOrient(ORIENT_LEFT);
      } else if (direction == MOVE_RIGHT) {
        //tft->fillRect(pos_x, pos_y, move_diff, height * size, SCREEN_BGCOLOR);
        pos_x += move_diff;
        pos_x = pos_x > target_x ? target_x : pos_x;
        setOrient(ORIENT_RIGHT);
      } else if (direction == MOVE_LEFTBACK) {
        //tft->fillRect(pos_x + width * size - move_diff, pos_y, move_diff,
        //              height * size, SCREEN_BGCOLOR);
        pos_x -= move_diff;
        pos_x = pos_x < target_x ? target_x : pos_x;
        setOrient(ORIENT_RIGHT);
      } else if (direction == MOVE_RIGHTBACK) {
        //tft->fillRect(pos_x, pos_y, move_diff, height * size, SCREEN_BGCOLOR);
        pos_x += move_diff;
        pos_x = pos_x > target_x ? target_x : pos_x;
        setOrient(ORIENT_LEFT);
      }
      e = drawSprite(image[orient][pattern], size);
      if (frame == 0) {
        pattern++;
        pattern %= 4;
      }
      if (pos_x == target_x) {
        if (pos_y < target_y) {
          direction = MOVE_DOWN;
          setOrient(ORIENT_FRONT);
        } else if (pos_y > target_y) {
          direction = MOVE_UP;
          setOrient(ORIENT_BACK);
        }
      }
      if (pos_y == target_y) {
        if (pos_x < target_x) {
          direction = MOVE_RIGHT;
          setOrient(ORIENT_RIGHT);
        } else if (pos_x > target_x) {
          direction = MOVE_LEFT;
          setOrient(ORIENT_LEFT);
        }
      }
      if (pos_x == target_x && pos_y == target_y) {
        setStatus(STATUS_WAIT);
      }
    } else {
      if (frame == 0) {
        e = drawSprite(image[orient][pattern], size);
        pattern++;
        pattern %= 4;
      }
    }
    frame++;
    frame %= speed;
  }
  //  uint32_t d = (1000 / FPS) - e;
  //  return d > 0 ? d : 0;
  return e;
}

uint32_t Character::drawBmpOnSprite(LGFX_Sprite *sp,unsigned char *data, int16_t x, int16_t y,
                                 int16_t w, int16_t h) {
  uint16_t row, col, buffidx = 0;
  uint32_t startTime = millis();

  for (col = 0; col < w; col++) { // For each scanline...
    for (row = 0; row < h; row++) {
      uint16_t c = pgm_read_word(data + buffidx);
      c = ((c >> 8) & 0x00ff) | ((c << 8) & 0xff00); // swap back and fore
      sp->drawPixel(col + x, row + y, c);
      buffidx += 2;
    } // end pixel
  }
  uint32_t etime = millis() - startTime;
  return etime;
}

uint32_t Character::drawSprite(unsigned char *data, uint8_t s) {
  uint16_t row, col, buffidx = 0;
  uint32_t startTime = millis();
  uint16_t fx, fy, sx, sy;

  if ((pos_x >= tft->width()) || (pos_y >= tft->height())) {
    return 0;
  }

  sprite->createSprite(width * 3, height * 3);
  fx = (pos_x / width) * width; // absolute position of sprite
  fy = (pos_y / height) * height;
  sx = pos_x % width; // relative position of drawing in the sprite
  sy = pos_y % height;

  // キャラクターの周囲のマップを描画する．

/*
  int bx = (pos_x / width);
  int by = (pos_y / height);
  map->drawMap(sprite,bx,by,3,3);
*/
///*
  for (int i = 0; i < 3; i++) {
    int bx = i + (pos_x / width);
    if (bx > 9) 
      break; 
    for (int j = 0; j < 3; j++) {
      int by = j + (pos_y / height);
      if (by > 9)
        break;      
      drawBmpOnSprite(sprite,(unsigned char *)bgimg[bgmap[by][bx]],i*32,j*32,32,32);
    }
  }
//*/

  //sprite->fillSprite(SCREEN_BGCOLOR);
  //sprite->fillSprite(TFT_TRANSPARENT);
  for (col = 0; col < width; col++) { // For each scanline...
    for (row = 0; row < height; row++) {
      uint16_t c = pgm_read_word(data + buffidx);
      c = ((c >> 8) & 0x00ff) | ((c << 8) & 0xff00); // swap back and fore
      if (c != 0x0000) {                             // ignore background
        if (s == 1) {
          sprite->drawPixel(sx + col, sy + row, c);
        } else {
          sprite->drawRect(sx + col * s, sy + row * s, s, s, c);
        }
      }
      buffidx += 2;
    } // end pixel
  }
  //  sprite->pushSprite(pos_x,pos_y,TFT_TRANSPARENT);
  sprite->pushSprite(fx, fy);
  sprite->deleteSprite();
  uint32_t etime = millis() - startTime;
  return etime;
}
