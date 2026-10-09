/**
 * ═══════════════════════════════════════════════════════════════════
 *  REC QUIZ BUZZER - FINAL SLAVE CODE (Production Ready)
 *  
 *  Features:
  *  - Interrupt-based button (microsecond precision)
  *  - ESP-NOW communication with heartbeat
  *  - Battery monitoring (analogReadMilliVolts - no calibration!)
  *  - 3 LED indicators (Sync, Action, Battery)
  *  - Buzzer feedback
  *  - Connection timeout detection
 *  
 * Hardware Connections:
  * - Button: GPIO 4 (with internal pull-up)
  * - LED Action (Red): GPIO 2 (ON for 2 seconds after press)  // ← FIX THIS
  * - LED Sync (Green): GPIO 15 (connection status)
  * - LED Battery (Blue): GPIO 27 (battery indicator)
  * - Buzzer: GPIO 5 (beeps for 1 second after press)  // ← FIX THIS
  * - Battery ADC: GPIO 34
 *  
 *  Developer: ASIF AHAMED S | ECE 2023-27
 *  Institution: Rajalakshmi Engineering College
 * ═══════════════════════════════════════════════════════════════════
 */

#include <esp_now.h>
#include <WiFi.h>

// ═══════════════════════════════════════════════════════════
//  ⚠️ CONFIGURATION REQUIRED FOR EACH TEAM NODE:
// 
//  1. Change TEAM_ID (line 20):
//     #define TEAM_ID 1  // Change to 1-10
// 
//  2. Update masterMAC (line 21) with your Master's MAC:
//     Find Master MAC from Serial Monitor on first boot
//     Format: {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}
// 
//  3. Battery calibration (if needed):
//     - MAX_BATTERY_VOLTAGE (line 32): 4.08V for your batteries
//     - Adjust if different battery type used
// ═══════════════════════════════════════════════════════════


#define TEAM_ID 7
uint8_t masterMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // Update master MAC ⚠️
// ═══════════════════════════════════════════════════

// Pin Definitions
#define BUTTON_PIN 4        // Button input
#define LED_ACTION 2        // Red LED - 2 seconds after press
#define LED_SYNC 15         // Green LED - connection status
#define LED_BATTERY 27      // Blue LED - battery status indicator
#define BUZZER_PIN 5        // Buzzer - 1 seconds after press
#define BATTERY_PIN 34      // ADC for battery monitoring

// Battery Configuration (4.08V max for 18650 batteries)
const float MAX_BATTERY_VOLTAGE = 4.08;
const float MIN_BATTERY_VOLTAGE = 3.0;
const float R1 = 10000.0;  // 10kΩ voltage divider
const float R2 = 10000.0;  // 10kΩ voltage divider

// Timing Constants
#define HEARTBEAT_INTERVAL 2000      // 2 seconds
#define CONNECTION_TIMEOUT 3000      // 3 seconds
#define ACTION_LED_DURATION 2000    // 2 seconds
#define BUZZER_DURATION 1000         // 1 seconds
#define BATTERY_CHECK_INTERVAL 5000  // 5 seconds
#define DEBOUNCE_TIME 50000          // 50ms in microseconds

// Interrupt Variables (volatile for ISR)
volatile bool buttonPressed = false;
volatile unsigned long buttonPressTime = 0;
volatile unsigned long lastInterruptTime = 0;

// Connection Status
volatile bool isConnectedToMaster = false;
unsigned long lastSuccessfulSend = 0;
unsigned long lastLedBlinkTime = 0;
bool ledBlinkState = false;

// Timing Trackers
unsigned long lastHeartbeatTime = 0;
unsigned long lastBatteryCheck = 0;
unsigned long actionStartTime = 0;
bool actionInProgress = false;

// Battery State
int currentBatteryZone = 2;  // Start with GREEN
float lastVoltage = 0.0;
float lastPercentage = 0.0;
unsigned long lastBatteryLedBlink = 0;
bool batteryLedState = false;

// ESP-NOW Data Structure
typedef struct {
  int teamID;
  unsigned long timestamp;
  bool buttonPressed;
  bool isHeartbeat;
  int batteryZone;      // 2=GREEN, 1=YELLOW, 0=RED
  float batteryPercent; // Actual percentage (0-100)
} BuzzerData;

BuzzerData outgoingData;
esp_now_peer_info_t peerInfo;

// ═══════════════════════════════════════════════════════════
//  BATTERY FUNCTIONS (YouTube Method - No Calibration!)
// ═══════════════════════════════════════════════════════════

float readBatteryVoltage() {
  float sum = 0;
  const int numSamples = 20;  // Average 20 readings for stability
  
  for (int i = 0; i < numSamples; i++) {
    sum += analogReadMilliVolts(BATTERY_PIN);  // ESP32 auto-calibration!
    delay(5);
  }
  
  float averageMilliVolts = sum / numSamples;
  float voltage = (averageMilliVolts / 1000.0);
  
  // Compensate for voltage divider (2:1 ratio)
  voltage = voltage * (R1 + R2) / R2;
  
  return voltage;
}

