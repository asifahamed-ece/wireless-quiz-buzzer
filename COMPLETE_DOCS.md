# 📚 Documentation Index

Every document in this repository, what it covers, and when to read it.

## Contents

- [Documents](#documents)
- [Source layout](#source-layout)
- [Suggested reading order](#suggested-reading-order)

---

## Documents

| Document | Covers | Read it when |
|----------|--------|--------------|
| [README.md](README.md) | What the system is, features, hardware list, install steps, usage, gallery, technical specifications | First, always |
| [HARDWARE_SETUP.md](HARDWARE_SETUP.md) | Components, pin assignments, wiring diagrams, assembly, bill of materials | Before wiring hardware |
| [SOFTWARE_SETUP.md](SOFTWARE_SETUP.md) | Installing PlatformIO, flashing the master, units and dashboard, network configuration | Before flashing firmware |
| [QUICK_REFERENCE.md](QUICK_REFERENCE.md) | Command cheatsheet, network details, pin map, expected serial output | While building or running |
| [API_REFERENCE.md](API_REFERENCE.md) | ESP-NOW packet structure, WebSocket messages, battery algorithm, phase state machine | When modifying firmware or writing a client |
| [TROUBLESHOOTING.md](TROUBLESHOOTING.md) | Symptoms and fixes for power, flashing, connectivity, and behaviour problems | When something does not work |
| [CONTRIBUTING.md](CONTRIBUTING.md) | How to report bugs, propose features, and submit pull requests | Before contributing code |
| [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) | Expected behaviour in project spaces | Before participating |
| [CHANGELOG.md](CHANGELOG.md) | Release history and planned features | When checking what changed |
| [LICENSE](LICENSE) | MIT licence terms | When checking what you may do with the code |

## Source layout

```
wireless-quiz-buzzer/
├── firmware/
│   ├── master/
│   │   ├── platformio.ini     # build config, points data_dir at dashboard/data
│   │   └── src/main.cpp       # access point, ESP-NOW receiver, web server
│   └── slave/
│       ├── platformio.ini     # build config
│       └── src/main.cpp       # button, buzzer, LEDs, battery, ESP-NOW sender
├── dashboard/
│   └── data/                  # index.html, style.css, script.js → LittleFS
└── images/                    # photographs used in the README gallery
```

## Suggested reading order

**Building the system for the first time**

1. [README.md](README.md) — overview and hardware list
2. [HARDWARE_SETUP.md](HARDWARE_SETUP.md) — wiring
3. [SOFTWARE_SETUP.md](SOFTWARE_SETUP.md) — flashing
4. [QUICK_REFERENCE.md](QUICK_REFERENCE.md) — during the build
5. [TROUBLESHOOTING.md](TROUBLESHOOTING.md) — if something fails

**Running a quiz**

1. [README.md](README.md) — usage section
2. [QUICK_REFERENCE.md](QUICK_REFERENCE.md) — network details and controls

**Changing the firmware**

1. [API_REFERENCE.md](API_REFERENCE.md) — protocols and data structures
2. [QUICK_REFERENCE.md](QUICK_REFERENCE.md) — pin map
3. [CONTRIBUTING.md](CONTRIBUTING.md) — commit and pull request conventions

---

Made with ❤️ by Asif Ahamed S
Rajalakshmi Engineering College, Chennai