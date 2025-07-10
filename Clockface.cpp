
#include "Clockface.h"

const char* FORMAT_TWO_DIGITS = "%02d";

EventBus eventBus;

unsigned long lastMillis = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastScrollUpdate = 0;
int scrollOffset = 0;

char hInWords[20];
char mInWords[20]; 
char formattedDate[20];

int temperature = 26;

DateI18nEN i18n;

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

  updateTime();
  updateDate();
  //updateTemperature();
}


void Clockface::update() 
{  
  if (millis() - lastMillis >= 1000) {

    if (_dateTime->getSecond() == 0) {
      updateTime();
    }

    if (_dateTime->getMinute() == 0 && _dateTime->getSecond() == 0) {
      updateDate();    
    }

    // Update weather every 5 minutes
    if (millis() - lastWeatherUpdate >= 300000) {
      updateWeather();
      lastWeatherUpdate = millis();
    }
    
    lastMillis = millis();
  }  
}

void Clockface::updateTime() 
{
  Locator::getDisplay()->fillRect(0, 0, 64, 40, 0x0000);  

  i18n.timeInWords(_dateTime->getHour(), _dateTime->getMinute(), hInWords, mInWords);  
  
  // Hour
  Locator::getDisplay()->setFont(&hour8pt7b);  
  Locator::getDisplay()->setCursor(1, 14);
  Locator::getDisplay()->setTextColor(0x2589);
  Locator::getDisplay()->println(hInWords);
  
  // Minute
  Locator::getDisplay()->setFont(&minute7pt7b);
  Locator::getDisplay()->setCursor(0, 25);
  Locator::getDisplay()->setTextColor(0xffff);
  Locator::getDisplay()->println(mInWords);

  // Separator line
  Locator::getDisplay()->drawFastHLine(1, 40, 62, 0xffff);

  if (WiFi.status() == WL_CONNECTED) {
    Locator::getDisplay()->drawRGBBitmap(1, 55, WIFI, 8, 8);
  } else {
    Locator::getDisplay()->fillRect(1, 55, 8, 8, 0x0000);
  }
  
  // Display weather condition next to WiFi icon
  updateWeather();
}

void Clockface::updateDate() 
{
  Locator::getDisplay()->fillRect(0, 41, 46, 13, 0x0000);

  // Date
  Locator::getDisplay()->setFont(&minute7pt7b);
  Locator::getDisplay()->setCursor(0, 52);
  Locator::getDisplay()->setTextColor(0x2589);
    
  const char* fmt = i18n.formatDate(_dateTime->getDay(), _dateTime->getMonth());

  Locator::getDisplay()->print(fmt);

  uint16_t dateWidth, h = 0;
  int16_t x1, y1 = 0;
  Locator::getDisplay()->getTextBounds(fmt, 0, 0, &x1, &y1, &dateWidth, &h);

  // Weekday
  Locator::getDisplay()->setFont(&small4pt7b);
  //Locator::getDisplay()->setFont(&minute7pt7b);
  Locator::getDisplay()->setCursor(dateWidth + 2, 52);
  Locator::getDisplay()->setTextColor(0xffff);
  Locator::getDisplay()->println(i18n.weekDayName(_dateTime->getWeekday()));  
}

void Clockface::updateTemperature() 
{

  Locator::getDisplay()->fillRect(46, 41, 18, 13, 0x0000);
  Locator::getDisplay()->setFont(&minute7pt7b);

  // Temperature
  // TODO get temperature
  temperature++;
  if (temperature > 30) temperature = 20;

  char buffer[4];  
  sprintf(buffer, "%d~", temperature);
  
  uint16_t tempWidth, h = 0;
  int16_t x1,y1 = 0;
  Locator::getDisplay()->getTextBounds(buffer, 0, 0, &x1, &y1, &tempWidth, &h);
 
  Locator::getDisplay()->setCursor(62-tempWidth, 52);
  Locator::getDisplay()->setTextColor(0xffff);
  Locator::getDisplay()->println(buffer);
  
  Locator::getDisplay()->drawRGBBitmap(12, 55, MAIL, 8, 8);
  Locator::getDisplay()->drawRGBBitmap(55, 55, WEATHER_CLOUDY_SUN, 8, 8);
}

void Clockface::updateWeather() 
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
  if (scrollOffset > 0) {
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
