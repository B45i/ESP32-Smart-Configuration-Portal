#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// --- Configuration ---
#define TRIGGER_PIN 36
#define CONFIG_FILE "/config.json"
#define AP_SSID "ESP32-Setup"


// Global variables
struct Config {
  char wifi_ssid[33] = "";
  char wifi_password[65] = "";
};

Config config;
AsyncWebServer server(80);
bool inConfigMode = false;

// --- Function Declarations ---
bool loadConfig();
bool saveConfig();
void startAccessPoint();
void setupWebServer();
bool connectToWiFi();
void handleGetConfig(AsyncWebServerRequest *request);
void handlePostConfig(AsyncWebServerRequest *request);

// --- Setup ---
void setup() {
  Serial.begin(115200);
  Serial.println("\n\nBooting...");

  pinMode(TRIGGER_PIN, INPUT);

  if (!LittleFS.begin()) {
    Serial.println("ERROR: Failed to mount LittleFS. Formatting...");
    return;
  }
  Serial.println("LittleFS mounted successfully.");

  bool triggerActive = (digitalRead(TRIGGER_PIN) == LOW);
  if (!triggerActive) {
    Serial.println("INFO: Trigger pin is activated.");
  }


  bool configLoaded = loadConfig();
  Serial.println(configLoaded ? "INFO: Config loaded" : "ERROR: Config Not Loaded");

  if (!triggerActive || !configLoaded || strlen(config.wifi_ssid) == 0) {
    inConfigMode = true;
    Serial.println("INFO: Entering Configuration Mode.");
    startAccessPoint();
    setupWebServer();
    server.begin();
    Serial.println("INFO: Configuration Server Started.");
    Serial.print("INFO: Connect to AP: ");
    Serial.println(AP_SSID);
    Serial.print("INFO: Go to http://");
    Serial.println(WiFi.softAPIP());
    return;
  }
  inConfigMode = false;
  Serial.println("INFO: Configuration found. Attempting to connect to WiFi...");
  if (connectToWiFi()) {
    Serial.println("INFO: WiFi Connected. Proceeding with normal operation.");
    setupWebServer();
    server.begin();
    Serial.println("INFO: Normal operation server running.");
  }
}

// --- Loop ---
void loop() {
  // if (!inConfigMode) {
  //   delay(10);
  // } else {
  //   delay(10);
  // }
}

// --- Function Implementations ---

bool loadConfig() {
  Serial.println("INFO: Loading configuration...");
  if (!LittleFS.exists(CONFIG_FILE)) {
    Serial.println("WARNING: Configuration file not found.");
    return false;
  }

  File configFile = LittleFS.open(CONFIG_FILE, "r");
  if (!configFile) {
    Serial.println("ERROR: Failed to open configuration file for reading.");
    return false;
  }

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    Serial.print("ERROR: Failed to parse configuration file: ");
    Serial.println(error.c_str());
    return false;
  }

  strlcpy(config.wifi_ssid, doc["wifi_ssid"] | "", sizeof(config.wifi_ssid));
  strlcpy(config.wifi_password, doc["wifi_password"] | "", sizeof(config.wifi_password));
  Serial.print("INFO: Loaded SSID: ");
  Serial.println(config.wifi_ssid);

  if (strlen(config.wifi_ssid) == 0) {
    Serial.println("WARNING: Loaded configuration has empty SSID.");
    return false;
  }

  return true;
}

bool saveConfig() {
  Serial.println("INFO: Saving configuration...");
  StaticJsonDocument<256> doc;

  doc["wifi_ssid"] = config.wifi_ssid;
  doc["wifi_password"] = config.wifi_password;

  File configFile = LittleFS.open(CONFIG_FILE, "w");
  if (!configFile) {
    Serial.println("ERROR: Failed to open configuration file for writing.");
    return false;
  }

  if (serializeJson(doc, configFile) == 0) {
    Serial.println("ERROR: Failed to write to configuration file.");
    configFile.close();
    return false;
  }

  configFile.close();
  Serial.println("INFO: Configuration saved successfully.");
  return true;
}

void startAccessPoint() {
  Serial.println("INFO: Starting Access Point (AP)...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);
  Serial.print("INFO: AP SSID: ");
  Serial.println(AP_SSID);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("INFO: AP IP address: ");
  Serial.println(apIP);
}

void setupWebServer() {
  Serial.println("INFO: Setting up Web Server routes...");
  server.serveStatic("/", LittleFS, "./").setDefaultFile("index.html").setCacheControl("max-age=6000");
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handlePostConfig);
}

void handleGetConfig(AsyncWebServerRequest *request) {
  Serial.println("INFO: GET /api/config received.");
  StaticJsonDocument<256> doc;
  doc["wifi_ssid"] = config.wifi_ssid;
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  request->send(200, "application/json", jsonResponse);
}

void handlePostConfig(AsyncWebServerRequest *request) {
  Serial.println("INFO: POST /api/config received.");

  if (request->hasParam("wifi_ssid", true)) {
    strncpy(config.wifi_ssid, request->getParam("wifi_ssid", true)->value().c_str(), sizeof(config.wifi_ssid) - 1);
    config.wifi_ssid[sizeof(config.wifi_ssid) - 1] = '\0';
    Serial.print("INFO: Received SSID: ");
    Serial.println(config.wifi_ssid);
  }

  if (request->hasParam("wifi_password", true)) {
    strncpy(config.wifi_password, request->getParam("wifi_password", true)->value().c_str(), sizeof(config.wifi_password) - 1);
    config.wifi_password[sizeof(config.wifi_password) - 1] = '\0';
    Serial.println("INFO:  Received password: ");
    Serial.println(config.wifi_password);

  }

  if (saveConfig()) {
    request->send(200, "text/plain", "Configuration updated. Restarting...");
    Serial.println("INFO: Configuration saved. Restarting in 3 seconds...");
    delay(3000);
    ESP.restart();
  } else {
    request->send(500, "text/plain", "Error saving configuration.");
  }
}


bool connectToWiFi() {
  if (strlen(config.wifi_ssid) == 0) {
    Serial.println("ERROR: Cannot connect to WiFi, SSID is empty.");
    return false;
  }

  Serial.print("INFO: Connecting to WiFi SSID: ");
  Serial.println(config.wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifi_ssid, config.wifi_password);


  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startTime > 20000) {
      Serial.println("\nERROR: WiFi connection timed out.");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      return false;
    }
  }

  Serial.println("\nINFO: WiFi connected!");
  Serial.print("INFO: IP Address: ");
  Serial.println(WiFi.localIP());
  return true;
}