# 📋 Software Setup Guide

## 🎯 Quick Overview

This guide covers setting up the **master ESP32 firmware**, the **team unit firmware**, and the **web dashboard**.

**System Architecture:**
```
Master ESP32 (AP)  ←→ [ESP-NOW] ←→ 10× Team ESP32 (Clients)
      ↓
   Web Dashboard (WebSocket)
      ↓
   Live Monitoring & Control
```

---

## 📋 PREREQUISITES

### Hardware Requirements
- **Master unit:** ESP32 DevKit + OLED 128x64 + 3× LEDs + Reset button + Power switch
- **Team units:** 10× ESP32 DevKit + button + 3× LEDs + buzzer + battery (each)
- **USB Cables:** For programming ESP32s
- **Power Supply:** 5V for all units

### Software Requirements
- **PlatformIO IDE** (recommended) or **Arduino IDE**
- **Git** (for version control)
- **USB Drivers:** CH340/CP2102 for ESP32

---

## 🔧 INSTALLATION STEPS

### STEP 1: Install PlatformIO

**Option A: PlatformIO Extension (Recommended)**
1. Install VS Code: https://code.visualstudio.com
2. Open VS Code
3. Go to Extensions (Ctrl+Shift+X)
4. Search "PlatformIO"
5. Click Install on "PlatformIO IDE"
6. Restart VS Code
7. ✅ PlatformIO ready!

**Option B: Arduino IDE**
1. Download: https://www.arduino.cc/en/software
2. Install
3. Go to Tools → Board → Boards Manager
4. Search "ESP32"
5. Install "esp32 by Espressif Systems"
6. ✅ Arduino ready!

---

### STEP 2: Create Project Structure

```bash
git clone https://github.com/asifahamed-ece/wireless-quiz-buzzer.git
cd wireless-quiz-buzzer

# The folders already exist after cloning - nothing to create
ls firmware dashboard/data
```

---

### STEP 3: Master ESP32 Setup

#### 3.1 PlatformIO project

`firmware/master/platformio.ini` as shipped:

```ini
[platformio]
; The dashboard is flashed to the master's LittleFS with: pio run --target uploadfs
data_dir = ../../dashboard/data

[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

; Enable LittleFS filesystem
board_build.filesystem = littlefs
board_build.partitions = default.csv

upload_speed = 921600
monitor_speed = 115200

lib_deps =
    adafruit/Adafruit GFX Library
    adafruit/Adafruit SSD1306
    https://github.com/me-no-dev/ESPAsyncWebServer.git
    https://github.com/me-no-dev/AsyncTCP.git
    bblanchon/ArduinoJson @ ^7.2.0
```

ESP-NOW, WiFi, Wire, and LittleFS come from the ESP32 Arduino framework and are
not listed as dependencies.

#### 3.2 Master firmware

`firmware/master/src/main.cpp` provides:

   - **WiFi AP mode**: creates `QuizBuzzer_AP` for the dashboard
   - **ESP-NOW**: receives packets from team units
   - **OLED**: shows channel, connected count, and phase
   - **WebSocket server**: pushes state updates to the dashboard
   - **Two-phase flow**: LISTEN ↔ READY, with ANSWERED on a win
   - **Battery tracking**: records each unit's zone and percentage
   - **Power switch**: standby mode stops WiFi, ESP-NOW, and the web server

#### 3.3 Pin Configuration

```cpp
#define RESET_BUTTON 13     // GPIO 13
#define LED_WINNER 2        // GPIO 2
#define LED_SYNC 15         // GPIO 15
#define LED_READY 16        // GPIO 16
#define POWER_SWITCH 25     // GPIO 25

// OLED: SDA=GPIO 21, SCL=GPIO 22 (I2C), address 0x3C
```

#### 3.4 Build and flash the master

```bash
cd firmware/master
pio run --target upload
pio device monitor --baud 115200
```

**Expected output:**
```
✅ LittleFS initialized!
🔴 STANDBY mode
```

