// ESP32 E-Paper Weather Clock
// Main application file
//
// Features:
// - WiFi auto-reconnect
// - NTP time sync
// - Weather data fetch and display
// - Modular code structure

// Ensures WiFi is connected, attempts to reconnect if not
// See networking.h for implementation
// Updates the E-Paper display with the current time, temperature, and humidity
#include "networking.h"
// Forward declaration for NTP sync
void syncTimeFromNTP();

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "EPD.h"
#include "pic.h"
#include "config.h"

#include <time.h>


#define PIC_TEMP_X 216  // The width of the resolution of the temperature icon
#define PIC_TEMP_Y 64   // The height of the resolution of the temperature icon
#define PIC_HUMI_X 184  // The width of the resolution of the humidity icon
#define PIC_HUMI_Y 88   // The height of the resolution of the humidity icon

// E-Paper display resolution (should match EPD_W * EPD_H / 8 for 1bpp)
#include "EPD_Init.h"
#define EPD_IMAGE_BUFFER_SIZE ((EPD_W * EPD_H) / 8)
static_assert(EPD_IMAGE_BUFFER_SIZE <= 15000, "EPD_IMAGE_BUFFER_SIZE exceeds available memory!");
uint8_t ImageBW[EPD_IMAGE_BUFFER_SIZE];  // Define the buffer size based on the resolution of the electronic paper display screen



unsigned long lastUpdateTime = 0;  // The timestamp of the last update time
unsigned long lastAnalysisTime = 0;  // Last analysis timestamp


// Define variables related to JSON data
String jsonBuffer;
int httpResponseCode;
JSONVar myObject;

// Define variables related to weather information
String temperature;
String humidity;
String city_js;

// Build URL for OpenWeatherMap API request
String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" WEATHER_CITY "," WEATHER_COUNTRY_CODE "&APPID=" OPENWEATHERMAP_API_KEY "&units=metric";

//****************************************************************************************
// Syncs time from NTP server
void syncTimeFromNTP() {
  // Syncs time from NTP server and prints result to serial
  const char* ntpServer = "pool.ntp.org";
  configTime(5.5 * 3600, 0, ntpServer); // Set time zone and NTP
  Serial.println("Syncing time from NTP...");
  delay(3000); // Wait for time to update
  time_t now = time(nullptr);
  if (now < 100000) {
    Serial.println("NTP time not set, retrying...");
    delay(2000);
    now = time(nullptr);
  }
  if (now > 100000) {
    Serial.print("NTP time synced: ");
    Serial.println(ctime(&now));
  } else {
    Serial.println("Failed to sync NTP time.");
  }
}

// Arduino setup function: initializes WiFi, serial, time, and E-Paper display
void setup() {
  // Start WiFi connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.begin(115200);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting...");
    delay(3000); // Wait and retry
  }
  Serial.println("WiFi is connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize display power (if required by hardware)
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);



  // Sync time from NTP
  syncTimeFromNTP();
  js_analysis();

  Serial.println("R24H R26H ready!");
  EPD_GPIOInit(); 
  EPD_Init();
  EPD_Clear(); 
  EPD_Update();     
                    
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE); 
  Paint_Clear(WHITE); 
  EPD_Display(ImageBW);
  EPD_Clear_R26H(ImageBW);
  EPD_Update();
  Update_Display(); 
}



// Arduino main loop: updates display and fetches weather data at intervals
void loop() {
  // Main loop: ensures WiFi, updates display, fetches weather, and keeps time accurate
  // Ensure WiFi is connected before any network operation
  ensureWiFiConnected();
  unsigned long currentTime = millis();

  // Update time, temperature, and humidity every minute
  if (currentTime - lastUpdateTime >= 60000) {
    // Optionally re-sync time from NTP every 10 minutes for accuracy
    static unsigned long lastNTPSync = 0;
    if (currentTime - lastNTPSync >= 600000) { // 10 minutes
      syncTimeFromNTP();
      lastNTPSync = currentTime;
    }
    Update_Display();
    lastUpdateTime = currentTime;
  }
  // Fetch and analyze weather data every 30 minutes
  if (currentTime - lastAnalysisTime >= 30 * 60000) {
    jsonBuffer = httpGETRequest(serverPath.c_str());
    Serial.println(jsonBuffer); // Print the obtained JSON data
    js_analysis();
    lastAnalysisTime = currentTime;
  }
}

