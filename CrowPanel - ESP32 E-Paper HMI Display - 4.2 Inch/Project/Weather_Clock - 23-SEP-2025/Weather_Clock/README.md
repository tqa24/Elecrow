# Weather Clock ESP32 E-Paper Project

This project displays the current time, temperature, and humidity on a 4.2-inch E-Paper display using an ESP32-S3. Weather data is fetched from OpenWeatherMap.

## Features
- WiFi connectivity (credentials in `config.h`)
- Fetches weather data from OpenWeatherMap
- Displays time, temperature, humidity, and icons
- Modular code for display and networking

## Setup
1. **Clone the repository**
2. **Create `config.h`** (see `config.h` template, not tracked in git):
   ```cpp
   #define WIFI_SSID "your_wifi"
   #define WIFI_PASSWORD "your_password"
   #define OPENWEATHERMAP_API_KEY "your_api_key"
   #define WEATHER_CITY "YourCity"
   #define WEATHER_COUNTRY_CODE "YourCountryCode"
   ```
3. **Install Arduino libraries:**
   - WiFi
   - HTTPClient
   - Arduino_JSON
4. **Compile and upload to ESP32-S3**

## File Structure
- `Weather_Clock.ino` - Main application logic
- `EPD*.h/cpp` - E-Paper display drivers and drawing
- `spi.h/cpp` - SPI communication
- `pic.h` - Image/icon data
- `config.h` - WiFi and API credentials (not tracked)

## Notes
- Do not commit `config.h` (contains sensitive data)
- Buffer sizes and pin numbers are now named constants

## License
See LICENSE file if present.