Flip the power switch on the master to start the system:
```
🟢 SYSTEM ENABLED
📡 AP IP: 192.168.4.1
🔑 MAC: AA:BB:CC:DD:EE:FF
✅ ESP-NOW initialized
✅ Web server started
📢 Starting in LISTEN mode
```

Record the MAC address printed here: every team unit needs it in `masterMAC`.

---

### STEP 4: Team Unit ESP32 Setup

#### 4.1 PlatformIO project

`firmware/slave/platformio.ini` as shipped:

```ini
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
```

No `lib_deps` are needed: the team unit uses only ESP-NOW and WiFi, both of
which the ESP32 Arduino framework provides.

#### 4.2 Firmware features

`firmware/slave/src/main.cpp`:

   - **ESP-NOW**: sends packets to the master
   - **Button**: interrupt-driven, on GPIO 4
   - **Buzzer**: sounds for 1 s on a press
   - **LEDs**: action (GPIO 2), sync (GPIO 15), battery (GPIO 27)
   - **Battery**: reads the divider on GPIO 34, reports percentage and zone
   - **Team ID**: configurable per unit

#### 4.3 Configure each unit

Before every flash, set the unit's ID in `firmware/slave/src/main.cpp`:

```cpp
#define TEAM_ID 1                                              // 1-10, unique per unit
uint8_t masterMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // replace with master MAC
```

Boot the master once and read its MAC address from the serial monitor, then paste
it into `masterMAC`. The shipped all-`0xFF` value is a broadcast address, which
works but is not specific to your master.

Pin assignments are fixed in the firmware and must match the wiring in
[HARDWARE_SETUP.md](HARDWARE_SETUP.md):

```cpp
#define BUTTON_PIN 4        // button, internal pull-up
#define LED_ACTION 2        // red, 2 s on press
#define LED_SYNC 15         // green, connection status
#define LED_BATTERY 27      // blue, battery health
#define BUZZER_PIN 5        // 1 s on press
#define BATTERY_PIN 34      // ADC, battery divider
```

#### 4.4 Upload to each unit

```bash
cd firmware/slave

# For each unit:
# 1. Edit TEAM_ID (1-10, unique)
# 2. Edit masterMAC if you want unicast instead of broadcast
# 3. Upload
pio run --target upload
pio device monitor --baud 115200
```

**Repeat for all 10 units.**

---

### STEP 5: Dashboard Setup

#### 5.1 File structure

```
dashboard/data/
├── index.html      dashboard markup
├── style.css       stylesheet
└── script.js       WebSocket client, audio, confetti
```

#### 5.2 Flash the dashboard

These files are already in the repository. `firmware/master/platformio.ini`
declares `data_dir = ../../dashboard/data`, so PlatformIO picks them up
automatically:

```bash
cd firmware/master
pio run --target uploadfs
```

The build lists the files it will write:

```
Building FS image from '.../dashboard/data' directory to .pio/build/esp32dev/littlefs.bin
/index.html
/script.js
/style.css
```

Edit any file in `dashboard/data/` and re-run the command to push the change to
the master.

The master serves these at:

| Path | Content type |
|------|--------------|
| `/` | `index.html` |
| `/style.css` | `text/css` |
| `/script.js` | `application/javascript` |

#### 5.3 Dashboard features

**HTML elements:**
- Audio toggle button (top right)
- Team status panel (left)
- Phase and winner display (centre)
- System info panel (right)
- Confetti container
- Response order list

**CSS styling:**
- Dark theme (`#0a0e27` background)
- Gradient panels
- Animations: `breathe`, `pulse`, `confetti-fall`
- Responsive grid layout
- Phase colours:
  - LISTEN: orange (`#ff8800`)
  - READY: pink (`#f093fb` → `#f5576c`)
  - WINNER: green (`#11998e` → `#38ef7d`)

**JavaScript features:**
- WebSocket connection to `ws://<host>/ws`, with reconnect
- Phase tracking and phase-change audio
- Web Audio API buzzer, reset, online, offline and winner sounds
- Background music loop during READY
- Speech synthesis announcing the winning team
- Confetti on a winner
- Battery zone display per team

