#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h> 
#include <ArduinoOTA.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 
#define SCREEN_ADDRESS 0x3C 

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
WebServer server(80);
Preferences preferences;

TwoWire I2Cbus = TwoWire(0);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2Cbus, OLED_RESET);

// Eyes Parameters
const int eyeRadius = 12;
const int leftEyeX = 40;
const int rightEyeX = 88;
const int eyeY = 32;

// Mood/Expressions Definitions
enum EyeMood { MOOD_NORMAL, MOOD_HAPPY, MOOD_LOADING };
EyeMood currentMood = MOOD_NORMAL;

String ssid = "";
String password = "";
bool isConnected = false;
bool apMode = false;
bool ipShown = false;

// Non-blocking Timers
unsigned long lastWiFiCheck = 0;
unsigned long lastEyeAction = 0;
unsigned long eyeDelay = 2000;
unsigned long ipScreenStartTime = 0;
unsigned long apStartTime = 0;
int eyeState = 0; 
int loadingOffset = 0;

void handleEyesAutomation();
void drawEyes();
void checkWiFiConnection();
void startAPMode();

// Dynamic HTML Portal with Wi-Fi Scanner built-in
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:Arial;text-align:center;background:#222;color:#fff;padding:20px;}";
  html += "select, input[type=password]{width:90%;padding:12px;margin:8px 0;border:1px solid #ccc;border-radius:4px;font-size:16px;}";
  html += "input[type=submit]{width:90%;background-color:#4CAF50;color:white;padding:14px 20px;margin:8px 0;border:none;border-radius:4px;cursor:pointer;font-size:16px;}</style></head>";
  html += "<body><h2>ESP32-S3 Wi-Fi Setup</h2><p>Apna Wi-Fi Network Select Karein:</p>";
  html += "<form action='/save' method='POST'>";
  html += "<select name='ssid'>";
  
  // Networks Scan karke drop-down list banana
  int n = WiFi.scanNetworks();
  if (n == 0) {
    html += "<option value=''>No networks found</option>";
  } else {
    for (int i = 0; i < n; ++i) {
      html += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + "dBm)</option>";
    }
  }
  
  html += "</select><br>";
  html += "<input type='password' name='pass' placeholder='Password'><br>";
  html += "<input type='submit' value='Save & Connect'>";
  html += "</form></body></html>";
  
  server.send(200, "text/html", html);
}

void handleSave() {
  String req_ssid = server.arg("ssid");
  String req_pass = server.arg("pass");
  
  if (req_ssid.length() > 0) {
    preferences.begin("wifi-creds", false);
    preferences.putString("ssid", req_ssid);
    preferences.putString("password", req_pass);
    preferences.end();
    
    server.send(200, "text/html", "<h2>Settings Saved! Reconnecting...</h2>");
    delay(2000);
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);
  I2Cbus.begin(8, 9, 400000);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    for(;;); 
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Saved Wi-Fi details read karna memory se (Yahan se personal IDs hata di hain)
  preferences.begin("wifi-creds", true);
  ssid = preferences.getString("ssid", ""); 
  password = preferences.getString("password", ""); 
  preferences.end();

  // Agar memory pehle se khali hai (First boot), toh direct Hotspot mode chalu hoga
  if(ssid == "") {
    checkWiFiConnection(); 
  } else {
    display.setCursor(5, 10);
    display.println("Connecting WiFi...");
    display.display();

    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 12) {
      delay(500);
      display.print(".");
      display.display();
      attempts++;
    }
    checkWiFiConnection();
  }

  // ==========================================
  // PAST YOUR PROJECT SETUP() CODE HERE:
  // ==========================================
  
}

void loop() {
  // 1. Har 5 second me network check
  if (millis() - lastWiFiCheck >= 5000) {
    lastWiFiCheck = millis();
    checkWiFiConnection();
  }

  // 2. Hotspot Mode active hai toh network configuration chalega
  if (apMode) {
    dnsServer.processNextRequest();
    server.handleClient();
    
    if (millis() - apStartTime >= 60000) {
      currentMood = MOOD_NORMAL;
      handleEyesAutomation(); 
    }
    
    // ==========================================
    // PAST YOUR PROJECT LOOP() CODE HERE (IF OFFLINE/AP MODE):
    // ==========================================
    return;
  }

  // 3. Online Mode Handling
  if (isConnected) {
    ArduinoOTA.handle(); 
    
    if (!ipShown) {
      currentMood = MOOD_HAPPY; 
      drawEyes();
      if (millis() - ipScreenStartTime >= 5000) {
        ipShown = true;
        display.clearDisplay();
        display.display(); 
      }
    }

    // ==========================================
    // PAST YOUR PROJECT LOOP() CODE HERE (IF ONLINE):
    // ==========================================
    
    if (currentMood == MOOD_LOADING) {
      drawEyes();
      delay(50);
    }
    
    delay(1);
    return; 
  }
}

