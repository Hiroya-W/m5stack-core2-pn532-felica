#include <Arduino.h>
#include <M5Core2.h>

#define LGFX_M5STACK_CORE2
#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>

static LGFX lcd;

void setup()
{
  // put your setup code here, to run once:
  M5.begin(true, true, true, true);
  lcd.init();
  lcd.setFont(&fonts::lgfxJapanGothic_40);
  lcd.setBrightness(128);
  lcd.setCursor(0, 0);
  lcd.println("Hello World");
  Serial.begin(115200); // ビットレート設定
}

void loop()
{
  // put your main code here, to run repeatedly:
  Serial.println("Hello World");
  delay(1000);
}