---

## 🚀 TESTING THE SYSTEM

### Test 1: Hardware Connection

1. Power on Master ESP32
2. Watch OLED display
3. Should show: "LISTEN", "CH:1", "Teams: 0/10"

### Test 2: Team Unit Connection

1. Power on team unit 1
2. Master serial: `✅ Team 1 connected! | Battery: 100% (🔋 GREEN)`
3. Master OLED: `Teams: 1/10`

### Test 3: Dashboard Access

1. Join WiFi network `QuizBuzzer_AP` (password `12345678`)
2. Open **http://192.168.4.1**
3. Should see:
   - Header: "QUIZ BUZZER SYSTEM"
   - Left panel: team status
   - Centre: phase display, starting at LISTEN
   - Right: system info
   - Audio toggle button (top right)

### Test 4: System Phases

**LISTEN mode:**
- Master OLED: `LISTEN ?`
- Dashboard centre: `LISTEN / Question Being Asked`
- Team unit presses button: **IGNORED** (master logs
  `buzzed in LISTEN mode (ignored)`)

**READY mode:**
1. Press RESET on the master
2. Master OLED: `READY`
3. A team unit press is **REGISTERED**
4. First press wins: OLED shows `BUZZED! T<n>`, dashboard shows the winner
5. Dashboard lists the response order

### Test 5: Complete Quiz Round

1. **LISTEN phase**: question is being read
   - Team presses are ignored
   - Master shows `LISTEN ?`

2. **Press RESET** → READY phase
   - Master shows `READY`
   - Teams can buzz

3. **A team buzzes**
   - First press is registered
   - LED_WINNER lights on the master
   - Dashboard shows the winner and team ID
   - Confetti, jingle, and voice announcement
   - The press is broadcast after a 200 ms batching window

4. **Further teams respond**
   - Response order list updates with position, team, and timestamp

5. **Press RESET** → back to LISTEN
   - Winner cleared, response list cleared
   - Back to `LISTEN ?`

---

## 📡 NETWORK CONFIGURATION

### WiFi Details

**Master AP:**
```
SSID: QuizBuzzer_AP
Password: 12345678
IP: 192.168.4.1
Channel: 1
```

**To Connect:**
1. Phone/Laptop WiFi settings
2. Find "QuizBuzzer_AP"
3. Enter password "12345678"
4. Open browser → 192.168.4.1

### ESP-NOW Configuration

```cpp
// Master
#define WIFI_CHANNEL 1          // both sides must match
#define MAX_TEAMS 10            // maximum team units
#define HEARTBEAT_TIMEOUT 5000  // drop a unit after 5 s of silence
#define BATCH_WINDOW 200        // broadcast 200 ms after a press

// Team unit
#define HEARTBEAT_INTERVAL 2000 // heartbeat every 2 s
#define CONNECTION_TIMEOUT 3000 // considered offline after 3 s
```

---

## 🔌 SERIAL DEBUG OUTPUT

### Master Console Typical Output

Boot with the power switch off:
```
✅ LittleFS initialized!
╔═══════════════════════════════════════════╗
║  QUIZ BUZZER - TWO PHASE SYSTEM           ║
╚═══════════════════════════════════════════╝
🔴 STANDBY mode
```

After flipping the switch on:
```
🟢 SYSTEM ENABLED
📡 AP IP: 192.168.4.1
✅ ESP-NOW initialized
✅ Web server started
📢 Starting in LISTEN mode
   Press RESET to enter READY mode
```

With units connected:
```
✅ Team 1 connected! | Battery: 100% (🔋 GREEN)
✅ Team 2 connected! | Battery: 98% (🔋 GREEN)
```

On a press:
```
=====================================
🏆 FIRST TO BUZZ: Team 3
⏱️  Timestamp: 1234567890 μs
=====================================
📝 Response #1: Team 3
📝 Response #2: Team 5
📦 Batch broadcast sent!
```

