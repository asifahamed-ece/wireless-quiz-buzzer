# 🔧 Troubleshooting Guide

## 🆘 QUICK HELP

**System not working?**
1. Check power (all devices powered on?)
2. Check serial output (errors?)
3. Check WiFi connection (dashboard loading?)
4. Restart master first, then slaves
5. See specific issue below

---

## 🔴 CRITICAL ISSUES

### Master ESP32 Won't Start

**Symptoms:**
- OLED blank
- No serial output
- LED not blinking
- Device unresponsive

**Troubleshooting:**

```
Step 1: Check Power
├─ LED indicator on USB? ✓ = Powered
├─ Try different USB cable
├─ Try different USB port (USB 3.0 might work better)
└─ Verify 5V power supply rated for 500mA+

Step 2: Check USB Driver
├─ Device Manager (Windows) or System Report (Mac)
├─ Look for: "CH340" or "CP2102" device
├─ If missing: Download driver
│  ├─ CH340: github.com/nodemcu/ch340g-mac-and-linux-driver
│  └─ CP2102: silabs.com/developers/usb-to-uart-bridge-vcp-drivers
├─ Restart computer after installing

Step 3: Check Cable
├─ Try different USB cable (data cable, not charge-only)
├─ Look for: Micro-USB with data lines
└─ Test with known-working cable

Step 4: Check OLED
├─ Verify connections: SDA (GPIO 21), SCL (GPIO 22)
├─ Check for loose wires
├─ Try reseating the OLED connector
└─ Test continuity with multimeter

Step 5: Reload Firmware
├─ Connect to computer
├─ Hold BOOT button while uploading
├─ Wait for completion message
└─ Check serial output
```

**Serial Output Check:**

```bash
# Use PlatformIO
pio device monitor --baud 115200

# Or Arduino IDE
Tools → Serial Monitor → Baud: 115200
```

**Expected Output:**
```
✅ LittleFS initialized!
✅ ESP-NOW initialized
🌐 WebSocket server started!
📢 Starting in LISTEN mode
```

---

### Can't Upload Firmware

**Error: "Failed to connect to ESP32"**

**Solution:**

```
1. Disconnect ESP32
2. Hold BOOT button (GPIO 0)
3. Connect to USB
4. Keep BOOT pressed for 2 seconds
5. Release BOOT
6. Start upload
7. Device should show: "Connecting..."
```

**Error: "Port not found"**

```
1. Check Device Manager for COM port
   Windows: Device Manager → Ports (COM & LPT)
   Mac: System Report → USB
   Linux: ls /dev/tty*

2. In PlatformIO:
   platformio.ini → Add: upload_port = COM3 (or /dev/ttyUSB0)

3. Arduino IDE:
   Tools → Port → Select COM3
```

**Error: "Timeout waiting for packet header"**

```
1. Different USB cable (must have data lines)
2. Try different USB port
3. Update USB driver
4. Reduce upload speed: upload_speed = 460800
5. Try Arduino IDE instead
```

---

## 🟠 CONNECTIVITY ISSUES

### Master Can't Find Slaves

**Symptoms:**
- Master OLED shows "Teams: 0/10"
- Serial: No "Team X connected" messages
- Dashboard shows no team status

**Troubleshooting:**

**Check Slave Power:**
```bash
# Plug in slave unit
# Serial output should show:
✅ Connected to master AP
🎯 Team ID: 1
📡 Sending heartbeat...
```

**If no output:**
1. Check slave USB connection
2. Verify correct firmware uploaded
3. Check Team ID is 1-10
4. Verify WiFi password: "12345678"

**Check WiFi SSID:**
```cpp
// In Master Code:
WiFi.softAP("QuizBuzzer_AP", "12345678", WIFI_CHANNEL);

// Verify SSID is exactly: "QuizBuzzer_AP"
// Password is exactly: "12345678"
```

**Check Channel:**
```cpp
#define WIFI_CHANNEL 1  // Must be same on master and slaves!
```

