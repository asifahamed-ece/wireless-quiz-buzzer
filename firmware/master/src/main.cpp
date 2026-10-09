/**
 * ═══════════════════════════════════════════════════════════════════
 * PROJECT: Wireless Quiz Buzzer - Master
 * VERSION: 1.1
 * DATE: January 2026
 *
 * SYSTEM SPECIFICATIONS:
 * - Max Teams: 10
 * - Communication: ESP-NOW (2.4GHz), receive only
 * - Range: ~50 meters
 * - Ranking: by arrival order at this unit, not by a computed response time
 * - Dashboard: real-time WebSocket + LittleFS static files
 * - Battery Monitoring: 3-zone system, from unit-reported values
 * - Two-Phase System: LISTEN -> READY (ANSWERED is a dashboard-side view)
 *
 * DEVELOPER: ASIF AHAMED
 * DEPARTMENT: Electronics and Communication Engineering
 * BATCH: 2023-2027
 * INSTITUTION: Rajalakshmi Engineering College, Chennai
 * ═══════════════════════════════════════════════════════════════════
 */

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// WiFi Channel
#define WIFI_CHANNEL 1

// OLED Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin Definitions
#define RESET_BUTTON 13
#define LED_WINNER 2
#define LED_SYNC 15
#define LED_READY 16
#define POWER_SWITCH 25

// System State
#define MAX_TEAMS 10
bool teamConnected[MAX_TEAMS] = {false};
int winnerTeam = 0;
unsigned long winnerTimestamp = 0;
bool quizActive = false;  // ✅ Start in LISTEN mode

// Power switch state
bool systemEnabled = false;
bool lastSwitchState = false;
unsigned long lastSwitchCheck = 0;
#define SWITCH_CHECK_INTERVAL 200

unsigned long lastResponseTime = 0;
#define BATCH_WINDOW 200 // 200ms window to batch responses

// Heartbeat Monitoring
#define HEARTBEAT_TIMEOUT 5000
unsigned long lastHeartbeat[MAX_TEAMS] = {0};

// Battery Storage
int teamBatteryZone[MAX_TEAMS] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
float teamBatteryPercent[MAX_TEAMS] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100};

// Interrupt variables
volatile bool resetPressed = false;
volatile unsigned long lastResetTime = 0;

// LED Control Variables
unsigned long lastSyncBlink = 0;
bool syncBlinkState = false;
unsigned long resetFlashStart = 0;
bool resetFlashActive = false;
#define SYNC_BLINK_INTERVAL 500
#define RESET_FLASH_DURATION 1000

// ESP-NOW Data Structure
typedef struct {
  int teamID;
  unsigned long timestamp;
  bool buttonPressed;
  bool isHeartbeat;
  int batteryZone;
  float batteryPercent;
} BuzzerData;

BuzzerData incomingData;

// WebSocket Server
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Response Tracking
#define MAX_RESPONSES 10
struct TeamResponse {
  int teamID;
  unsigned long timestamp;
  int position;
};

TeamResponse responseOrder[MAX_RESPONSES];
int responseCount = 0;
bool teamHasResponded[MAX_TEAMS] = {false};

// ═══════════════════════════════════════════════════════════
//  SMART BROADCAST - Priority-based throttling
// ═══════════════════════════════════════════════════════════

String lastBroadcastData = "";
unsigned long lastBroadcastTime = 0;
#define MIN_BROADCAST_INTERVAL 1000 // For non-critical updates