void clear_all() {
  EPD_Clear();               
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
  Paint_Clear(WHITE);            
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW); 
}

//  Update time and temperature/humidity
// Updates the E-Paper display with the current time, temperature, and humidity
void Update_Display() {
  // Get current time information
  time_t t;
  time(&t);
  struct tm *timeinfo = localtime(&t);
  if (timeinfo == NULL) {
    Serial.println("Failed to get time");
    return;
  }
  Serial.println("----------The screen time display is about to refresh----------");
  delay(1000);
  Serial.print("Current time: ");
  Serial.println(asctime(timeinfo));

  // Format date and time strings
  static char buff1[20]; // Storage for Year-Month-Day
  static char buff2[20]; // Storage for hour:minute
  char buffer[40];       // Used for temperature and humidity
  sprintf(buff2, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
  sprintf(buff1, "%02d-%02d-%04d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);

  // Get day of the week
  const char *daysOfWeek[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  const char *dayOfWeek = daysOfWeek[timeinfo->tm_wday];

  // Draw UI elements
  EPD_DrawLine(0, 150, 400, 150, BLACK); // Horizontal divider
  EPD_ShowPicture(0, 208, PIC_TEMP_X, PIC_TEMP_Y, gImage_pic_temp1, WHITE);  // Temperature icon
  EPD_ShowPicture(216, 200, PIC_HUMI_X, PIC_HUMI_Y, gImage_pic_humi, WHITE); // Humidity icon

  // Display date, time, and day
  EPD_ShowString(80, 0, buff1, 48, BLACK);      // Date
  EPD_ShowString(80, 48, buff2, 96, BLACK);     // Time
  EPD_ShowString(120, 155, dayOfWeek, 48, BLACK); // Day of week

  // Display temperature
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s C", temperature);
  EPD_ShowString(90, 260, buffer, 24, BLACK);
  // Display humidity
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s %", humidity);
  EPD_ShowString(330, 260, buffer, 24, BLACK);
  Serial.println("* * * * * * The screen time display has been refreshed * * * * * *");

  // Update E-Paper display
  EPD_Display(ImageBW);
  EPD_Update_Part();
}

// Fetches and parses weather data from OpenWeatherMap, updates global variables
void js_analysis() {
  // Check if successfully connected to WiFi network
  if (WiFi.status() == WL_CONNECTED) {
    // Loop until a valid HTTP response code of 200 is obtained
    while (httpResponseCode != 200) {
      // Send HTTP GET request and retrieve response content
      jsonBuffer = httpGETRequest(serverPath.c_str());
      Serial.println(jsonBuffer); // Print the obtained JSON data
      myObject = JSON.parse(jsonBuffer); // Parse JSON data

      // Check if JSON parsing is successful
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return; // If parsing fails, exit the function
      }
      delay(1000); // Wait and retry
    }

    // Extract weather information from parsed JSON data
    temperature = JSON.stringify(myObject["main"]["temp"]);
    humidity = JSON.stringify(myObject["main"]["humidity"]);
    city_js = JSON.stringify(myObject["name"]);

    // Print extracted weather information
    Serial.print("Obtained temperature: ");
    Serial.println(temperature);
    Serial.print("Obtained humidity: ");
    Serial.println(humidity);
    Serial.print("Obtained city_js: ");
    Serial.println(city_js);
  } else {
    // If WiFi is disconnected, print a prompt message
    Serial.println("WiFi Disconnected");
  }
}

//Define the function for HTTP GET requests
String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  //Initialize the HTTP client and specify the requested server URL
  http.begin(client, serverName);

  // Send HTTP GET request
  httpResponseCode = http.GET();

  // The response content returned by initialization
  String payload = "{}";

  //Check the response code and process the response content
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode); // Print response code
    payload = http.getString(); // Obtain response content
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode); // Print error code
  }
  //Release HTTP client resources
  http.end();

  return payload; // Return response content
}
