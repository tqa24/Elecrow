/**
 * Wireless Info Display - ESP32 E-Paper HMI Display Controller
 * 
 * This project creates a web-based interface for controlling a 4.2-inch e-paper display.
 * Features:
 * - WiFi-enabled web server for remote control
 * - Up to 8 configurable text rows with different font sizes
 * - Color inversion support (black on white / white on black)
 * - Real-time preview in web interface
 * - Display border toggle
 * - Input validation and error handling
 * 
 * Hardware: ESP32 with 4.2" e-paper display (GxEPD2_420_GYE042A87)
 * Author: Fusion Automate
 * Date: September 23, 2025
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "GxEPD2_BW.h"
#include "config.h"

// Pin definitions
#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45

// Web server
WebServer server(WEB_SERVER_PORT);

// E-paper display initialization
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

// Display settings structure
struct Row {
  String text;

  int fontSize;
  Row() : text(""), fontSize(18) {}
};

struct DisplaySettings {
  static const int MAX_ROWS = 8;
  Row rows[MAX_ROWS];
  bool border;
  bool invertColors;  // New field for color inversion
  DisplaySettings() : border(true), invertColors(false) {}
};

DisplaySettings currentSettings;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Info Display Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; margin: 20px; background-color: #f0f0f0; }
    .content { max-width: 800px; margin: auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
    .form-group { margin-bottom: 15px; }
    label { display: block; margin-bottom: 5px; font-weight: bold; }
    input[type=text] { 
      width: calc(100% - 180px);
      padding: 8px;
      margin-bottom: 0;
      border: 1px solid #ddd;
      border-radius: 4px;
    }
    select { 
      width: 100px;
      padding: 8px;
      border: 1px solid #ddd;
      border-radius: 4px;
      margin: 0 5px;
    }
    button { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; }
    button:hover { background-color: #45a049; }
    
    .preview-container {
      width: 400px;
      height: 300px;
      margin: 20px auto;
      position: relative;
    }
    .preview {
      width: 400px;
      height: 300px;
      position: relative;
      border: 1px solid #000;
      background: white;
      overflow: hidden;
      transition: all 0.3s ease;
    }
    .preview.inverted {
      background: black;
      color: white;
    }
    .preview-row {
      position: absolute;
      left: 10px;
      right: 10px;
      white-space: nowrap;
      overflow: hidden;
      font-family: monospace;
    }
    .error { color: red; display: none; margin-top: 5px; }
    .row-container { margin-bottom: 10px; }
    .row-group {
      display: grid;
      grid-template-columns: auto 110px 60px;
      gap: 5px;
      margin-bottom: 5px;
      align-items: center;
    }
    .del-btn {
      background-color: #ff4444;
      padding: 8px;
      font-size: 12px;
      height: 35px;
      margin: 0;
      width: 100%;
    }
    .color-options {
      display: flex;
      gap: 10px;
      margin-bottom: 15px;
    }
    .color-btn {
      padding: 10px 20px;
      border: 2px solid #ddd;
      border-radius: 4px;
      cursor: pointer;
      transition: all 0.3s ease;
    }
    .color-btn.normal {
      background: white;
      color: black;
    }
    .color-btn.inverted {
      background: black;
      color: white;
    }
    .color-btn.selected {
      border-color: #4CAF50;
    }
  </style>
  <script>
    const rowHeights = {
      12: 32,
      18: 40,
      24: 48
    };

    function calculateRowPositions() {
      let positions = [];
      let currentY = 10;
      
      for (let i = 0; i < 8; i++) {
        const fontSize = parseInt(document.getElementById(`fontSize${i}`).value);
        const height = rowHeights[fontSize];
        positions.push({
          y: currentY,
          height: height
        });
        currentY += height;
      }
      return positions;
    }

 function updatePreview() {
  const preview = document.querySelector('.preview');
  preview.innerHTML = '';
  
  const positions = calculateRowPositions();
  
  for (let i = 0; i < 8; i++) {
    const text = document.getElementById(`row${i}`).value;
    const fontSize = document.getElementById(`fontSize${i}`).value;
    if (text) {
      const rowDiv = document.createElement('div');
      rowDiv.className = 'preview-row';
      
      // Create a span with double character width
      const textSpan = document.createElement('span');
      
      // Use a non-breaking space for visual representation
      textSpan.innerHTML = text.replace(/ /g, '&nbsp;');
      textSpan.style.letterSpacing = `${fontSize * 0.65}px`; // Adjust letter spacing
      
      rowDiv.appendChild(textSpan);
      rowDiv.style.top = `${positions[i].y}px`;
      rowDiv.style.fontSize = `${fontSize}px`;
      preview.appendChild(rowDiv);
    }
  }
}

    
    function toggleColors(inverted) {
      const preview = document.querySelector('.preview');
      preview.className = inverted ? 'preview inverted' : 'preview';
      
      // Update button styles
      document.querySelector('.color-btn.normal').className = 'color-btn normal' + (!inverted ? ' selected' : '');
      document.querySelector('.color-btn.inverted').className = 'color-btn inverted' + (inverted ? ' selected' : '');
    }
    
    function clearRow(index) {
      document.getElementById(`row${index}`).value = '';
      updatePreview();
    }
    
    function submitForm() {
      const rows = [];
      for (let i = 0; i < 8; i++) {
        rows.push({
          text: document.getElementById(`row${i}`).value,
          fontSize: parseInt(document.getElementById(`fontSize${i}`).value)
        });
      }
      
      const formData = {
        rows: rows,
        border: document.getElementById('border').checked,
        invertColors: document.querySelector('.preview').classList.contains('inverted')
      };
      
      fetch('/update', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(formData)
      }).then(response => {
        if (response.ok) {
          alert('Display updated successfully!');
        }
      });
      return false;
    }
  </script>
</head>
<body>
  <div class="content">
    <h1>Info Display Control</h1>
    <form onsubmit="return submitForm()">
      <div class="form-group">
        <label>Display Colors:</label>
        <div class="color-options">
          <div class="color-btn normal selected" onclick="toggleColors(false)">Black on White</div>
          <div class="color-btn inverted" onclick="toggleColors(true)">White on Black</div>
        </div>
      </div>
      
      <div class="form-group">
        <label>Display Rows:</label>
        <div class="row-container">
          %row_inputs%
        </div>
      </div>
      
      <div class="form-group">
        <label>
          <input type="checkbox" id="border" name="border" checked>
          Show Border
        </label>
      </div>
      
      <div class="preview-container">
        <div class="preview"></div>
      </div>
      <button type="submit">Update Display</button>
    </form>
  </div>
  <script>
    updatePreview();
  </script>
</body>
</html>
)rawliteral";

/**
 * Control power to the e-paper display
 * @param state HIGH to power on, LOW to power off
 */
