# ⚡ Quick Reference

Command cheatsheet for building, flashing, and running the system. For background
explanation see the other guides; for a full walkthrough start with the README.

## Contents

- [Network details](#network-details)
- [Clone](#clone)
- [Per-unit configuration](#per-unit-configuration)
- [Master](#master)
- [Dashboard](#dashboard)
- [Team units](#team-units)
- [Serial monitor](#serial-monitor)
- [Expected output](#expected-output)
- [Hardware pin map](#hardware-pin-map)
- [Build sizes](#build-sizes)
- [Common problems](#common-problems)

---

## Network details

| Setting | Value |
|---------|-------|
| WiFi SSID | `QuizBuzzer_AP` |
| Password | `12345678` |
| Dashboard URL | `http://192.168.4.1` |
| WebSocket URL | `ws://192.168.4.1/ws` |
| WiFi channel | 1 |
| Max team units | 10 |

The master runs a WiFi access point. It does not need an internet connection.

## Clone

```bash
git clone https://github.com/asifahamed-ece/wireless-quiz-buzzer.git
cd wireless-quiz-buzzer
```

## Per-unit configuration

Each team unit is flashed separately with a unique ID. Edit
`firmware/slave/src/main.cpp` before every flash:

```cpp
#define TEAM_ID 1                                              // 1-10, unique per unit
uint8_t masterMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // replace with master MAC
```

Boot the master once and read its MAC address from the serial monitor:

```
🔑 MAC: AA:BB:CC:DD:EE:FF
```

Paste it into `masterMAC`. The shipped all-`0xFF` value is a broadcast address: it
works, but units then respond to any ESP-NOW traffic on the channel rather than
to your master alone.

## Master

```bash
cd firmware/master
pio run                      # build
pio run --target upload      # flash
pio run --target uploadfs    # flash the dashboard to LittleFS
```

## Dashboard

The dashboard lives in `dashboard/data` and is declared as the PlatformIO
`data_dir`, so it is flashed from the master project:

```bash
cd firmware/master
pio run --target uploadfs
```

That writes `index.html`, `style.css`, and `script.js` into the master's
LittleFS. Edit the files in `dashboard/data/` and re-run the command to update.

## Team units

Flash once per unit, changing `TEAM_ID` each time:

```bash
cd firmware/slave
pio run --target upload
```

Repeat for units 1 through 10.

## Serial monitor

```bash
pio device monitor --baud 115200
```

## Expected output

Master, on boot:

```
✅ LittleFS initialized!
🔴 STANDBY mode
```

Flip the master's power switch to start it:

```
🟢 SYSTEM ENABLED
📡 AP IP: 192.168.4.1
🔑 MAC: AA:BB:CC:DD:EE:FF
✅ ESP-NOW initialized
✅ Web server started
📢 Starting in LISTEN mode
```

When a unit connects:

```
✅ Team 1 connected! | Battery: 100% (🔋 GREEN)
```

On a press:

```
=====================================
🏆 FIRST TO BUZZ: Team 1
⏱️  Timestamp: 1234567890 μs
=====================================
📝 Response #1: Team 1
📦 Batch broadcast sent!
```

Team unit, on boot:

```
🏷️  Team ID: 1
🔑 MAC Address: AA:BB:CC:DD:EE:FF
✅ ESP-NOW initialized!
✅ Master peer added!
🔋 Battery: 4.02V (99%) - 🔋 GREEN
🔍 Searching for Master...
```

## Hardware pin map

Master — `firmware/master/src/main.cpp`:

| Signal | GPIO |
|--------|------|
| Reset button | 13 |
| LED winner | 2 |
| LED sync | 15 |
| LED ready | 16 |
| Power switch | 25 |
| OLED SDA / SCL | 21 / 22 |

Team unit — `firmware/slave/src/main.cpp`:

| Signal | GPIO |
|--------|------|
| Button | 4 |
| LED action | 2 |
| LED sync | 15 |
| LED battery | 27 |
| Buzzer | 5 |
| Battery ADC | 34 |

The master has no buzzer. All buzzer feedback comes from the team units and the
dashboard.

## Build sizes

Verified with PlatformIO Core 6.2.0:

| Target | Flash | RAM |
|--------|-------|-----|
| `firmware/master` (`esp32dev`) | 894,369 B (68.2%) | 45,336 B (13.8%) |
| `firmware/slave` (`esp32doit-devkit-v1`) | 744,765 B (56.8%) | 44,288 B (13.5%) |

## Common problems

| Symptom | Fix |
|---------|-----|
| `UnicodeDecodeError` reading `platformio.ini` | The file is UTF-16. Re-save it as UTF-8. |
| `pio run --target uploadfs` finds no files | Run it from `firmware/master`, where `data_dir` is configured. |
| "Failed to connect to ESP32" | Hold BOOT while plugging in, release, then flash. |
| Dashboard will not load | Confirm you joined `QuizBuzzer_AP` and are on `192.168.4.x`. |
| Unit shows offline | Check `masterMAC` matches the master, and `TEAM_ID` is unique. |
| Buzzer does not sound | Team units buzz locally; the master has no buzzer. |
| OLED blank | Check SDA=21, SCL=22, and that the address is `0x3C`. |

Longer explanations are in [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

---

Made with ❤️ by Asif Ahamed S
Rajalakshmi Engineering College, Chennai