**Multiple Slaves Not Connecting:**
1. Start with one slave only
2. Verify it connects
3. Add second slave
4. If fails: Check for duplicate Team IDs
5. Restart master between adding slaves

**Manual ESP-NOW Pairing:**
```cpp
// In master code, add debug output:
Serial.print("Waiting for data on channel ");
Serial.println(WIFI_CHANNEL);

// In slave code, verify:
Serial.print("Attempting to connect to channel ");
Serial.println(WIFI_CHANNEL);
```

---

### Dashboard Won't Load

**Symptoms:**
- Browser shows "Connection refused"
- Page blank or doesn't load
- WiFi connected but no dashboard

**Troubleshooting:**

**Check IP Address:**
```
Master IP: http://192.168.4.1

If not working:
1. Open Command Prompt / Terminal
2. ping 192.168.4.1
3. Should show: "Reply from 192.168.4.1"
4. If no reply: Master not properly broadcasting AP

If Master OLED shows "192.168.4.1":
- Try different browser (Chrome, Firefox, Safari)
- Clear browser cache (Ctrl+Shift+Delete)
- Try Incognito/Private mode
- Try different device (phone vs laptop)
```

**Check WiFi Connection:**
```
1. Device WiFi Settings
2. Confirm connected to: "QuizBuzzer_AP"
3. IP should be: 192.168.X.X (not 192.168.4.X)
4. If wrong: Disconnect and reconnect
5. Check signal strength (should be strong)
```

**Check Master Web Server:**
```bash
# In serial monitor, should show:
🌐 WebSocket server started!
   URL: http://192.168.4.1

# If not showing:
1. Restart master
2. Check for errors in serial output
3. Verify LittleFS initialized
```

**Manual Test:**
```bash
# From command line (on connected device)
curl http://192.168.4.1/

# Should return HTML content (not error)

# If error: Check master serial for details
```

**Clear Browser Cache:**
```
Chrome: Ctrl+Shift+Delete
Firefox: Ctrl+Shift+Delete
Safari: Develop → Empty Web Storage
Safari: Clear History...
```

---

## 🟡 FUNCTIONALITY ISSUES

### Buzzer Not Responding

**Symptoms:**
- Slave button press not detected
- Dashboard doesn't show response
- Serial shows: Team X connected, but no buzzer

**Check System Phase:**
```
Master OLED should show:
LISTEN mode?  → Button IGNORED (by design)
READY mode    → Button ACCEPTED

To change phases:
Press RESET button on master
```

**Check Button Connection:**
```cpp
#define BUTTON_PIN 33  // Verify correct pin

// Test button:
1. Connect multimeter to button
2. Press button
3. Should show: Continuity or 0Ω

// If no continuity:
├─ Button damaged
├─ Wiring loose
└─ Wrong pin configured
```

**Check if Team Already Responded:**
```
In READY mode:
- First team to buzz: REGISTERED
- Second press by same team: IGNORED
- Press by different team: REGISTERED
- Response limit: 10 responses per round

Error message in serial:
"❌ Team X already responded"
```

**Master Serial Debug:**
```
Good output:
✅ Team 3 connected! | Battery: 100% (🔋 GREEN)
📝 Response #1: Team 3
📝 Response #2: Team 5

Bad output:
🚫 Team 3 buzzed in LISTEN mode (ignored)
⚠️ Team 3 already responded
```

---

### Winner Not Displaying

**Symptoms:**
- Team buzzes but winner box stays empty
- LED_WINNER doesn't light up
- Dashboard shows no winner

**Troubleshooting:**

**Check System State:**
```
System must be in READY mode:
1. Master OLED: Pink "READY"
2. Serial: "READY mode" shown
3. Reset button: Press once to enter READY

If in LISTEN mode:
└─ All buzzers ignored (by design)
```

**Check Master LED:**
```cpp
#define LED_WINNER 2  // GPIO 2 (check pin)

// Test LED:
1. Measure voltage: Should be 3.3V or 0V
2. Check if light is on
3. Verify correct GPIO pin

// If LED won't light:
├─ Check power connection
├─ LED might be reversed
├─ GPIO pin might be wrong
└─ Current limiting resistor missing
```