void epdPower(int state) {
  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, state);
}

/**
 * Initialize the e-paper display with configured settings
 */
void epdInit() {
  epd.init(EPD_INIT_BAUD_RATE, EPD_INIT_RESET, EPD_INIT_DELAY, EPD_INIT_PARTIAL);
  epd.setRotation(0);
  epd.setTextColor(currentSettings.invertColors ? GxEPD_WHITE : GxEPD_BLACK);
  epd.setFullWindow();
}

/**
 * Set the font size for e-paper display
 * @param size Font size (12, 18, or 24 points)
 */
void setFontSize(int size) {
  switch (size) {
    case FONT_SIZE_SMALL:
      epd.setFont(&FreeMonoBold12pt7b);
      break;
    case FONT_SIZE_MEDIUM:
      epd.setFont(&FreeMonoBold18pt7b);
      break;
    case FONT_SIZE_LARGE:
      epd.setFont(&FreeMonoBold24pt7b);
      break;
  }
}

/**
 * Get the row height in pixels for a given font size
 * @param fontSize Font size in points
 * @return Row height in pixels
 */
int getRowHeight(int fontSize) {
  switch (fontSize) {
    case FONT_SIZE_SMALL: return ROW_HEIGHT_SMALL;
    case FONT_SIZE_MEDIUM: return ROW_HEIGHT_MEDIUM;
    case FONT_SIZE_LARGE: return ROW_HEIGHT_LARGE;
    default: return ROW_HEIGHT_MEDIUM;
  }
}

