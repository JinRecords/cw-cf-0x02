#include "Clockface.h"
#include "ColorUtil.h"

EventBus eventBus;

unsigned long lastWeatherUpdate = 0;

// Helper function to draw a conic gradient circle
void drawConicGradientCircle(Adafruit_GFX* display, int16_t x, int16_t y, uint16_t radius, float angle, uint16_t startColor, uint16_t endColor) {
    for (int i = 0; i < 360; i += 5) { // Increment by 5 degrees for performance
        float rad1 = radians(i + angle);
        float rad2 = radians(i + 5 + angle);
        
        int16_t x1 = x + radius * cos(rad1);
        int16_t y1 = y + radius * sin(rad1);
        int16_t x2 = x + radius * cos(rad2);
        int16_t y2 = y + radius * sin(rad2);

        uint16_t color = ColorUtil::interpolate(startColor, endColor, i / 360.0);
        
        display->drawLine(x, y, x1, y1, color);
    }
}

Clockface::Clockface(Adafruit_GFX* display)
{
  _display = display;

  Locator::provide(display);
  Locator::provide(&eventBus);
}

void Clockface::setup(CWDateTime *dateTime) {
  this->_dateTime = dateTime;
  Locator::getDisplay()->setTextWrap(true);
  Locator::getDisplay()->fillRect(0, 0, 64, 64, 0x0000);  
}

void Clockface::update() 
{  
    // Update weather data every 5 minutes
    if (millis() - lastWeatherUpdate >= 300000) {
      updateWeatherData();
      lastWeatherUpdate = millis();
    }

    WeatherData weather = CWWeatherService::getInstance()->getCurrentWeather();

    // Default to a generic palette
    uint16_t bgColor = 0x1082;
    uint16_t secStartColor = 0x2104, secEndColor = 0x4208;
    uint16_t minStartColor = 0x630C, minEndColor = 0x8410;
    uint16_t hourStartColor = 0xA514, hourEndColor = 0xC618;

    if (weather.status == WEATHER_OK && weather.isValid) {
        if (weather.condition == "clear") { // Sunny
            bgColor = 0xFFF0; // Light yellow
            secStartColor = 0xFFE0; // Yellow
            secEndColor = 0xFD20;   // Light Orange
            minStartColor = 0xFC00; // Orange
            minEndColor = 0xFA00;   // Dark Orange
            hourStartColor = 0xF800; // Red
            hourEndColor = 0xA800;   // Dark Red
        } else if (weather.condition == "rain" || weather.condition == "drizzle") {
            bgColor = 0xAEFF; // Very light blue
            secStartColor = 0x001F; // Dark Blue
            secEndColor = 0x001A;   // Slightly Darker Blue
            minStartColor = 0x001F; // Dark Blue
            minEndColor = 0x0010;   // Very Dark Blue
            hourStartColor = 0x801F; // Purple
            hourEndColor = 0x401A;   // Dark Purple
        } else if (weather.condition == "clouds" || weather.condition == "overcast" || weather.condition == "fog") {
            bgColor = 0x8410; // Light Grey
            secStartColor = 0xFFFF; // White
            secEndColor = 0xC618;   // Grey
            minStartColor = 0xC618; // Grey
            minEndColor = 0x630C;   // Dark Grey
            hourStartColor = 0x4208; // Darker Grey
            hourEndColor = 0x2104;   // Even Darker Grey
        }
    }

    // Draw background noise
    for (int i = 0; i < 20; i++) {
        _display->drawPixel(random(64), random(64), ColorUtil::interpolate(bgColor, 0x0000, random(100) / 100.0));
    }

    drawSecond(_dateTime->getSecond(), secStartColor, secEndColor);
    drawMinute(_dateTime->getMinute(), minStartColor, minEndColor);
    drawHour(_dateTime->getHour(), _dateTime->getMinute(), hourStartColor, hourEndColor);
}

void Clockface::drawHour(uint8_t hour, uint8_t minute, uint16_t startColor, uint16_t endColor) {
    float hour_angle = (hour % 12 + minute / 60.0) / 12.0 * 360;
    drawConicGradientCircle(_display, 32, 32, 12, hour_angle, startColor, endColor);
}

void Clockface::drawMinute(uint8_t minute, uint16_t startColor, uint16_t endColor) {
    float minute_angle = (minute / 60.0) * 360;
    drawConicGradientCircle(_display, 32, 32, 22, minute_angle, startColor, endColor);
}

void Clockface::drawSecond(uint8_t second, uint16_t startColor, uint16_t endColor) {
    float second_angle = (second / 60.0) * 360;
    drawConicGradientCircle(_display, 32, 32, 30, second_angle, startColor, endColor);
}

void Clockface::updateWeatherData() 
{
  CWWeatherService::getInstance()->getCurrentWeather();
}