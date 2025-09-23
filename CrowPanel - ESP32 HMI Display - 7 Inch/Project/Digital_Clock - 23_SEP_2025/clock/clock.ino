#include <PCA9557.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <WiFi.h>
#include <time.h>
#include "ui.h"

class LGFX : public lgfx::LGFX_Device
{
public:
  lgfx::Bus_RGB     _bus_instance;
  lgfx::Panel_RGB   _panel_instance;

  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();
      cfg.panel = &_panel_instance;
      
      cfg.pin_d0  = GPIO_NUM_15; // B0
      cfg.pin_d1  = GPIO_NUM_7;  // B1
      cfg.pin_d2  = GPIO_NUM_6;  // B2
      cfg.pin_d3  = GPIO_NUM_5;  // B3
      cfg.pin_d4  = GPIO_NUM_4;  // B4
      
      cfg.pin_d5  = GPIO_NUM_9;  // G0
      cfg.pin_d6  = GPIO_NUM_46; // G1
      cfg.pin_d7  = GPIO_NUM_3;  // G2
      cfg.pin_d8  = GPIO_NUM_8;  // G3
      cfg.pin_d9  = GPIO_NUM_16; // G4
      cfg.pin_d10 = GPIO_NUM_1;  // G5
      
      cfg.pin_d11 = GPIO_NUM_14; // R0
      cfg.pin_d12 = GPIO_NUM_21; // R1
      cfg.pin_d13 = GPIO_NUM_47; // R2
      cfg.pin_d14 = GPIO_NUM_48; // R3
      cfg.pin_d15 = GPIO_NUM_45; // R4

      cfg.pin_henable = GPIO_NUM_41;
      cfg.pin_vsync   = GPIO_NUM_40;
      cfg.pin_hsync   = GPIO_NUM_39;
      cfg.pin_pclk    = GPIO_NUM_0;
      cfg.freq_write  = 15000000;

      cfg.hsync_polarity    = 0;
      cfg.hsync_front_porch = 40;
      cfg.hsync_pulse_width = 48;
      cfg.hsync_back_porch  = 40;
      
      cfg.vsync_polarity    = 0;
      cfg.vsync_front_porch = 1;
      cfg.vsync_pulse_width = 31;
      cfg.vsync_back_porch  = 13;

      cfg.pclk_active_neg   = 1;
      cfg.de_idle_high      = 0;
      cfg.pclk_idle_high    = 0;

      _bus_instance.config(cfg);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.memory_width  = 800;
      cfg.memory_height = 480;
      cfg.panel_width  = 800;
      cfg.panel_height = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      _panel_instance.config(cfg);
    }
    _panel_instance.setBus(&_bus_instance);
    setPanel(&_panel_instance);
  }
};

LGFX lcd;
int led;
int brightness = 255; // Brightness level (0-255)

// WiFi credentials
const char* ssid = "Capgemini_4G";
const char* password = "MN704116";

// Time settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;  // Use UTC as base
const int daylightOffset_sec = 0;  // Handle DST manually

// Time zone offsets
const long IST_OFFSET = 19800;  // IST offset in seconds (UTC+5:30 = 19800 seconds)
const long UK_OFFSET = 0;       // UK offset in seconds (UTC+0, DST handled separately)

// Time update variables
unsigned long lastTimeUpdate = 0;
bool timeInitialized = false;

// Forward declarations
bool isUKInDST(struct tm* timeinfo);

// Interactive features
int currentTheme = 5; // Start with theme 5 (white background) - 0=Green, 1=Blue, 2=Red, 3=Cyan, 4=Yellow, 5=White
bool use24HourFormat = false; // Default to 12-hour format
unsigned long lastInteractionTime = 0;
bool buttonsVisible = true;
//UI
#define TFT_BL 2
SPIClass& spi = SPI;
#include "touch.h"

/* Change to your screen resolution */
static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t disp_draw_buf[800 * 480 / 15];
static lv_disp_drv_t disp_drv;

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
 lcd.pushImageDMA(area->x1, area->y1, w, h,(lgfx::rgb565_t*)&color_p->full);
