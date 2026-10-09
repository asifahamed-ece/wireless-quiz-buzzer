# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-01-07

### Added
- Initial release of the quiz buzzer system
- ESP32 master-slave architecture with ESP-NOW communication
- Real-time WebSocket dashboard with instant updates
- Two-phase quiz system (LISTEN → READY → ANSWERED)
- Background music (BGM) during READY phase
- Audio feedback system (buzzer sounds, winner jingles, voice announcements)
- Confetti animation effects for winners
- Battery monitoring with 3-zone health system (Green/Yellow/Red)
- OLED display with phase indicators and team count
- Team status panel with online/offline tracking
- Response order display with microsecond timestamps
- Smart broadcast system with priority-based throttling
- Debounce protection for reset button (1-second cooldown)
- Heartbeat monitoring for automatic disconnection detection
- Mobile-responsive dashboard design
- Developer credits display (bottom-left badge)
- Support for 10 simultaneous teams
- Sub-millisecond response time (<1ms precision)
- 50+ meter wireless range
- Standalone operation (no internet required)
- Real-time battery health monitoring
- Professional audio-visual feedback system
- Power switch with standby mode
- Automatic team reconnection handling
- Batch response processing (200ms window optimization)

### Features
- ✅ Microsecond-level timing accuracy (μs precision)
- ✅ Wireless communication up to 50 meters
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