float calculateBatteryPercentage(float voltage) {
  float percentage = (voltage - MIN_BATTERY_VOLTAGE) / 
                     (MAX_BATTERY_VOLTAGE - MIN_BATTERY_VOLTAGE) * 100.0;
  return constrain(percentage, 0, 100);
}

int getBatteryZone(float voltage) {
  if(voltage >= 3.70) return 2;      // GREEN (>60% SoC)
  else if(voltage >= 3.45) return 1; // YELLOW (40-60% SoC)
  else return 0;                     // RED (<40% SoC)
}

void updateBatteryStatus() {
  lastVoltage = readBatteryVoltage();
  lastPercentage = calculateBatteryPercentage(lastVoltage);
  int newZone = getBatteryZone(lastVoltage);
  
  // Warn on zone change
  if(newZone != currentBatteryZone) {
    currentBatteryZone = newZone;
    Serial.print("\n⚠️ BATTERY ZONE CHANGED: ");
    if(newZone == 2) Serial.println("🔋 GREEN (Good)");
    else if(newZone == 1) Serial.println("⚠️ YELLOW (Charge soon!)");
    else Serial.println("🔴 RED (Replace NOW!)");
  }
  
  // Periodic warnings for low battery
  if(currentBatteryZone == 0) {
    Serial.println("🔴 Battery CRITICAL - Replace immediately!");
  }
}

// ═══════════════════════════════════════════════════════════
//  BATTERY LED INDICATOR (Blue LED)
// ═══════════════════════════════════════════════════════════

void updateBatteryLED() {
  unsigned long currentTime = millis();
  
  if(currentBatteryZone == 2) {
    // GREEN zone - LED OFF (battery good)
    digitalWrite(LED_BATTERY, LOW);
    
  } else if(currentBatteryZone == 1) {
    // YELLOW zone - SLOW BLINK (1 second interval)
    if(currentTime - lastBatteryLedBlink >= 1000) {
      lastBatteryLedBlink = currentTime;
      batteryLedState = !batteryLedState;
      digitalWrite(LED_BATTERY, batteryLedState);
    }
    
  } else {
    // RED zone - FAST BLINK (300ms interval)
    if(currentTime - lastBatteryLedBlink >= 300) {
      lastBatteryLedBlink = currentTime;
      batteryLedState = !batteryLedState;
      digitalWrite(LED_BATTERY, batteryLedState);
    }
  }
}

// ═══════════════════════════════════════════════════════════
//  INTERRUPT SERVICE ROUTINE (Hardware Button)
// ═══════════════════════════════════════════════════════════

void IRAM_ATTR buttonISR() {
  unsigned long currentTime = micros();
  
  // Debounce: Ignore presses within 50ms
  if (currentTime - lastInterruptTime > DEBOUNCE_TIME) {
    buttonPressed = true;
    buttonPressTime = currentTime;  // Microsecond precision!
    lastInterruptTime = currentTime;
  }
}

// ═══════════════════════════════════════════════════════════
//  ESP-NOW CALLBACK (Send Status)
// ═══════════════════════════════════════════════════════════

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if(status == ESP_NOW_SEND_SUCCESS) {
    isConnectedToMaster = true;
    lastSuccessfulSend = millis();
  } else {
    isConnectedToMaster = false;
  }
}

// ═══════════════════════════════════════════════════════════
//  LED CONTROL (Connection Status Indicator)
// ═══════════════════════════════════════════════════════════

void updateSyncLED() {
  // Check for connection timeout
  if(millis() - lastSuccessfulSend > CONNECTION_TIMEOUT) {
    isConnectedToMaster = false;
  }
  
  if(isConnectedToMaster) {
    // SOLID GREEN - Connected to master
    digitalWrite(LED_SYNC, HIGH);
  } else {
    // BLINKING GREEN - Searching for master
    if(millis() - lastLedBlinkTime > 500) {
      lastLedBlinkTime = millis();
      ledBlinkState = !ledBlinkState;
      digitalWrite(LED_SYNC, ledBlinkState);
    }
  }
}

// ═══════════════════════════════════════════════════════════
//  ACTION FEEDBACK (Buzzer + LED after button press)
// ═══════════════════════════════════════════════════════════

void updateActionFeedback() {
  if(actionInProgress) {
    unsigned long elapsed = millis() - actionStartTime;
    
    // Buzzer: ON for 1 second
    if(elapsed < BUZZER_DURATION) {
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }
    
    // Red LED: ON for 2 seconds
    if(elapsed < ACTION_LED_DURATION) {
      digitalWrite(LED_ACTION, HIGH);
    } else {
      digitalWrite(LED_ACTION, LOW);
      actionInProgress = false;  // Done
    }
  } else {
    // Ensure both are off when not in action
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_ACTION, LOW);
  }
}