#else
  lcd.pushImageDMA(area->x1, area->y1, w, h,(lgfx::rgb565_t*)&color_p->full);
#endif

  lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  if (touch_has_signal())
  {
    if (touch_touched())
    {
      data->state = LV_INDEV_STATE_PR;
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
      // Serial.print("Data x ");
      // Serial.println(data->point.x);
      // Serial.print("Data y ");
      // Serial.println(data->point.y);
    }
    else if (touch_released())
    {
      data->state = LV_INDEV_STATE_REL;
    }
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
  delay(15);
}

void setup()
{
  Serial.begin(9600);
  delay(1000); // Wait for serial to initialize
  Serial.println("=== CrowPanel 7\" HMI Display ===");
  Serial.println("Initializing system...");

  // Initialize LED pin
  pinMode(38, OUTPUT);
  digitalWrite(38, LOW);

  Wire.begin(19, 20);
  Serial.println("Initializing LCD...");
  lcd.begin();
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextSize(2);
  delay(200);
  Serial.println("LCD initialized");
  
  Serial.println("Initializing LVGL...");
  lv_init();

  touch_init();

  screenWidth = lcd.width();
  screenHeight = lcd.height();

  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / 15);
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  // Initialize backlight with PWM for brightness control
#ifdef TFT_BL
  ledcSetup(1, 300, 8);
  ledcAttachPin(TFT_BL, 1);
  ledcWrite(1, brightness); // Use brightness variable
#endif

  Serial.println("Initializing UI...");
  ui_init();
  lv_timer_handler();
  Serial.println("UI initialized");
  Serial.println("Screen should now be visible with black background");
  
  // Initialize WiFi
  Serial.println("Connecting to WiFi...");
  updateStatusMessage("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
    lv_timer_handler(); // Keep UI responsive
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    updateStatusMessage("WiFi Connected - Syncing time...");
    
    // Initialize time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Time synchronization started...");
    
    // Wait for time to be set
    struct tm timeinfo;
    int attempts = 0;
    while (!getLocalTime(&timeinfo) && attempts < 10) {
      Serial.println("Failed to obtain time, retrying...");
      delay(1000);
      attempts++;
      lv_timer_handler(); // Keep UI responsive
    }
    
    if (attempts < 10) {
      timeInitialized = true;
      Serial.println("Time synchronized successfully!");
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
      updateStatusMessage("Time synchronized");
    } else {
      Serial.println("Failed to synchronize time, will use manual time");
      updateStatusMessage("Time sync failed");
    }
  } else {
    Serial.println();
    Serial.println("WiFi connection failed. Clock will not sync with NTP.");
    updateStatusMessage("WiFi connection failed");
  }
  
  Serial.println("System initialization complete!");
  Serial.println("Digital clock is now running");
  
  // Initialize interaction timer
  lastInteractionTime = millis();
  
  // Set default white theme (theme 5)
  delay(500); // Wait for UI to be fully initialized
  applyWhiteTheme();
}

// Function to set display brightness
void setBrightness(int level) {
  brightness = constrain(level, 0, 255);
  ledcWrite(1, brightness);
  Serial.print("Brightness set to: ");
  Serial.println(brightness);
}