/**
 * Update the e-paper display with current settings
 */
void updateDisplay() {
  epdPower(HIGH);
  epdInit();
  
  // Set background color based on inversion setting
  epd.fillScreen(currentSettings.invertColors ? GxEPD_BLACK : GxEPD_WHITE);
  
  if (currentSettings.border) {
    epd.drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, currentSettings.invertColors ? GxEPD_WHITE : GxEPD_BLACK);
  }
  
  int yPos = getRowHeight(FONT_SIZE_LARGE);
  int margin = DISPLAY_MARGIN;
  
  for (int i = 0; i < currentSettings.MAX_ROWS; i++) {
    if (currentSettings.rows[i].text.length() > 0) {
      setFontSize(currentSettings.rows[i].fontSize);
      epd.setCursor(margin, yPos);
      epd.print(currentSettings.rows[i].text);
      yPos += getRowHeight(currentSettings.rows[i].fontSize);
    }
  }
  
  epd.display();
  epd.hibernate();
  epdPower(LOW);
}

// Rest of the code remains the same until handleUpdate()



/**
 * Generate HTML for row input controls
 * @return HTML string containing input fields for all display rows
 */
String getRowInputsHTML() {
  String html = "";
  for (int i = 0; i < currentSettings.MAX_ROWS; i++) {
    html += "<div class='row-group'>";
    
    // Text input with length limit
    html += "<input type='text' id='row" + String(i) + "' maxlength='" + String(MAX_TEXT_LENGTH) + "' ";
    html += "placeholder='Row " + String(i + 1) + "' ";
    html += "value='" + currentSettings.rows[i].text + "' ";
    html += "onkeyup='updatePreview()'>";
    
    // Font size selector
    html += "<select id='fontSize" + String(i) + "' onchange='updatePreview()'>";
    html += "<option value='" + String(FONT_SIZE_SMALL) + "'" + String(currentSettings.rows[i].fontSize == FONT_SIZE_SMALL ? " selected" : "") + ">Small (" + String(FONT_SIZE_SMALL) + "pt)</option>";
    html += "<option value='" + String(FONT_SIZE_MEDIUM) + "'" + String(currentSettings.rows[i].fontSize == FONT_SIZE_MEDIUM ? " selected" : "") + ">Medium (" + String(FONT_SIZE_MEDIUM) + "pt)</option>";
    html += "<option value='" + String(FONT_SIZE_LARGE) + "'" + String(currentSettings.rows[i].fontSize == FONT_SIZE_LARGE ? " selected" : "") + ">Large (" + String(FONT_SIZE_LARGE) + "pt)</option>";
    html += "</select>";
    
    // Clear button
    html += "<button type='button' class='del-btn' onclick='clearRow(" + String(i) + ")'>DEL</button>";
    html += "</div>";
  }
  return html;
}

/**
 * Generate complete HTML page with current settings
 * @return Complete HTML page as string
 */
String getHTML() {
  String html = String(index_html);
  html.replace("%row_inputs%", getRowInputsHTML());
  return html;
}

/**
 * Handle HTTP request for root page
 */
void handleRoot() {
  server.send(200, "text/html", getHTML());
}

/**
 * Validate display settings before applying them
 * @param settings Settings to validate
 * @return true if settings are valid, false otherwise
 */
bool validateSettings(const DisplaySettings& settings) {
  for (int i = 0; i < settings.MAX_ROWS; i++) {
    if (settings.rows[i].text.length() > MAX_TEXT_LENGTH) {
      return false;
    }
    if (settings.rows[i].fontSize != FONT_SIZE_SMALL && 
        settings.rows[i].fontSize != FONT_SIZE_MEDIUM && 
        settings.rows[i].fontSize != FONT_SIZE_LARGE) {
      return false;
    }
  }
  return true;
}

