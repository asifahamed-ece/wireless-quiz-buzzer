# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-01-07

### Added
- Initial release of the quiz buzzer system
- ESP32 master/team-unit architecture with ESP-NOW communication
- Real-time WebSocket dashboard with instant updates
- Two-phase quiz system (LISTEN → READY → ANSWERED)
- Background music (BGM) during READY phase
- Audio feedback system (buzzer sounds, winner jingles, voice announcements)
- Confetti animation effects for winners
- Battery monitoring with 3-zone health system (Green/Yellow/Red)
- OLED display with phase indicators and team count
- Team status panel with online/offline tracking
- Response order display with arrival timestamps
- Smart broadcast system with priority-based throttling
- Debounce protection for reset button (100 ms debounce, 1 s cooldown)
- Heartbeat monitoring for automatic disconnection detection
- Mobile-responsive dashboard design
- Developer credits display (bottom-left badge)
- Support for 10 simultaneous teams
- Standalone operation (no internet required)
- Real-time battery health monitoring
- Professional audio-visual feedback system
- Power switch with standby mode
- Automatic team reconnection handling
- Batch response processing (200ms window optimization)

### Features
- ✅ ESP-NOW wireless communication over tens of metres (not formally measured)
- ✅ Professional web dashboard
- ✅ Three-LED status indicators (Winner, Sync, Ready)
- ✅ Sound effects and background music
- ✅ Voice announcements (Text-to-Speech)
- ✅ Modern UI with glassmorphism design
- ✅ Real-time team status updates
- ✅ Battery percentage display
- ✅ Responsive layout (desktop & mobile)

### Documentation
- Complete README with setup instructions
- Hardware setup guide
- Software installation guide
- User manual
- API reference
- Troubleshooting guide
- Contributing guidelines
- Code of conduct
- MIT License

### Technical Details
- Built with ESP32 microcontrollers
- Uses ESP-NOW for low-latency wireless communication
- WebSocket for real-time dashboard updates
- Modern web technologies (HTML5, CSS3, ES6+)
- AsyncWebServer for web hosting
- ArduinoJson for data handling
- Adafruit libraries for OLED display

## [Unreleased]

### Fixed
- `platformio.ini` was UTF-16 encoded on both firmware targets, which made
  `pio run` fail with `UnicodeDecodeError` on a fresh clone
- Dashboard upload step referenced `dashboard/data` as a PlatformIO project; it
  is now wired through `data_dir` in the master project
- Team unit serial output reported 5 s and 10 s durations for a 1 s buzzer and a
  2 s LED
- Master serial banner box was one character narrower than its borders
- Defaulted `TEAM_ID` to 1 so a unit is not accidentally flashed as team 7

### Changed
- Renamed the project to `wireless-quiz-buzzer` across all documentation, links,
  and in-product strings
- Team units are documented as "team units" rather than "slaves"
- Every documented pin, interval, and battery threshold reconciled against the
  firmware source
- Team unit configuration comment rewritten to describe the procedure instead of
  line numbers that had drifted

### Documentation
- Rewrote `QUICK_REFERENCE.md` as a command cheatsheet
- Rewrote `COMPLETE_DOCS.md` as an index of the documents that exist
- Removed response-time claims the firmware does not implement, and documented
  that ranking uses arrival order at the master
- Documented the real battery algorithm: calibrated `analogReadMilliVolts`, 4.08 V
  ceiling, 2:1 divider, voltage-based zone thresholds
- Corrected the team unit pin map (GPIO 4, 2, 15, 27, 5, 34) and noted the
  battery divider requirement
- Replaced fabricated serial transcripts with strings taken from the source
- `CONTRIBUTING.md`: branch base corrected to `Main`, commit format aligned with
  the one-line convention, broken table-of-contents anchor fixed

### Planned Features
- [ ] Mobile app version (Android/iOS)
- [ ] Score tracking and leaderboard
- [ ] Timer functionality
- [ ] Custom team names and colors
- [ ] Statistics dashboard
- [ ] Export results to CSV/PDF
- [ ] Multi-language support
- [ ] Custom audio uploads
- [ ] PCB design files (Eagle/KiCad)
- [ ] 3D-printed enclosure designs
- [ ] Bluetooth connectivity
- [ ] Integration with quiz platforms
- [ ] Video recording support

---

## Version Guidelines

- **Major**: Breaking changes, significant new features
- **Minor**: New features, non-breaking changes
- **Patch**: Bug fixes, documentation updates

---

**See [Releases](https://github.com/asifahamed-ece/wireless-quiz-buzzer/releases) for binary downloads and previous versions.**