// ═══════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  
  // Pin setup
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_ACTION, OUTPUT);
  pinMode(LED_SYNC, OUTPUT);
  pinMode(LED_BATTERY, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BATTERY_PIN, INPUT);
  
  // ADC configuration
  analogReadResolution(12);       // 12-bit resolution (0-4095)
  analogSetAttenuation(ADC_11db); // For 0-3.3V range
  
  // Initial LED states
  digitalWrite(LED_ACTION, LOW);
  digitalWrite(LED_SYNC, LOW);
  digitalWrite(LED_BATTERY, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Attach hardware interrupt to button
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  
  // WiFi setup
  WiFi.mode(WIFI_STA);
  
  // Startup banner
  Serial.println("\n╔═══════════════════════════════════════════╗");
  Serial.println("║  REC QUIZ BUZZER - FINAL PRODUCTION       ║");
  Serial.println("╚═══════════════════════════════════════════╝");
  Serial.print("🏷️  Team ID: ");
  Serial.println(TEAM_ID);
  Serial.print("🔑 MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("═══════════════════════════════════════════\n");
  
  // ESP-NOW initialization
  if(esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed!");
    while(1) { delay(1000); }  // Halt on failure
  }
  Serial.println("✅ ESP-NOW initialized!");
  
  // Register send callback
  esp_now_register_send_cb(OnDataSent);
  
  // Add master as peer
  memcpy(peerInfo.peer_addr, masterMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if(esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("❌ Failed to add master peer!");
    while(1) { delay(1000); }  // Halt on failure
  }
  Serial.println("✅ Master peer added!");
  
  // Read initial battery status
  updateBatteryStatus();
  
  Serial.print("🔋 Battery: ");
  Serial.print(lastVoltage, 2);
  Serial.print("V (");
  Serial.print(lastPercentage, 0);
  Serial.print("%) - ");
  if(currentBatteryZone == 2) Serial.println("🔋 GREEN");
  else if(currentBatteryZone == 1) Serial.println("⚠️ YELLOW");
  else Serial.println("🔴 RED");
  
  // Send initial connection packet
  outgoingData.teamID = TEAM_ID;
  outgoingData.timestamp = micros();
  outgoingData.buttonPressed = false;
  outgoingData.isHeartbeat = false;
  outgoingData.batteryZone = currentBatteryZone;
  outgoingData.batteryPercent = lastPercentage;
  
  esp_now_send(masterMAC, (uint8_t *)&outgoingData, sizeof(outgoingData));
  
  Serial.println("🔍 Searching for Master...");
  Serial.println("⚡ Interrupt-based system ACTIVE\n");
}

// ═══════════════════════════════════════════════════════════
//  MAIN LOOP
// ═══════════════════════════════════════════════════════════

void loop() {
  // Update visual feedback (minimal overhead)
  updateSyncLED();
  updateActionFeedback();
  updateBatteryLED();
  
  // Periodic battery check (every 5 seconds)
  if(millis() - lastBatteryCheck >= BATTERY_CHECK_INTERVAL) {
    lastBatteryCheck = millis();
    updateBatteryStatus();
  }
  
  // ─────────────── BUTTON PRESS HANDLER ───────────────
  if(buttonPressed) {
    buttonPressed = false;  // Clear flag immediately for minimum latency
    
    Serial.println("\n════════════════════════════════════");
    Serial.println("🔴 BUTTON PRESSED!");
    Serial.print("⏱️  Timestamp: ");
    Serial.print(buttonPressTime);
    Serial.println(" μs");
    Serial.println("════════════════════════════════════\n");
    
    // Start action feedback (buzzer + LED)
    actionInProgress = true;
    actionStartTime = millis();
    
    // Prepare data packet with battery info
    outgoingData.teamID = TEAM_ID;
    outgoingData.timestamp = buttonPressTime;  // Microsecond precision
    outgoingData.buttonPressed = true;
    outgoingData.isHeartbeat = false;
    outgoingData.batteryZone = currentBatteryZone;
    outgoingData.batteryPercent = lastPercentage;
    
    // Send to master (non-blocking)
    esp_err_t result = esp_now_send(masterMAC, (uint8_t *)&outgoingData, sizeof(outgoingData));
    
    if(result == ESP_OK) {
      Serial.println("📡 Signal sent to Master!");
      Serial.println("🔊 Buzzer: ON for 1 second");
      Serial.println("🔴 Red LED: ON for 2 seconds\n");
    } else {
      Serial.println("❌ Error sending signal!\n");
    }
  }
  
  // ─────────────── HEARTBEAT (Every 2 seconds) ───────────────
  if(millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
    lastHeartbeatTime = millis();
    
    // Prepare heartbeat packet
    outgoingData.teamID = TEAM_ID;
    outgoingData.timestamp = micros();
    outgoingData.buttonPressed = false;
    outgoingData.isHeartbeat = true;
    outgoingData.batteryZone = currentBatteryZone;
    outgoingData.batteryPercent = lastPercentage;
    
    // Send to master (keeps connection alive)
    esp_now_send(masterMAC, (uint8_t *)&outgoingData, sizeof(outgoingData));
  }
  
  delay(1);  // Small delay for stability (minimal latency impact)
}