// Priority broadcast - ignores throttle for critical events
void broadcastUpdatePriority() {
  if(ws.count() == 0) return;
  
  JsonDocument doc;
  
  // Teams status
  JsonArray teams = doc["teams"].to<JsonArray>();
  int connectedCount = 0, greenCount = 0, yellowCount = 0, redCount = 0;
  
  for(int i = 0; i < MAX_TEAMS; i++) {
    JsonObject team = teams.add<JsonObject>();
    team["id"] = i + 1;
    team["connected"] = teamConnected[i];
    team["zone"] = teamBatteryZone[i];
    team["percent"] = teamBatteryPercent[i];
    
    if(teamConnected[i]) {
      connectedCount++;
      if(teamBatteryZone[i] == 2) greenCount++;
      else if(teamBatteryZone[i] == 1) yellowCount++;
      else redCount++;
    }
  }
  
  doc["winnerTeam"] = winnerTeam;
  doc["winnerTime"] = winnerTimestamp / 1000.0;
  
  // Responses
  JsonArray responses = doc["responses"].to<JsonArray>();
  for(int i = 0; i < responseCount; i++) {
    JsonObject resp = responses.add<JsonObject>();
    resp["position"] = i + 1;
    resp["team"] = responseOrder[i].teamID;
    resp["time"] = responseOrder[i].timestamp / 1000.0;
  }
  
  doc["connectedCount"] = connectedCount;
  doc["greenCount"] = greenCount;
  doc["yellowCount"] = yellowCount;
  doc["redCount"] = redCount;
  doc["responseCount"] = responseCount;
  doc["quizActive"] = quizActive;
  doc["channel"] = WIFI_CHANNEL;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // ✅ DEBUG: Print quiz state
  Serial.print("📡 PRIORITY Broadcast → quizActive=");
  Serial.print(quizActive ? "TRUE" : "FALSE");
  Serial.print(" | Winner=");
  Serial.print(winnerTeam);
  Serial.print(" | Clients=");
  Serial.println(ws.count());
  
  ws.textAll(jsonString);
  lastBroadcastData = jsonString;
  lastBroadcastTime = millis();
}

// Normal broadcast - throttled for non-critical updates
void broadcastUpdateSafe() {
  unsigned long now = millis();
  
  // Throttle non-critical updates
  if(now - lastBroadcastTime < MIN_BROADCAST_INTERVAL) {
    Serial.println("⏳ Broadcast throttled (non-critical)");
    return;
  }
  
  if(ws.count() == 0) return;
  
  JsonDocument doc;
  
  // Teams status
  JsonArray teams = doc["teams"].to<JsonArray>();
  int connectedCount = 0, greenCount = 0, yellowCount = 0, redCount = 0;
  
  for(int i = 0; i < MAX_TEAMS; i++) {
    JsonObject team = teams.add<JsonObject>();
    team["id"] = i + 1;
    team["connected"] = teamConnected[i];
    team["zone"] = teamBatteryZone[i];
    team["percent"] = teamBatteryPercent[i];
    
    if(teamConnected[i]) {
      connectedCount++;
      if(teamBatteryZone[i] == 2) greenCount++;
      else if(teamBatteryZone[i] == 1) yellowCount++;
      else redCount++;
    }
  }
  
  doc["winnerTeam"] = winnerTeam;
  doc["winnerTime"] = winnerTimestamp / 1000.0;
  
  JsonArray responses = doc["responses"].to<JsonArray>();
  for(int i = 0; i < responseCount; i++) {
    JsonObject resp = responses.add<JsonObject>();
    resp["position"] = i + 1;
    resp["team"] = responseOrder[i].teamID;
    resp["time"] = responseOrder[i].timestamp / 1000.0;
  }
  
  doc["connectedCount"] = connectedCount;
  doc["greenCount"] = greenCount;
  doc["yellowCount"] = yellowCount;
  doc["redCount"] = redCount;
  doc["responseCount"] = responseCount;
  doc["quizActive"] = quizActive;
  doc["channel"] = WIFI_CHANNEL;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Check if data changed
  if(jsonString == lastBroadcastData) {
    Serial.println("📡 No changes detected");
    return;
  }
  
  ws.textAll(jsonString);
  lastBroadcastData = jsonString;
  lastBroadcastTime = now;
  
  Serial.print("📡 Broadcast → ");
  Serial.print(ws.count());
  Serial.println(" clients");
}

// ═══════════════════════════════════════════════════════════
//  POWER SWITCH & INTERRUPT
// ═══════════════════════════════════════════════════════════

bool readPowerSwitch() {
  return (digitalRead(POWER_SWITCH) == LOW);
}

void IRAM_ATTR resetButtonISR() {
  unsigned long currentTime = millis();
  // ✅ Increased debounce to catch press/release bounces
  if (currentTime - lastResetTime > 100) {
    resetPressed = true;
    lastResetTime = currentTime;
  }
}

// ═══════════════════════════════════════════════════════════
//  DISPLAY FUNCTIONS
// ═══════════════════════════════════════════════════════════

void displayStandbyScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("QUIZ BUZZER");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(25, 20);
  display.println("STAND");
  display.setCursor(40, 38);
  display.println("BY");
  display.setTextSize(1);
  display.setCursor(5, 56);
  display.println("Turn ON Switch");
  display.display();
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("QUIZ BUZZER");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  display.setCursor(0, 12);
  display.print("CH:");
  display.print(WIFI_CHANNEL);
  
  int connectedCount = 0;
  for(int i = 0; i < MAX_TEAMS; i++) {
    if(teamConnected[i]) connectedCount++;
  }
  display.setCursor(40, 12);
  display.print("Teams:");
  display.print(connectedCount);
  display.println("/10");
  display.drawLine(0, 22, 128, 22, SSD1306_WHITE);
  
  // ✅ Phase-based display
  if(!quizActive) {
    // LISTEN mode
    display.setTextSize(2);
    display.setCursor(15, 28);
    display.print("LISTEN");
    display.setTextSize(3);
    display.setCursor(50, 45);
    display.print("?");
  } else if(winnerTeam > 0) {
    // Winner shown
    display.setTextSize(2);
    display.setCursor(10, 28);
    display.print("BUZZED!");
    display.setCursor(30, 48);
    display.print("T");
    display.print(winnerTeam);
  } else {
    // READY mode
    display.setTextSize(2);
    display.setCursor(25, 35);
    display.println("READY");
    display.setTextSize(1);
    display.setCursor(30, 52);
    display.print("Press!");
  }
  
  display.display();
}

// ═══════════════════════════════════════════════════════════
//  LED CONTROL
// ═══════════════════════════════════════════════════════════