void checkWiFiConnection() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!isConnected) { 
      isConnected = true;
      apMode = false;
      ipShown = false;
      ipScreenStartTime = millis();
      WiFi.mode(WIFI_STA); 
      
      ArduinoOTA.setHostname("ESP32S3-Wireless");
      ArduinoOTA.onStart([]() {
        currentMood = MOOD_LOADING; 
      });
      ArduinoOTA.begin();
    }
  } 
  else {
    if (isConnected || (!isConnected && !apMode)) { 
      isConnected = false;
      apMode = true;
      apStartTime = millis();
      WiFi.disconnect();
      
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(5, 5);
      display.println("Setup AP Active!");
      display.setCursor(5, 20);
      display.println("Connect Mobile to:"); 
      display.setCursor(5, 35);
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); 
      display.println(" ESP32-S3-Setup ");
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(5, 50);
      display.println("Open IP: 192.168.4.1");
      display.display();

      WiFi.mode(WIFI_AP_STA); 
      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
      WiFi.softAP("ESP32-S3-Setup"); 

      dnsServer.start(DNS_PORT, "*", apIP);
      server.on("/", handleRoot);
      server.on("/save", HTTP_POST, handleSave);
      server.onNotFound(handleRoot); 
      server.begin();
    }
  }
}

void handleEyesAutomation() {
  if (millis() - lastEyeAction >= eyeDelay) {
    lastEyeAction = millis();
    if (eyeState == 0) {
      eyeState = 1;
      eyeDelay = 150; 
    } else {
      eyeState = 0;
      eyeDelay = random(2000, 4000); 
    }
  }
  drawEyes();
}

void drawEyes() {
  display.clearDisplay();
  
  if (currentMood == MOOD_HAPPY) {
    display.drawCircle(leftEyeX, eyeY, eyeRadius, SSD1306_WHITE);
    display.fillRect(leftEyeX - eyeRadius - 1, eyeY, (eyeRadius * 2) + 2, eyeRadius + 1, SSD1306_BLACK);
    display.drawCircle(rightEyeX, eyeY, eyeRadius, SSD1306_WHITE);
    display.fillRect(rightEyeX - eyeRadius - 1, eyeY, (eyeRadius * 2) + 2, eyeRadius + 1, SSD1306_BLACK);
    
    display.setTextSize(1);
    display.setCursor(22, 53);
    display.print("IP: "); display.print(WiFi.localIP());
  } 
  else if (currentMood == MOOD_LOADING) {
    display.drawCircle(leftEyeX, eyeY, eyeRadius, SSD1306_WHITE);
    display.drawCircle(rightEyeX, eyeY, eyeRadius, SSD1306_WHITE);
    
    loadingOffset++;
    int pX = sin(loadingOffset * 0.4) * 4;
    int pY = cos(loadingOffset * 0.4) * 4;
    
    display.fillCircle(leftEyeX + pX, eyeY + pY, 4, SSD1306_WHITE);
    display.fillCircle(rightEyeX + pX, eyeY + pY, 4, SSD1306_WHITE);
    
    display.setTextSize(1);
    display.setCursor(34, 53);
    display.print("Uploading...");
  } 
  else {
    if (eyeState == 1) { 
      display.drawFastHLine(leftEyeX - eyeRadius, eyeY, eyeRadius * 2, SSD1306_WHITE);
      display.drawFastHLine(rightEyeX - eyeRadius, eyeY, eyeRadius * 2, SSD1306_WHITE);
    } else { 
      display.fillCircle(leftEyeX, eyeY, eyeRadius, SSD1306_WHITE);
      display.fillCircle(rightEyeX, eyeY, eyeRadius, SSD1306_WHITE);
    }
  }
  
  display.display();
}

// ====================================================================
// past project here 
// ====================================================================