// Function to apply white theme on startup
void applyWhiteTheme() {
  // Set white background theme
  lv_obj_set_style_bg_color(ui_Screen1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT); // White background
  lv_obj_set_style_text_color(ui_TimeLabel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT); // Black IST time
  lv_obj_set_style_text_color(ui_UKTimeLabel, lv_color_hex(0x0066CC), LV_PART_MAIN | LV_STATE_DEFAULT); // Blue UK time
  lv_obj_set_style_text_color(ui_ISTLabel, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT); // Gray timezone labels
  lv_obj_set_style_text_color(ui_UKLabel, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(ui_DateLabel, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT); // Dark gray date
  lv_obj_set_style_text_color(ui_DayLabel, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT); // Gray day
  lv_obj_set_style_text_color(ui_StatusLabel, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT); // Light gray status
  Serial.println("White theme applied as default");
}

// Interactive functions
void increaseBrightness() {
  brightness = min(255, brightness + 25);
  setBrightness(brightness);
  lastInteractionTime = millis();
  Serial.print("Brightness increased to: ");
  Serial.println(brightness);
}

void decreaseBrightness() {
  brightness = max(25, brightness - 25);
  setBrightness(brightness);
  lastInteractionTime = millis();
  Serial.print("Brightness decreased to: ");
  Serial.println(brightness);
}

void changeColorTheme() {
  currentTheme = (currentTheme + 1) % 6;
  
  if (currentTheme == 5) {
    // White background theme
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT); // White background
    lv_obj_set_style_text_color(ui_TimeLabel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT); // Black IST time
    lv_obj_set_style_text_color(ui_UKTimeLabel, lv_color_hex(0x0066CC), LV_PART_MAIN | LV_STATE_DEFAULT); // Blue UK time
    lv_obj_set_style_text_color(ui_ISTLabel, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT); // Gray timezone labels
    lv_obj_set_style_text_color(ui_UKLabel, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_DateLabel, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT); // Dark gray date
    lv_obj_set_style_text_color(ui_DayLabel, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT); // Gray day
    lv_obj_set_style_text_color(ui_StatusLabel, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT); // Light gray status
  } else {
    // Dark background themes
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT); // Black background
    uint32_t timeColors[] = {0x00FF00, 0x0080FF, 0xFF0040, 0x00FFFF, 0xFFFF00};
    uint32_t dateColors[] = {0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF};
    uint32_t dayColors[] = {0x00FFFF, 0x00FFFF, 0x00FFFF, 0x00FFFF, 0x00FFFF};
    
    lv_obj_set_style_text_color(ui_TimeLabel, lv_color_hex(timeColors[currentTheme]), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_UKTimeLabel, lv_color_hex(0xFF8000), LV_PART_MAIN | LV_STATE_DEFAULT); // Keep UK time orange
    lv_obj_set_style_text_color(ui_ISTLabel, lv_color_hex(0x00FFFF), LV_PART_MAIN | LV_STATE_DEFAULT); // Keep timezone labels cyan
    lv_obj_set_style_text_color(ui_UKLabel, lv_color_hex(0x00FFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_DateLabel, lv_color_hex(dateColors[currentTheme]), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_DayLabel, lv_color_hex(dayColors[currentTheme]), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_StatusLabel, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  
  lastInteractionTime = millis();
  Serial.print("Theme changed to: ");
  Serial.println(currentTheme);
}

void toggleTimeFormat() {
  use24HourFormat = !use24HourFormat;
  lv_label_set_text(ui_FormatLabel, use24HourFormat ? "24H" : "12H");
  lastInteractionTime = millis();
  Serial.print("Time format: ");
  Serial.println(use24HourFormat ? "24H" : "12H");
}

void showTimeDetails() {
  char detailText[100];
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    bool ukDST = isUKInDST(&timeinfo);
    sprintf(detailText, "IST: UTC+5:30 | UK: UTC+%d | Week %d", 
            ukDST ? 1 : 0, (timeinfo.tm_yday / 7) + 1);
  } else {
    sprintf(detailText, "Dual Time: IST & UK | Uptime: %lu s", millis() / 1000);
  }
  lv_label_set_text(ui_StatusLabel, detailText);
  lastInteractionTime = millis();
  Serial.println("Time details shown");
}

// Function to check if UK is in DST (British Summer Time)
bool isUKInDST(struct tm* timeinfo) {
  int month = timeinfo->tm_mon + 1; // tm_mon is 0-based
  int day = timeinfo->tm_mday;
  
  // DST starts on the last Sunday in March and ends on the last Sunday in October
  if (month < 3 || month > 10) return false;
  if (month > 3 && month < 10) return true;
  
  // For March and October, use simplified logic
  // DST typically starts around March 25-31 and ends around October 25-31
  if (month == 3) {
    return (day >= 25); // Approximate last Sunday of March
  }
  if (month == 10) {
    return (day < 25); // Before last Sunday of October
  }
  
  return false;
}

// Function to update time display
void updateTimeDisplay() {
  struct tm timeinfo;
  
  if (getLocalTime(&timeinfo) || timeInitialized) {
    // Get current UTC time
    time_t utcTime = mktime(&timeinfo);
    
    // Calculate IST time (UTC + 5:30)
    time_t istTime = utcTime + IST_OFFSET;
    struct tm istTimeinfo;
    localtime_r(&istTime, &istTimeinfo);
    
    // Calculate UK time (UTC + 0 or +1 for DST)
    int ukOffset = UK_OFFSET;
    if (isUKInDST(&timeinfo)) {
      ukOffset += 3600; // Add 1 hour for DST
    }
    time_t ukTime = utcTime + ukOffset;
    struct tm ukTimeinfo;
    localtime_r(&ukTime, &ukTimeinfo);
    
    char istTimeStr[12];
    char ukTimeStr[12];
    char dateStr[20];
    char dayStr[10];
    
    // Format IST time based on 12H/24H preference
    if (use24HourFormat) {
      strftime(istTimeStr, sizeof(istTimeStr), "%H:%M:%S", &istTimeinfo);
      strftime(ukTimeStr, sizeof(ukTimeStr), "%H:%M:%S", &ukTimeinfo);
    } else {
      strftime(istTimeStr, sizeof(istTimeStr), "%I:%M:%S %p", &istTimeinfo);
      strftime(ukTimeStr, sizeof(ukTimeStr), "%I:%M:%S %p", &ukTimeinfo);
    }
    
    // Format date using IST (Month DD, YYYY)
    strftime(dateStr, sizeof(dateStr), "%B %d, %Y", &istTimeinfo);
    
    // Format day of week using IST
    strftime(dayStr, sizeof(dayStr), "%A", &istTimeinfo);
    
    // Update the display
    updateClockDisplay(istTimeStr, ukTimeStr, dateStr, dayStr);
  } else {
    // If we can't get time, show system uptime as fallback
    unsigned long totalSeconds = millis() / 1000;
    unsigned long hours = totalSeconds / 3600;
    unsigned long minutes = (totalSeconds % 3600) / 60;
    unsigned long seconds = totalSeconds % 60;
    
    char istTimeStr[12];
    char ukTimeStr[12];
    char dateStr[20];
    char dayStr[10];
    
    // Show uptime as IST reference
    sprintf(istTimeStr, "%02lu:%02lu:%02lu", hours % 24, minutes, seconds);
    // UK time is approximately IST - 5:30 hours (rough estimate without proper time sync)
    unsigned long ukTotalSeconds = totalSeconds - 19800; // Subtract 5:30 hours
    unsigned long ukHours = (ukTotalSeconds / 3600) % 24;
    unsigned long ukMinutes = (ukTotalSeconds % 3600) / 60;
    unsigned long ukSecs = ukTotalSeconds % 60;
    sprintf(ukTimeStr, "%02lu:%02lu:%02lu", ukHours, ukMinutes, ukSecs);
    strcpy(dateStr, "No Time Sync");
    strcpy(dayStr, "Uptime");
    
    updateClockDisplay(istTimeStr, ukTimeStr, dateStr, dayStr);
  }
}

void hideButtons() {
  lv_obj_add_flag(ui_BrightnessUpBtn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_BrightnessDownBtn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_ThemeBtn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_FormatBtn, LV_OBJ_FLAG_HIDDEN);
  buttonsVisible = false;
}

void showButtons() {
  lv_obj_clear_flag(ui_BrightnessUpBtn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(ui_BrightnessDownBtn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(ui_ThemeBtn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(ui_FormatBtn, LV_OBJ_FLAG_HIDDEN);
  buttonsVisible = true;
}

void loop()
{
  // Update time display every 1 second
  if(millis() - lastTimeUpdate > 1000) {
    updateTimeDisplay();
    lastTimeUpdate = millis();
  }
  
  // Handle LVGL timer
  lv_timer_handler();
  delay(10);
}