**Check Winner Box in Dashboard:**
```
Should show when team buzzes:
┌─────────────────────┐
│ 🏆 WINNER 🏆       │
│                     │
│ TEAM 3              │
│ 1.234567 sec        │
└─────────────────────┘
```

**Check Serial Output:**
```
Should show:
=====================================
🏆 FIRST TO BUZZ: Team 3
⏱️  Timestamp: 1234567890 μs
=====================================

If not showing:
1. Team didn't buzz in READY mode
2. System not in READY mode
3. Check button connection
```

**Manual Test:**
```cpp
// Add to master code for testing:
void testWinner() {
  winnerTeam = 3;
  winnerTimestamp = micros();
  digitalWrite(LED_WINNER, HIGH);
  Serial.println("TEST: Winner set to Team 3");
}

// Call in setup() to test
testWinner();
```

---

### Response Order Wrong

**Symptoms:**
- Faster team listed after slower team
- Timestamps seem incorrect
- Order doesn't match physical button presses

**Timing Precision Check:**
```cpp
// Master uses microseconds:
unsigned long masterTime = micros();

// Slave reports microseconds:
unsigned long slaveTime = incomingData.timestamp;

// Response time = Master - Slave
// Accurate to microsecond (0.000001 sec)
```

**Common Causes:**

```
1. Different clock speeds
   └─ Not fixable (design limitation)

2. Long WiFi transmission time
   └─ Check ESP-NOW range (should be <50m)

3. Slave timestamp incorrect
   └─ Slave micros() starting from wrong value
   └─ Check: Serial output timestamp

4. Multiple responses in same microsecond
   └─ Order undefined (rare)
   └─ Usually fine for competitions

5. Overflow (micros wraps at ~71 minutes)
   └─ Only happens if system runs >71 minutes
   └─ Restart system to reset
```

**Verification:**

```bash
# Check master serial:
📝 Response #1: Team 3 (1.234567 sec)
📝 Response #2: Team 5 (1.456789 sec)

# If times are close (within 0.001 sec):
└─ Order should be correct

# If times differ by 0.1+ sec:
└─ Definitely correct order
```

---

## 🔵 PERFORMANCE ISSUES

### Battery Levels Not Updating

**Symptoms:**
- Dashboard shows "No data"
- Battery percent stuck at 100%
- Zone colors not changing

**Check Master Receives Data:**
```bash
# Serial output should show:
✅ Team 1 connected! | Battery: 95% (🔋 GREEN)
✅ Team 2 connected! | Battery: 85% (⚠️ YELLOW)
✅ Team 3 connected! | Battery: 35% (🪫 RED)
```

**If not showing:**
1. Slaves not connected
2. Check heartbeat message (every 500ms)
3. Verify battery code in slave firmware

**Check Dashboard Updates:**
```
Right panel should show:
Current Status: 🔋×8 ⚠️×1 🪫×1

If not updating:
1. Refresh browser (F5)
2. Check browser console (F12)
3. Look for WebSocket errors
```

**Battery Calculation Verification:**
```cpp
// Expected formula:
float voltage = 4.2;  // Max: 4.2V
float percent = ((voltage - 3.0) / (4.2 - 3.0)) * 100;
// Result: 100%

float voltage = 3.6;  // Mid: 3.6V
float percent = ((voltage - 3.0) / (4.2 - 3.0)) * 100;
// Result: ~50%

float voltage = 3.0;  // Min: 3.0V
float percent = ((voltage - 3.0) / (4.2 - 3.0)) * 100;
// Result: 0%
```

---

### WebSocket Disconnects

**Symptoms:**
- Dashboard shows "Connecting..." indefinitely
- Connection drops after few seconds
- "WS_EVT_DISCONNECT" in serial

