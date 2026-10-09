# 🔌 API Reference

## Overview

This document describes the communication protocols used by the system:
- **ESP-NOW**: Master ↔ Slave communication
- **WebSocket**: Master ↔ Dashboard real-time updates
- **HTTP REST**: Dashboard requests

---

## 📡 ESP-NOW PROTOCOL

### Purpose
Wireless communication between Master ESP32 and Slave ESP32 units.

### Configuration
```cpp
#define WIFI_CHANNEL 1          // WiFi channel
#define MAX_TEAMS 10            // Maximum slave units
#define HEARTBEAT_TIMEOUT 5000  // 5 seconds
```

### Data Structure

**BuzzerData (Slave → Master)**

```cpp
typedef struct {
  int teamID;              // 1-10 (Team identification)
  unsigned long timestamp; // microseconds (precise timing)
  bool buttonPressed;      // true if button pressed
  bool isHeartbeat;        // true if heartbeat signal
  int batteryZone;         // 2=GREEN, 1=YELLOW, 0=RED
  float batteryPercent;    // 0-100%
} BuzzerData;
```

**Example Message:**
```json
{
  "teamID": 3,
  "timestamp": 1234567890,
  "buttonPressed": true,
  "isHeartbeat": false,
  "batteryZone": 2,
  "batteryPercent": 95.5
}
```

### Message Types

#### 1. Heartbeat Message (every 2 seconds)
```cpp
// Sent continuously to keep the connection alive
data.teamID = 3;
data.timestamp = micros();
data.buttonPressed = false;
data.isHeartbeat = true;        // ← Heartbeat flag
data.batteryZone = 2;
data.batteryPercent = 95.5;
```

Interval: `HEARTBEAT_INTERVAL` = 2000 ms.

**Purpose:**
- Keeps the master informed that the unit is present
- Carries the current battery zone and percentage
- Lets the master detect a unit going silent

**Timeout:** the master drops a unit that has sent nothing for
`HEARTBEAT_TIMEOUT` = 5000 ms. The unit itself considers itself disconnected from
the master after `CONNECTION_TIMEOUT` = 3000 ms without a successful send, which
is what drives its green LED blinking.

#### 2. Button Press Message (on button click)
```cpp
// Sent when the button is pressed
data.teamID = 3;
data.timestamp = buttonPressTime;  // micros() captured in the ISR
data.buttonPressed = true;         // ← Button pressed flag
data.isHeartbeat = false;
data.batteryZone = 2;
data.batteryPercent = 95.5;
```

The press timestamp is captured by an interrupt on GPIO 4, debounced by 50 ms,
so it is taken as close to the physical press as the firmware allows.

**Behavior:**
- Accepted only while the master is in READY mode
- Ignored in LISTEN mode (the master logs it and discards the packet)
- Ignored if that unit already responded in the current round
- The first accepted press wins; later presses are still added to the response
  order

