
#include "Clockface.h"

const char* FORMAT_TWO_DIGITS = "%02d";

EventBus eventBus;

unsigned long lastMillis = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastScrollUpdate = 0;
int scrollOffset = 0;

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
  if (millis() - lastMillis >= 1000) {

    Locator::getDisplay()->fillRect(0, 0, 64, 64, 0x0000);

    WeatherData weather = CWWeatherService::getInstance()->getCurrentWeather();
    uint16_t color = 0xFFFF;

    if (weather.status == WEATHER_OK && weather.isValid) {
        if (weather.condition == "clear") {
            color = 0xFFB347; // Bright, warm sunny tones
        } else if (weather.condition == "clouds") {
            color = 0xB0E0E6; // Striking cloudy blue tones
        } else if (weather.condition == "rain") {
            color = 0x5F9EA0; // Vibrant rain blue tones
        } else if (weather.condition == "snow") {
            color = 0xFFFAFA; // Crisp and bright snow tones
        } else {
            color = 0x87CEEB; // Default blue sky
        }
    }

    drawHour(_dateTime->getHour(), _dateTime->getMinute(), color);
    drawMinute(_dateTime->getMinute(), color);
    drawSecond(_dateTime->getSecond(), color);

    // Update weather data every 5 minutes
    if (millis() - lastWeatherUpdate >= 300000) {
      updateWeatherData();
      lastWeatherUpdate = millis();
    }
    
    // Update weather display continuously for smooth scrolling
    updateWeatherDisplay();
    
    lastMillis = millis();
  }  
}

void Clockface::drawHour(uint8_t hour, uint8_t minute, uint16_t color) {
    float hour_angle = (hour % 12 + minute / 60.0) / 12.0 * 2 * PI;
    int16_t x = 32 + 10 * cos(hour_angle - PI / 2);
    int16_t y = 32 + 10 * sin(hour_angle - PI / 2);
    Locator::getDisplay()->fillCircle(x, y, 12, color);
}

void Clockface::drawMinute(uint8_t minute, uint16_t color) {
    float minute_angle = (minute / 60.0) * 2 * PI;
    int16_t x = 32 + 16 * cos(minute_angle - PI / 2);
    int16_t y = 32 + 16 * sin(minute_angle - PI / 2);
    Locator::getDisplay()->fillCircle(x, y, 10, color);
}

void Clockface::drawSecond(uint8_t second, uint16_t color) {
    float second_angle = (second / 60.0) * 2 * PI;
    int16_t x = 32 + 22 * cos(second_angle - PI / 2);
    int16_t y = 32 + 22 * sin(second_angle - PI / 2);
    Locator::getDisplay()->fillCircle(x, y, 8, color);
}

void Clockface::updateWeatherData() 
{
  // This method only fetches weather data, doesn't update display
  // The display will be updated by updateWeatherDisplay() which runs continuously
  CWWeatherService::getInstance()->getCurrentWeather();
}

void Clockface::updateWeatherDisplay() 
{
  // Clear the area next to WiFi icon (from x=10 to x=64, y=55 to y=63)
  Locator::getDisplay()->fillRect(10, 55, 54, 8, 0x0000);
  
  WeatherData weather = CWWeatherService::getInstance()->getCurrentWeather();
  
  // Determine what to display based on weather status
  String displayText = "";
  const unsigned short* weatherIcon = nullptr;
  
  switch (weather.status) {
    case WEATHER_OK:
      if (weather.isValid && !weather.condition.isEmpty()) {
        displayText = weather.condition;
        
        // Set weather icon based on condition
        if (weather.condition == "clear") {
          weatherIcon = WEATHER_CLEAR;
        } else if (weather.condition == "cloudy" || weather.condition == "partly" || weather.condition == "overcast") {
          weatherIcon = WEATHER_CLOUDY;
        } else if (weather.condition == "rain" || weather.condition == "drizzle") {
          weatherIcon = WEATHER_RAIN;
        } else if (weather.condition == "thunder") {
          weatherIcon = WEATHER_THUNDER;
        } else if (weather.condition == "snow") {
          weatherIcon = WEATHER_SNOW;
        } else if (weather.condition == "fog") {
          weatherIcon = WEATHER_FOG;
        }
      }
      break;
      
    case WEATHER_CONNECTING:
      displayText = "...";
      weatherIcon = WEATHER_CLOUDY; // Use cloudy icon as placeholder
      break;
      
    case WEATHER_ERROR:
    default:
      // Check if we're still in startup retry mode
      static unsigned long lastErrorCheck = 0;
      if (millis() - lastErrorCheck >= 1000) { // Check every second
        lastErrorCheck = millis();
        // The weather service will handle retries internally
        // We just show "error" until it succeeds
      }
      displayText = "error";
      weatherIcon = WEATHER_CLOUDY; // Use cloudy icon as placeholder
      break;
  }
  
  // Display weather icon
  if (weatherIcon) {
    Locator::getDisplay()->drawRGBBitmap(10, 55, weatherIcon, 8, 8);
  }
  
  // Display weather text with scrolling if needed
  if (!displayText.isEmpty()) {
    scrollText(displayText, 20, 62, 44); // 44 pixels max width (64 - 20)
  }
}

void Clockface::scrollText(const String& text, int x, int y, int maxWidth) {
  Locator::getDisplay()->setFont(&small4pt7b);
  Locator::getDisplay()->setTextColor(0xffff);
  
  // Calculate text width
  uint16_t textWidth, h = 0;
  int16_t x1, y1 = 0;
  Locator::getDisplay()->getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &h);
  
  // If text fits within maxWidth, display normally
  if (textWidth <= maxWidth) {
    Locator::getDisplay()->setCursor(x, y);
    Locator::getDisplay()->print(text);
    scrollOffset = 0; // Reset scroll offset
    return;
  }
  
  // Text is too long, implement scrolling
  if (millis() - lastScrollUpdate >= 300) { // Scroll every 300ms for better readability
    scrollOffset++;
    if (scrollOffset > text.length() + 5) { // Add some padding and pause at end
      scrollOffset = 0;
    }
    lastScrollUpdate = millis();
  }
  
  // Clear the text area
  Locator::getDisplay()->fillRect(x, y - h, maxWidth, h, 0x0000);
  
  // Calculate which part of the text to show
  String displayText = text;
  
  // Show first 5 characters immediately, then scroll from there
  if (scrollOffset == 0) {
    // Show first 5 characters
    displayText = text.substring(0, min(5, (int)text.length()));
  } else if (scrollOffset > 0) {
    if (scrollOffset < text.length()) {
      displayText = text.substring(scrollOffset);
    } else {
      // Show beginning of text when we reach the end
      displayText = text.substring(0, scrollOffset - text.length());
    }
  }
  
  // Display the text
  Locator::getDisplay()->setCursor(x, y);
  Locator::getDisplay()->print(displayText);
}