void updateLEDs() {
  unsigned long currentTime = millis();
  
  if(!systemEnabled) {
    digitalWrite(LED_READY, HIGH);
  } else {
    if(resetFlashActive) {
      if(currentTime - resetFlashStart < RESET_FLASH_DURATION) {
        digitalWrite(LED_READY, HIGH);
      } else {
        digitalWrite(LED_READY, LOW);
        resetFlashActive = false;
      }
    } else {
      digitalWrite(LED_READY, LOW);
    }
  }
  
  if(!systemEnabled) {
    digitalWrite(LED_SYNC, LOW);
  } else {
    int connectedCount = 0;
    for(int i = 0; i < MAX_TEAMS; i++) {
      if(teamConnected[i]) connectedCount++;
    }
    if(connectedCount > 0) {
      digitalWrite(LED_SYNC, HIGH);
    } else {
      if(currentTime - lastSyncBlink >= SYNC_BLINK_INTERVAL) {
        lastSyncBlink = currentTime;
        syncBlinkState = !syncBlinkState;
        digitalWrite(LED_SYNC, syncBlinkState);
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════
//  ESP-NOW CALLBACK
// ═══════════════════════════════════════════════════════════

void OnDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if(!systemEnabled) return;
  
  memcpy(&incomingData, data, sizeof(incomingData));
  int teamID = incomingData.teamID;
  
  if(teamID < 1 || teamID > MAX_TEAMS) {
    Serial.println("❌ Invalid team ID!");
    return;
  }
  
  lastHeartbeat[teamID - 1] = millis();
  teamBatteryZone[teamID - 1] = incomingData.batteryZone;
  teamBatteryPercent[teamID - 1] = incomingData.batteryPercent;
  
  if(!teamConnected[teamID - 1]) {
    teamConnected[teamID - 1] = true;
    Serial.print("✅ Team ");
    Serial.print(teamID);
    Serial.print(" connected! | Battery: ");
    Serial.print((int)incomingData.batteryPercent);
    Serial.print("% (");
    if(incomingData.batteryZone == 2) Serial.print("🔋 GREEN");
    else if(incomingData.batteryZone == 1) Serial.print("⚠️ YELLOW");
    else Serial.print("🪫 RED");
    Serial.println(")");
    updateDisplay();
    broadcastUpdateSafe();
  }
  
  if(incomingData.isHeartbeat) return;
  
  // ✅ Only accept buzzer in READY mode
  if(incomingData.buttonPressed && quizActive) {
    if(teamHasResponded[teamID - 1]) {
      Serial.print("⚠️ Team ");
      Serial.print(teamID);
      Serial.println(" already responded");
      return;
    }
    
    unsigned long masterTimestamp = micros();
    teamHasResponded[teamID - 1] = true;
    
    // First press
    if(winnerTeam == 0) {
      winnerTeam = teamID;
      winnerTimestamp = masterTimestamp;
      digitalWrite(LED_WINNER, HIGH);
      
      Serial.println("=====================================");
      Serial.print("🏆 FIRST TO BUZZ: Team ");
      Serial.println(teamID);
      Serial.print("⏱️  Timestamp: ");
      Serial.print(masterTimestamp);
      Serial.println(" μs");
      Serial.println("=====================================");
    }
    
    // Log all responses
    if(responseCount < MAX_RESPONSES) {
      responseOrder[responseCount].teamID = teamID;
      responseOrder[responseCount].timestamp = masterTimestamp;
      responseOrder[responseCount].position = responseCount + 1;
      responseCount++;
      
      Serial.print("📝 Response #");
      Serial.print(responseCount);
      Serial.print(": Team ");
      Serial.println(teamID);
    }
    
    updateDisplay();
    lastResponseTime = millis();
  } else if(incomingData.buttonPressed && !quizActive) {
    // ✅ Buzzer pressed in LISTEN mode - ignore it
    Serial.print("🚫 Team ");
    Serial.print(teamID);
    Serial.println(" buzzed in LISTEN mode (ignored)");
  }
}

// ═══════════════════════════════════════════════════════════
//  POWER SWITCH HANDLER
// ═══════════════════════════════════════════════════════════

void checkPowerSwitch() {
  bool currentState = readPowerSwitch();
  
  if(currentState != lastSwitchState) {
    lastSwitchState = currentState;
    
    if(currentState) {
      systemEnabled = true;
      Serial.println("\n🟢 SYSTEM ENABLED");
      
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP("QuizBuzzer_AP", "12345678", WIFI_CHANNEL);
      esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
      
      Serial.print("📡 AP IP: ");
      Serial.println(WiFi.softAPIP());
      
      if(esp_now_init() == ESP_OK) {
        esp_now_register_recv_cb(OnDataRecv);
        Serial.println("✅ ESP-NOW initialized");
      }
      
      server.begin();
      Serial.println("✅ Web server started");
      
      // ✅ Start in LISTEN mode
      quizActive = false;
      Serial.println("📢 Starting in LISTEN mode");
      Serial.println("   Press RESET to enter READY mode\n");
      
      updateDisplay();
      broadcastUpdateSafe();
      
    } else {
      systemEnabled = false;
      Serial.println("\n🔴 SYSTEM STANDBY");
      digitalWrite(LED_WINNER, LOW);
      displayStandbyScreen();
      server.end();
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_OFF);
    }
  }
}

// ═══════════════════════════════════════════════════════════
//  WEBSOCKET HANDLERS
// ═══════════════════════════════════════════════════════════

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                       AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if(type == WS_EVT_CONNECT) {
    Serial.printf("🔌 WebSocket client #%u connected\n", client->id());
    broadcastUpdateSafe(); // Send initial data
  } else if(type == WS_EVT_DISCONNECT) {
    Serial.printf("🔌 WebSocket client #%u disconnected\n", client->id());
  }
}

// ═══════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  
  pinMode(RESET_BUTTON, INPUT_PULLUP);
  pinMode(LED_WINNER, OUTPUT);
  pinMode(LED_SYNC, OUTPUT);
  pinMode(LED_READY, OUTPUT);
  pinMode(POWER_SWITCH, INPUT_PULLUP);
  
  digitalWrite(LED_WINNER, LOW);
  digitalWrite(LED_SYNC, LOW);
  digitalWrite(LED_READY, LOW);
  
  attachInterrupt(digitalPinToInterrupt(RESET_BUTTON), resetButtonISR, FALLING);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED failed!");
    while(1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();
  delay(1000);
  
  if(!LittleFS.begin(true)) {
    Serial.println("❌ LittleFS Mount Failed!");
    while(1);
  }
  Serial.println("✅ LittleFS initialized!");
  
  // WebSocket setup
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  
  // Serve static files
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/script.js", "application/javascript");
  });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(204);
  });
  
  systemEnabled = readPowerSwitch();
  lastSwitchState = systemEnabled;
  
  Serial.println("\n╔═══════════════════════════════════════════╗");
  Serial.println("║  QUIZ BUZZER - TWO PHASE SYSTEM           ║");
  Serial.println("╚═══════════════════════════════════════════╝");
  
  if(!systemEnabled) {
    Serial.println("🔴 STANDBY mode");
    displayStandbyScreen();
    return;
  }
  
  Serial.println("🟢 Starting system...");
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("QuizBuzzer_AP", "12345678", WIFI_CHANNEL);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  
  Serial.print("📡 AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("🔑 MAC: ");
  Serial.println(WiFi.macAddress());
  
  if(esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed!");
    return;
  }
  Serial.println("✅ ESP-NOW initialized");
  esp_now_register_recv_cb(OnDataRecv);
  
  server.begin();
  Serial.println("🌐 WebSocket server started!");
  Serial.println("   URL: http://192.168.4.1\n");
  
  // ✅ Start in LISTEN mode
  quizActive = false;
  Serial.println("📢 Starting in LISTEN mode");
  Serial.println("   Press RESET to enter READY mode\n");
  
  updateDisplay();
}