**Ranking:** units are ranked by the order presses arrive at the master. See
[Timing and response calculation](#-timing-and-response-calculation) for what
the timestamps do and do not mean.

### Battery Levels

```
ZONE 2 (GREEN):   voltage >= 3.70 V   🔋
ZONE 1 (YELLOW):  voltage >= 3.45 V   ⚠️
ZONE 0 (RED):     below 3.45 V        🪫
```

The thresholds are in volts, not percentages. See
[Battery monitoring](#-battery-monitoring) for the full algorithm.

### Timings

```
Team unit heartbeat:        every 2000 ms
Master drops a unit after:  5000 ms of silence
Broadcast after a press:    200 ms batching window (BATCH_WINDOW)
Non-critical broadcasts:    at most once per 1000 ms (MIN_BROADCAST_INTERVAL)
```

The master is receive-only. It never sends ESP-NOW packets, so there is no
beacon and no master-to-unit latency to measure.

---

## 🌐 WEBSOCKET PROTOCOL

### Purpose
Real-time communication between Master and Dashboard (web browsers).

### Connection

**URL:**
```
ws://192.168.4.1/ws
```

**Port:** 80 (same as HTTP)

**Connection Flow:**
```
Dashboard (Browser)
    ↓
    [1] WebSocket handshake
    ↓
Master ESP32
    ↓
    [2] Connected confirmation
    ↓
    [3] Initial data sent
    ↓
    [4] Real-time updates (every message change)
```

### Message Format

**Master → Dashboard (JSON)**

All messages are JSON objects containing:

```json
{
  "teams": [
    {
      "id": 1,
      "connected": true,
      "zone": 2,
      "percent": 95.5
    },
    {
      "id": 2,
      "connected": false,
      "zone": 2,
      "percent": 100
    }
  ],
  "winnerTeam": 3,
  "winnerTime": 1234.56,
  "responses": [
    {
      "position": 1,
      "team": 3,
      "time": 1234.56
    },
    {
      "position": 2,
      "team": 5,
      "time": 1456.79
    }
  ],
  "connectedCount": 8,
  "greenCount": 7,
  "yellowCount": 1,
  "redCount": 0,
  "responseCount": 2,
  "quizActive": true,
  "channel": 1
}
```

### Field Descriptions

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `teams[].id` | int | 1-10 | Team number |
| `teams[].connected` | bool | - | Online status |
| `teams[].zone` | int | 0-2 | Battery zone |
| `teams[].percent` | float | 0-100 | Battery % |
| `winnerTeam` | int | 0-10 | 0 = none, 1-10 = team |
| `winnerTime` | float | - | Master uptime in ms when the winner pressed |
| `responses[]` | array | - | All responses in order |
| `responses[].position` | int | 1-10 | Response rank |
| `responses[].team` | int | 1-10 | Team ID |
| `responses[].time` | float | - | Master uptime in ms when that press arrived |
| `connectedCount` | int | 0-10 | Teams connected |
| `greenCount` | int | 0-10 | Green battery teams |
| `yellowCount` | int | 0-10 | Yellow battery teams |
| `redCount` | int | 0-10 | Red battery teams |
| `responseCount` | int | 0-10 | Total responses |
| `quizActive` | bool | - | READY mode? |
| `channel` | int | 1 | WiFi channel |

### Update Frequency

**Priority Updates (Immediate):**
- Button press detected
- Winner announced
- Phase change (LISTEN ↔ READY)
- New response received

**Safe Updates (Max 1/second):**
- Team connection status
- Battery level changes
- Heartbeat information

### JavaScript Client Example

```javascript
// Connect to the master's WebSocket, using the current page's host so the
// dashboard works whether it is opened by IP or by hostname.
const ws = new WebSocket(`ws://${window.location.hostname}/ws`);

ws.onopen = () => {
  console.log('Connected to Master');
};

ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  // updateDashboard() dispatches to the four panel update functions
  updateDashboard(data);
};

ws.onclose = () => {
  console.log('Disconnected from Master');
  // the real client retries every 5 s via setInterval
};

ws.onerror = (error) => {
  console.error('WebSocket error:', error);
};
```

The shipped dashboard's `updateDashboard()` derives the phase and then calls
`updateTeamsList()`, `updateCenterDisplay()`, `updateResponsesList()`, and
`updateInfoPanel()`. It never sends anything back over the socket.

---

## 🔄 SYSTEM STATES

### Two-Phase System

**LISTEN phase:**
```
quizActive = false

Characteristics:
- The question is being read
- Team unit presses are ignored
- Dashboard centre: "LISTEN / Question Being Asked"
- Master OLED: "LISTEN ?"

Transition: press RESET → READY phase
```

**READY phase:**
```
quizActive = true

Characteristics:
- Waiting for presses
- Team unit presses are processed
- First press = winner
- Master OLED: "READY", then "BUZZED! T<n>" once a unit has pressed
- Dashboard centre: "READY / Press Your Buzzer!", then the winner card

Transition: press RESET → LISTEN phase
```

ANSWERED is not a separate state on the master. It exists only in the dashboard,
which derives it from `quizActive == true` combined with `winnerTeam > 0`. The
master clears `winnerTeam` and the response list on every RESET, so the round
starts over either way.

### State Transitions

```
LISTEN (initial)
    ↓ [press RESET]
READY
    ↓ [first press received]
ANSWERED (winner shown) — dashboard-side view only
    ↓ [press RESET]
LISTEN (cycle repeats)
```

---

## 📊 BATTERY MONITORING

### Algorithm

The team unit averages 20 ADC readings taken 5 ms apart, undoes the 2:1 divider,
then converts voltage to a percentage and to a zone:

```cpp
// readBatteryVoltage()
float sum = 0;
for (int i = 0; i < 20; i++) {
  sum += analogReadMilliVolts(BATTERY_PIN);
  delay(5);
}
float voltage = (sum / 20) / 1000.0;
voltage = voltage * (R1 + R2) / R2;   // R1 = R2 = 10k → ×2

// calculateBatteryPercentage()
float percentage = (voltage - 3.0) / (4.08 - 3.0) * 100.0;
return constrain(percentage, 0, 100);

// getBatteryZone() — thresholds are in VOLTS, not percent
if      (voltage >= 3.70) return 2;  // GREEN
else if (voltage >= 3.45) return 1;  // YELLOW
else                        return 0; // RED
```

Notes on the implementation:

- `analogReadMilliVolts` uses the ESP32's calibrated internal reference, so no
  manual calibration constant is needed.
- The ADC is configured for 12-bit resolution with `ADC_11db` attenuation.
- GPIO 34 is an ADC-only pin. The 2:1 divider is required; without it every
  reading is halved and every unit reports RED.
- The zone thresholds are voltage thresholds, not percentage thresholds. They do
  not correspond exactly to 60% and 40% of the 3.0-4.08 V range.
- The battery is re-read every 5 seconds (`BATTERY_CHECK_INTERVAL`).

### Three-Zone System

| Zone | Colour | Voltage | Approx. percent | Status |
|------|--------|---------|-----------------|--------|
| 2 | 🟢 GREEN | ≥ 3.70 V | ~65% and above | Ready to use |
| 1 | 🟡 YELLOW | 3.45 - 3.70 V | ~42% - 65% | Charge soon |
| 0 | 🔴 RED | < 3.45 V | below ~42% | Replace now |

### Master Tracking

```cpp
// Master maintains battery status for all teams
int   teamBatteryZone[MAX_TEAMS];      // 0, 1, or 2
float teamBatteryPercent[MAX_TEAMS];   // 0-100

// Updated on every packet received from a team unit
```

The master reads both fields from each incoming packet and stores them against
the sending unit's `teamID`. Units start at 100% / GREEN until they report.

### Dashboard Display

**Left panel, per team** — the battery zone appears as a small icon next to each
connected team:

```
🔋 GREEN   zone 2
⚠️ YELLOW  zone 1
🪫 RED     zone 0
```

**Right panel** — counts across connected units:

```
🔋×8 ⚠️×1 🪫×0
```

plus a summary line: `All good`, `n low`, or `n critical`.

---

## ⏱️ Timing and Response Calculation

This section describes what the firmware actually measures, because it is not
what you might assume from the field names.

### What each unit timestamps

Each team unit captures its own press time in the button interrupt:

```cpp
void IRAM_ATTR buttonISR() {
  unsigned long currentTime = micros();
  if (currentTime - lastInterruptTime > DEBOUNCE_TIME) {  // 50 ms debounce
    buttonPressed = true;
    buttonPressTime = currentTime;   // this unit's clock
    lastInterruptTime = currentTime;
  }
}
```

That value is transmitted in the packet as `timestamp`.

### What the master records

On arrival, the master does **not** use the timestamp from the packet. It stamps
its own clock:

```cpp
// master, in the ESP-NOW receive callback
unsigned long masterTimestamp = micros();   // master's clock, not the unit's
```

`incomingData.timestamp` is never read. Every response is therefore recorded
with the master's arrival time.

### What the dashboard shows

```cpp
doc["winnerTime"] = winnerTimestamp / 1000.0;              // ms since master boot
resp["time"]      = responseOrder[i].timestamp / 1000.0;   // ms since master boot
```

The dashboard renders both as `⚡ <value> ms`.

So the number on the dashboard is **the master's uptime in milliseconds at the
moment the press arrived**, not a measured reaction time. It is not a response
time, and the values are not comparable between units, because they all come
from one clock.

### Ranking

Ranking does not depend on those numbers. The master records presses in the
order the receive callback fires them, and that arrival order is the ranking:

```
Position 1: the first packet the master processed
Position 2: the second, and so on
```

`position` is assigned sequentially as packets are received. For units close
together on a quiet channel this is a fair ordering of who pressed first. Signal
strength and interference both affect it.

### Why the unit timestamp is unused

Using the per-unit timestamp for true response timing would require two things
the firmware does not do:

1. The master would need to subtract `incomingData.timestamp` from its own
   `micros()`.
2. The unit clocks would need to be synchronised to the master, since each
   ESP32's `micros()` counts from its own boot and the counters are unrelated.

Without synchronisation, the subtraction would produce meaningless values —
potentially negative ones. The field is present in the packet structure and the
master already receives it, so adding a sync step and using it is a contained
change rather than a redesign.

---

## 🎯 HTTP REST Endpoints

### Static Files

**GET /index.html**
```
Response: HTML dashboard file
Content-Type: text/html
Status: 200 OK
```

**GET /style.css**
```
Response: CSS stylesheet
Content-Type: text/css
Status: 200 OK
```

**GET /script.js**
```
Response: JavaScript file
Content-Type: application/javascript
Status: 200 OK
```

**GET /favicon.ico**
```
Response: (no content)
Status: 204 No Content
```

---

## 🔐 SECURITY NOTES

**Current Implementation:**
- No authentication (local network only)
- No encryption on ESP-NOW
- WiFi password: `12345678` (weak)

**Production Recommendations:**
1. Change WiFi password to strong value
2. Add authentication to WebSocket
3. Implement HTTPS/WSS
4. Validate all input data
5. Rate limiting on requests

---

## 📈 Bandwidth Usage

### Team units → Master (ESP-NOW)

Traffic is one-way: the master never sends ESP-NOW packets.

**Per packet:**
- `BuzzerData` is 20 bytes (`sizeof` with natural alignment on ESP32)
- Heartbeat: one packet per unit every 2 s
- Button press: one packet per press

**Steady state (10 units connected, all heartbeating):**
```
10 units × 20 bytes / 2 s = 100 bytes/sec ≈ 0.8 Kbps
```

Button presses add a short burst of at most 10 packets per round, which is
negligible against the heartbeat rate.

### Master → Dashboard (WebSocket)

**Per message:**
- JSON payload is roughly 500-700 bytes for 10 teams
- Non-critical updates are throttled to at most one per second
  (`MIN_BROADCAST_INTERVAL`)
- Critical updates (phase change, winner, press batches) bypass the throttle

```
Idle:              1 msg/sec  × ~600 B = ~600 B/sec ≈ 4.8 Kbps
Active round:      a few messages per round, plus throttled status updates
```

### Dashboard → Master

None. The dashboard never sends messages; `ws.send()` is not used anywhere in
`dashboard/data/script.js`. It only receives.

---

## 🧪 Testing

### Manual ESP-NOW Test

Add this to `firmware/slave/src/main.cpp` temporarily to send one synthetic press
instead of waiting for the button:

```cpp
BuzzerData testData;
testData.teamID = TEAM_ID;
testData.timestamp = micros();
testData.buttonPressed = true;
testData.isHeartbeat = false;
testData.batteryZone = currentBatteryZone;
testData.batteryPercent = lastPercentage;

esp_now_send(masterMAC, (uint8_t *)&testData, sizeof(testData));
```

The master should log `FIRST TO BUZZ` if it is in READY mode, or
`buzzed in LISTEN mode (ignored)` if it is not.

### Manual WebSocket Test

```bash
npm install -g wscat
wscat -c ws://192.168.4.1/ws
```

JSON updates arrive on connect and after each change.

### Dashboard Checks

In the browser console (F12):

```javascript
// Connection state: 0=CONNECTING, 1=OPEN, 2=CLOSING, 3=CLOSED
console.log(ws.readyState);
```

Note that `ws` is a module-level `let` in `script.js`, not on `window`, so it may
need to be reached directly by name. The dashboard never calls `ws.send()`.

To confirm the master is serving files:

```bash
curl -I http://192.168.4.1/            # index.html
curl -I http://192.168.4.1/script.js   # 200 + application/javascript
```

---

## 📞 DEBUGGING

### Master Serial Output

```
📡 PRIORITY Broadcast → quizActive=TRUE | Winner=3 | Clients=1
📡 Broadcast → 1 clients
📡 No changes detected
⏳ Broadcast throttled (non-critical)
🔌 WebSocket client #1 connected
🔌 WebSocket client #1 disconnected
```

### Dashboard Console

Real output from `dashboard/data/script.js`:

```
📊 Dashboard loaded
📢 Forcing LISTEN phase on load
✅ Dashboard initialized in LISTEN mode
✅ WebSocket connected
📡 Received: quizActive=true, winner=3, responses=2
🔄 Phase Change: LISTEN → READY
🎵 BGM Started (LISTEN → READY)
🔊 Team 3 buzzed
🏆 Victory jingle played!
🏆 BGM Stopped (Winner found)
✅ Team 4 online
❌ Team 4 offline
❌ WebSocket disconnected
🔄 Attempting reconnect...
```

On failure:

```
JSON parse error: ...
WebSocket error: ...
```

---

## 📚 Related Documentation

- [README.md](README.md): overview and usage
- [HARDWARE_SETUP.md](HARDWARE_SETUP.md): wiring and bill of materials
- [SOFTWARE_SETUP.md](SOFTWARE_SETUP.md): flashing and network setup
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md): symptom-driven fixes
- [QUICK_REFERENCE.md](QUICK_REFERENCE.md): command cheatsheet
- [COMPLETE_DOCS.md](COMPLETE_DOCS.md): index of all documents

---

**Made with ❤️ by Asif Ahamed S**  
**Rajalakshmi Engineering College, Chennai**  
**Version 1.1**