**Check WiFi Stability:**
```bash
# From device connected to AP:
ping -c 10 192.168.4.1

# Should show: 0% packet loss
# If high loss: WiFi interference or weak signal
```

**Check Master Load:**
```
Master handles:
- ESP-NOW reception (1000+ msg/sec potential)
- WebSocket transmission (real-time)
- OLED updates
- LED control

If overloaded:
└─ Reduce broadcast frequency
└─ Increase batch window
└─ Limit WebSocket clients
```

**Browser Cache Issue:**
```
1. Hard refresh: Ctrl+Shift+R
2. Clear WebSocket cache: Ctrl+Shift+Delete
3. Try different browser
4. Try Incognito/Private mode
5. Check browser console (F12) for errors
```

---

## ⚠️ HARDWARE ISSUES

### OLED Display Problems

**Display Blank:**
```
1. Check power: Pins 3.3V and GND
2. Check I2C: SDA (GPIO 21), SCL (GPIO 22)
3. Check address: 0x3C (standard, not 0x3D)
4. Try different OLED
5. Check for cold solder joints
```

**Text Unreadable:**
```
1. Check contrast: Software controlled
2. Check power supply voltage (stable 3.3V?)
3. Check wiring (especially SCL/SDA)
4. Display might be wrong address (try 0x3D)
```

**Partial Display:**
```
1. Try rebooting master
2. Check I2C speed: 100kHz (standard)
3. Verify library: Adafruit_SSD1306
4. Check for memory issues
```

---

### LED Issues

**LED Not Lighting:**
```
1. Check polarity: Longer leg = positive
2. Check resistor: Should be 220Ω - 1kΩ
3. Test voltage: Should be 3.3V when on
4. Try different LED
5. Check GPIO pin configuration
```

**LED Always On/Off:**
```cpp
// Check logic:
digitalWrite(LED_PIN, HIGH);   // ON
digitalWrite(LED_PIN, LOW);    // OFF

// Verify correct pin and polarity
```

---

## 🧪 TESTING CHECKLIST

Use this to isolate issues:

```
□ Master powers on
  └─ OLED shows startup message
  └─ Serial output visible

□ Master starts (60 seconds)
  └─ OLED shows IP: 192.168.4.1
  └─ Serial: "WebSocket server started"

□ Slave 1 powers on
  └─ Serial: "Connected to master AP"
  └─ Master OLED: "Teams: 1/10"

□ Connect to WiFi
  └─ Find "QuizBuzzer_AP"
  └─ Enter password "12345678"
  └─ Verify IP: 192.168.4.X

□ Open Dashboard
  └─ http://192.168.4.1
  └─ Page loads (not timeout)

□ Team 1 appears in dashboard
  └─ Left panel: Team 1 listed
  └─ Status: Green indicator (connected)

□ Press RESET (enter READY)
  └─ Master OLED: "READY"
  └─ Dashboard center: Pink gradient

□ Team 1 presses button
  └─ Master OLED: "BUZZED! T1"
  └─ LED_WINNER: Lights up
  └─ Dashboard: Shows Team 1 as winner

□ Add more slaves (repeat as needed)
  └─ Each slave appears in dashboard
  └─ All connected status shows

□ Complete quiz round
  └─ Multiple teams buzz
  └─ Response order correct
  └─ Press RESET for next round
```

---

## 📞 STILL STUCK?

**Provide this information for support:**

```
1. What exactly happens? (describe error)
2. What should happen? (expected behavior)
3. Hardware setup: (list components)
4. Firmware version: (check commit hash)
5. Serial output: (copy from monitor)
6. Screenshot: (of dashboard or error)
7. Steps to reproduce: (1, 2, 3...)
8. What have you tried? (already attempted fixes)
```

**Getting Help:**
- Check serial output first (most info)
- Try restarting master
- Try restarting all devices
- Check all power connections
- Verify all USB cables
- Read this guide completely
- Post on GitHub Issues

---

**Made with ❤️ by Asif Ahamed S**  
**Last Updated: January 2026**  
**For support: Create GitHub Issue with details above**