/**
 * Handle display update requests from web interface
 */
void handleUpdate() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    
    if (!error) {
      DisplaySettings newSettings;
      JsonArray rows = doc["rows"];
      int i = 0;
      for (JsonVariant row : rows) {
        newSettings.rows[i].text = row["text"].as<String>();
        newSettings.rows[i].fontSize = row["fontSize"].as<int>();
        i++;
      }
      
      newSettings.border = doc["border"].as<bool>();
      newSettings.invertColors = doc["invertColors"].as<bool>();
      
      // Validate settings before applying
      if (validateSettings(newSettings)) {
        currentSettings = newSettings;
        updateDisplay();
        server.send(200, "text/plain", "OK");
      } else {
        server.send(400, "text/plain", "Invalid settings");
      }
    } else {
      server.send(400, "text/plain", "Invalid JSON");
    }
  } else {
    server.send(400, "text/plain", "No data received");
  }
}

/**
 * Connect to WiFi with timeout and error handling
 * @return true if connection successful, false otherwise
 */
bool connectWiFi() {
  Serial.print("Connecting to WiFi network: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_TIMEOUT) {
    delay(WIFI_RETRY_DELAY);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println();
    Serial.println("Failed to connect to WiFi!");
    return false;
  }
}

/**
 * Display startup screen with project information
 */
void displayStartupScreen() {
  epdPower(HIGH);
  epdInit();
  epd.fillScreen(GxEPD_WHITE);
  epd.drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, GxEPD_BLACK);
  epd.setFont(&FreeMonoBold24pt7b);
  epd.setCursor(90, 70);
  epd.print("Wireless");
  epd.setCursor(35, 150);
  epd.print("Info-Display");
  epd.setFont(&FreeMonoBold18pt7b);
  epd.setCursor(45, 230);
  epd.print("Fusion Automate"); 
  epd.display();
  delay(2000);
}

/**
 * Display WiFi connection status and IP address
 * @param connected Whether WiFi is connected
 * @param ipAddress IP address string (if connected)
 */
void displayWiFiStatus(bool connected, String ipAddress = "") {
  epd.fillScreen(GxEPD_WHITE);
  epd.drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, GxEPD_BLACK);
  epd.setFont(&FreeMonoBold18pt7b);
  
  if (connected) {
    epd.setCursor(60, 70);
    epd.print("WiFi Connected");
    
    epd.setFont(&FreeMonoBold12pt7b);
    epd.setCursor(80, 150);
    epd.print("IP: " + ipAddress);
  } else {
    epd.setCursor(50, 70);
    epd.print("WiFi Failed");
    epd.setCursor(30, 150);
    epd.print("Check Settings");
  }
  
  epd.setFont(&FreeMonoBold18pt7b);
  epd.setCursor(45, 230);
  epd.print("Fusion Automate");
  epd.display();
  delay(IP_DISPLAY_DURATION);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Wireless Info Display Starting...");

  // Display startup screen
  displayStartupScreen();
  
  // Connect to WiFi
  bool wifiConnected = connectWiFi();
  
  // Display WiFi status
  if (wifiConnected) {
    String ip = WiFi.localIP().toString();
    displayWiFiStatus(true, ip);
  } else {
    displayWiFiStatus(false);
    // Continue anyway for debugging purposes
    Serial.println("Continuing without WiFi for debugging...");
  }
  
  // Clear screen for normal operation
  epd.fillScreen(GxEPD_WHITE);
  epd.hibernate(); 
  epdPower(LOW);
  
  // Setup web server routes only if WiFi is connected
  if (wifiConnected) {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/update", HTTP_POST, handleUpdate);
    server.begin();
    Serial.println("Web server started");
  }
  
  // Initial display update with default settings
  currentSettings.invertColors = false;
  updateDisplay();
  
  Serial.println("Setup completed successfully");
}



/**
 * Main program loop - handle web server requests
 */
void loop() {
  // Only handle web server if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }
  delay(LOOP_DELAY);  // Small delay to prevent watchdog issues
}