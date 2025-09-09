#pragma once

#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Tile.h>
#include <Locator.h>
#include <Game.h>
#include <Object.h>
#include <ImageUtils.h>
#include <WiFi.h>

// Commons
#include "IClockface.h"
#include "Icons.h"
#include "CWWeatherService.h"
#include "WeatherIcons.h"
#include "small4pt7b.h"

class Clockface: public IClockface {
  private:
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    void updateWeatherData();
        void drawHour(uint8_t hour, uint8_t minute, uint16_t startColor, uint16_t endColor);
    void drawMinute(uint8_t minute, uint16_t startColor, uint16_t endColor);
    void drawSecond(uint8_t second, uint16_t startColor, uint16_t endColor);

  public:
    Clockface(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime);
    void update();
};