If a unit presses during LISTEN:
```
🚫 Team 3 buzzed in LISTEN mode (ignored)
```

If the same unit presses twice in one round:
```
⚠️ Team 3 already responded
```

If a unit goes quiet for more than 5 s:
```
❌ Team 3 DISCONNECTED
```

### Team Unit Console Typical Output

```
╔═══════════════════════════════════════════╗
║  QUIZ BUZZER - TEAM UNIT FIRMWARE         ║
╚═══════════════════════════════════════════╝
🏷️  Team ID: 1
🔑 MAC Address: AA:BB:CC:DD:EE:FF
═══════════════════════════════════════════
✅ ESP-NOW initialized!
✅ Master peer added!
🔋 Battery: 4.02V (99%) - 🔋 GREEN
🔍 Searching for Master...
⚡ Interrupt-based system ACTIVE
```

On a press:
```
════════════════════════════════════
🔴 BUTTON PRESSED!
⏱️  Timestamp: 1234567890 μs
════════════════════════════════════
📡 Signal sent to Master!
🔊 Buzzer: ON for 1 second
🔴 Red LED: ON for 2 seconds
```

If the battery zone changes:
```
⚠️ BATTERY ZONE CHANGED: ⚠️ YELLOW (Charge soon!)
```

---

## ⚙️ TROUBLESHOOTING

### Problem: Master doesn't show up

**Solution:**
- Check OLED connection (SDA=21, SCL=22)
- Verify power supply
- Check USB cable connection
- Try uploading again

### Problem: Team units can't connect

**Solution:**
- Check WiFi channel (must be 1 on both sides)
- Verify each unit's `TEAM_ID` is unique and in 1-10
- Check the unit's `masterMAC` matches the master's printed MAC
- Restart the master first, then the units

### Problem: Dashboard won't load

**Solution:**
1. Check the URL: http://192.168.4.1
2. Confirm you are joined to `QuizBuzzer_AP`
3. Check the browser console (F12) for errors
4. Try a different browser
5. Clear the cache

### Problem: Team unit button not responding

**Solution:**
- Check the button is on GPIO 4 and returns to GND
- Confirm the master is in READY mode, not LISTEN
- Watch the master's serial output for the unit
- Watch the unit's own serial output for `BUTTON PRESSED!`
- Check the green LED is solid, meaning the unit has a master

### Problem: Response order looks wrong

Teams are ranked by the order presses reach the master, not by the timestamp on
each unit. See [API_REFERENCE.md](API_REFERENCE.md) for the details. Things that
affect the order:

- Signal strength: units further from the master arrive later
- WiFi interference on channel 1
- Units whose `TEAM_ID` collide are indistinguishable

---

## 📊 PERFORMANCE MONITORING

### Check Master Status

```bash
# View real-time serial output
pio device monitor --baud 115200

# Look for:
# ✅ Team connections
# 📡 WebSocket clients
# 📝 Response tracking
# 🪫 Battery status
```

### Check Team Unit Status

```bash
# Monitor each unit over USB
# Look for:
# 🏷️  Team ID
# 🔋 Battery level
# 🔴 BUTTON PRESSED!
```

### Dashboard Monitoring

- **Left panel**: team online/offline status and battery zone
- **Right panel**: battery health counts and system info
- **Centre**: phase display, winner, and response order
- **Top right**: audio toggle status

---

## 🎯 Next Steps

1. Flash the master and all team units
2. Flash the dashboard
3. Run test rounds with the units close to the master
4. Check battery readings
5. Test the audio with sound enabled
6. Move units to their real positions

---

## 📞 Support

- **Serial monitor**: the most detailed source of information
- **Dashboard**: live system state
- **GitHub issues**: report problems
- **Docs**: [README.md](README.md), [TROUBLESHOOTING.md](TROUBLESHOOTING.md),
  [QUICK_REFERENCE.md](QUICK_REFERENCE.md)

---

**Made with ❤️ by Asif Ahamed S**  
**Rajalakshmi Engineering College, Chennai**  
**2026**
