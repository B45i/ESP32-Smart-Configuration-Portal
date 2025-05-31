#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <DNSServer.h>

// --- Configuration ---
#define TRIGGER_PIN 36
#define CONFIG_FILE "/config.json"
#define AP_SSID "ESP32-Setup"

// --- Global variables ---
struct Config {
  char wifi_ssid[33] = "";
  char wifi_password[65] = "";
};

Config config;
AsyncWebServer server(80);
DNSServer dnsServer;
bool inConfigMode = false;

// --- Function Declarations ---
bool loadConfig();
bool saveConfig();
void startAccessPoint();
void startConfigMode(); // New function to start configuration mode
void setupWebServer();
bool connectToWiFi();
void handleGetConfig(AsyncWebServerRequest *request);
void handlePostConfig(AsyncWebServerRequest *request);

// --- Setup ---
void setup() {
  Serial.begin(115200);
  Serial.println("\n\nBooting...");

  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  if (!LittleFS.begin()) {
    Serial.println("ERROR: Failed to mount LittleFS. Formatting...");
    LittleFS.format();
    return;
  }
  Serial.println("LittleFS mounted successfully.");

  bool triggerActive = (digitalRead(TRIGGER_PIN) == LOW);
  if (triggerActive) {
    Serial.println("INFO: Trigger pin activated, forcing Config Mode.");
  }

  bool configLoaded = loadConfig();
  
  // Condition to enter configuration mode
  if (triggerActive || !configLoaded) {
    startConfigMode();
    return; // Stop setup() here for config mode
  }

  // If config was loaded, try to connect
  Serial.println("INFO: Configuration found. Attempting to connect to WiFi...");
  if (connectToWiFi()) {
    inConfigMode = false;
    Serial.println("INFO: WiFi Connected. Proceeding with normal operation.");
    // Your normal operation code would start here
  } else {
    // If connection fails, enter config mode as a fallback
    Serial.println("ERROR: Failed to connect with saved credentials.");
    startConfigMode();
  }
}

// --- Loop ---
void loop() {
  if (inConfigMode) {
    dnsServer.processNextRequest();
  }
  delay(10);
}

// --- Function Implementations ---

/**
 * @brief Consolidates all actions required to start the configuration portal.
 */
void startConfigMode() {
  inConfigMode = true;
  Serial.println("INFO: Entering Configuration Mode.");
  
  startAccessPoint(); // Starts the WiFi Access Point and the DNS server
  setupWebServer();   // Sets up the web server routes
  server.begin();     // Starts the web server
  
  Serial.println("INFO: Configuration server with Captive Portal is active.");
  Serial.print("INFO: Connect to AP: ");
  Serial.println(AP_SSID);
  Serial.print("INFO: Go to http://");
  Serial.println(WiFi.softAPIP());
}

bool loadConfig() {
  if (!LittleFS.exists(CONFIG_FILE)) {
    Serial.println("WARN: Configuration file not found.");
    return false;
  }

  File configFile = LittleFS.open(CONFIG_FILE, "r");
  if (!configFile) {
    Serial.println("ERROR: Failed to open config file for reading.");
    return false;
  }

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    Serial.print("ERROR: Failed to parse config file: ");
    Serial.println(error.c_str());
    return false;
  }

  strlcpy(config.wifi_ssid, doc["wifi_ssid"] | "", sizeof(config.wifi_ssid));
  strlcpy(config.wifi_password, doc["wifi_password"] | "", sizeof(config.wifi_password));
  
  if (strlen(config.wifi_ssid) == 0) {
    Serial.println("WARN: Loaded configuration has an empty SSID.");
    return false;
  }
  
  Serial.print("INFO: Loaded SSID from config: ");
  Serial.println(config.wifi_ssid);
  return true;
}

bool saveConfig() {
  StaticJsonDocument<256> doc;
  doc["wifi_ssid"] = config.wifi_ssid;
  doc["wifi_password"] = config.wifi_password;

  File configFile = LittleFS.open(CONFIG_FILE, "w");
  if (!configFile) {
    Serial.println("ERROR: Failed to open config file for writing.");
    return false;
  }

  if (serializeJson(doc, configFile) == 0) {
    Serial.println("ERROR: Failed to write to config file.");
    configFile.close();
    return false;
  }

  configFile.close();
  Serial.println("INFO: Configuration saved successfully.");
  return true;
}

void startAccessPoint() {
  Serial.println("INFO: Starting Access Point (AP) and DNS server...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);
  IPAddress apIP = WiFi.softAPIP();
  dnsServer.start(53, "*", apIP);
}

void setupWebServer() {
  Serial.println("INFO: Setting up Web Server routes for configuration...");
  server.onNotFound([](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handlePostConfig);
}

void handleGetConfig(AsyncWebServerRequest *request) {
  StaticJsonDocument<256> doc;
  doc["wifi_ssid"] = config.wifi_ssid;
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  request->send(200, "application/json", jsonResponse);
}

void handlePostConfig(AsyncWebServerRequest *request) {
  if (request->hasParam("wifi_ssid", true)) {
    strlcpy(config.wifi_ssid, request->getParam("wifi_ssid", true)->value().c_str(), sizeof(config.wifi_ssid));
    Serial.print("INFO: Received SSID for saving: ");
    Serial.println(config.wifi_ssid);
  }
  if (request->hasParam("wifi_password", true)) {
    strlcpy(config.wifi_password, request->getParam("wifi_password", true)->value().c_str(), sizeof(config.wifi_password));
    Serial.println("INFO: Received password for saving.");
  }
  if (saveConfig()) {
    request->send(200, "text/plain", "Configuration updated. Restarting...");
    Serial.println("INFO: Restarting in 3 seconds...");
    delay(3000);
    ESP.restart();
  } else {
    request->send(500, "text/plain", "Error saving configuration.");
  }
}

bool connectToWiFi() {
  Serial.print("INFO: Connecting to WiFi SSID: ");
  Serial.println(config.wifi_ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifi_ssid, config.wifi_password);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startTime > 20000) { // 20-second connection timeout
      Serial.println("\nERROR: WiFi connection timed out.");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      return false;
    }
  }
  return true;
}