// ═══════════════════════════════════════════════════════════
//  MAIN LOOP 
// ═══════════════════════════════════════════════════════════

void loop() {
  // Aggressive WebSocket cleanup
  static unsigned long lastCleanup = 0;
  if(millis() - lastCleanup > 5000) {
    lastCleanup = millis();
    ws.cleanupClients();
    
    if(ws.count() > 0) {
      Serial.print("📡 Active clients: ");
      Serial.println(ws.count());
    }
  }
  
  updateLEDs();
  
  if(millis() - lastSwitchCheck >= SWITCH_CHECK_INTERVAL) {
    lastSwitchCheck = millis();
    checkPowerSwitch();
  }
  
  if(!systemEnabled) {
    delay(100);
    return;
  }
  
  // ═══════════════════════════════════════════════════════════
  // 🔥 BATCHED BROADCAST: Send after 200ms window
  // ═══════════════════════════════════════════════════════════
  static bool batchPending = false;
  if(lastResponseTime > 0 && !batchPending) {
    batchPending = true;
  }
  
  if(batchPending && (millis() - lastResponseTime > BATCH_WINDOW)) {
    batchPending = false;
    lastResponseTime = 0;
    broadcastUpdatePriority(); // FORCE broadcast, ignore throttle
    Serial.println("📦 Batch broadcast sent!");
  }
  
  // Check for heartbeat timeouts (throttled)
  static unsigned long lastTimeoutCheck = 0;
  if(millis() - lastTimeoutCheck > 2000) {
    lastTimeoutCheck = millis();
    bool statusChanged = false;
    
    for(int i = 0; i < MAX_TEAMS; i++) {
      if(teamConnected[i]) {
        if(millis() - lastHeartbeat[i] > HEARTBEAT_TIMEOUT) {
          teamConnected[i] = false;
          statusChanged = true;
          Serial.print("❌ Team ");
          Serial.print(i + 1);
          Serial.println(" DISCONNECTED");
        }
      }
    }
    
    if(statusChanged) {
      updateDisplay();
      delay(50);
      broadcastUpdateSafe();
    }
  }
  
  // ✅ FIXED: Reset button handler with 1-second cooldown
  if(resetPressed) {
    resetPressed = false;
    
    // Check cooldown period
    unsigned long now = millis();
    static unsigned long lastResetExecution = 0;
    const unsigned long RESET_COOLDOWN = 1000; // 1 second cooldown
    
    if(now - lastResetExecution < RESET_COOLDOWN) {
      Serial.println("⏱️ Reset ignored (cooldown active)");
      return; // Ignore this trigger
    }
    
    lastResetExecution = now; // Update last execution time
    resetFlashActive = true;
    resetFlashStart = now;
    
    // ✅ Toggle between LISTEN and READY
    if(!quizActive) {
      // Currently LISTEN → Switch to READY
      quizActive = true;
      Serial.println("\n📢 LISTEN → READY mode");
      Serial.println("   Accepting buzzers now!\n");
    } else {
      // Currently READY/ANSWERED → Switch to LISTEN
      quizActive = false;
      Serial.println("\n⏳ READY/ANSWERED → LISTEN mode");
      Serial.println("   Buzzers ignored!\n");
    }
    
    // Clear quiz data
    winnerTeam = 0;
    winnerTimestamp = 0;
    responseCount = 0;
    memset(responseOrder, 0, sizeof(responseOrder));
    memset(teamHasResponded, false, sizeof(teamHasResponded));
    digitalWrite(LED_WINNER, LOW);
    lastResponseTime = 0;
    batchPending = false;
    
    Serial.println("🔄 SYSTEM RESET COMPLETE\n");
    updateDisplay();
    delay(50);
    broadcastUpdatePriority(); // Force immediate broadcast
  }
  
  delay(10);
}
