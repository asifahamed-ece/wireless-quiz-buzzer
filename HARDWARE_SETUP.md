# 🛠️ Hardware Setup Guide

## Master Unit Assembly

### Components Required

```
1x ESP32 DevKit (30-pin)
1x OLED Display (0.96" I2C - SSD1306)
1x Push Button (Momentary SPST)
1x Toggle Switch (SPST)
3x LEDs (Red, Green, Blue - 5mm)
3x 220Ω Resistors (LED current limiting)
1x Breadboard or PCB
1x 5V Power Supply (2A rated)
Jumper wires (M-F and M-M)
```

The master has no buzzer. All buzzer feedback comes from the team units and
from the dashboard.

Both the reset button and the power switch use the ESP32's internal pull-up
(`INPUT_PULLUP` in `firmware/master/src/main.cpp`), so no external pull resistors
are needed. Wire each to GND to close the circuit.

### Pinout Configuration

#### ESP32 Master Pins
```
GPIO 13 (D13) → RESET BUTTON (to GND)
GPIO 2  (D2)  → LED_WINNER (Red)
GPIO 15 (D15) → LED_SYNC (Green)
GPIO 16 (D16) → LED_READY (Blue)
GPIO 25 (D25) → POWER SWITCH (to GND)
SDA (GPIO 21) → OLED SDA
SCL (GPIO 22) → OLED SCL
GND           → OLED GND
3.3V          → OLED VCC
```

### Wiring Diagram

```
┌─────────────────────────────────────────────────┐
│              ESP32 DevKit                       │
│                                                 │
│  GPIO13 ─────────┐                             │
│                  ├─→ Reset Button              │
│  GND ────────────┘                             │
│                                                 │
│  GPIO2 ───[220Ω]─→ LED_WINNER (Red)           │
│  GPIO15 ──[220Ω]─→ LED_SYNC (Green)           │
│  GPIO16 ──[220Ω]─→ LED_READY (Blue)           │
│  GND ────────────→ LEDs GND                    │
│                                                 │
│  GPIO25 ─────────┐                             │
│                  ├─→ Power Switch              │
│  GND ────────────┘                             │
│                                                 │
│  GPIO21 (SDA) ──→ OLED SDA                     │
│  GPIO22 (SCL) ──→ OLED SCL                     │
│  GND ──────────→ OLED GND                      │
│  3.3V ─────────→ OLED VCC                      │
│                                                 │
└─────────────────────────────────────────────────┘

        5V Power Supply
        └─→ ESP32 5V
```

### Assembly Steps

#### 1. Prepare the Breadboard
- Place ESP32 on breadboard center
- Connect power rails (5V and GND)

#### 2. Add OLED Display
```
OLED Connections:
GND  → Breadboard GND rail
VCC  → Breadboard 3.3V rail
SDA  → GPIO21
SCL  → GPIO22
```

#### 3. Add Reset Button
```
Button Connections:
Pin 1 → GPIO13
Pin 2 → GND
```

#### 4. Add Power Switch
```
Switch Connections:
Pin 1 → GPIO25
Pin 2 → GND
```

#### 5. Add LEDs
```
Red LED (Winner):
Anode   → GPIO2 (through 220Ω resistor)
Cathode → GND

Green LED (Sync):
Anode   → GPIO15 (through 220Ω resistor)
Cathode → GND

Blue LED (Ready):
Anode   → GPIO16 (through 220Ω resistor)
Cathode → GND
```

---

## Team Unit Assembly (Repeat for Teams 1-10)

### Components Required (Per Team Unit)

```
1x ESP32 DevKit (30-pin)
1x Push Button (Momentary SPST)
3x LEDs (5mm - Red, Green, Blue)
3x 220Ω Resistors (LED current limiting)
1x Active Buzzer
2x 10kΩ Resistors (battery voltage divider)
1x 18650 Li-ion Battery
1x 18650 Battery Holder
Jumper wires
```

### Pinout Configuration

#### ESP32 Team Unit Pins
```
GPIO 4  (D4)  → BUTTON (to GND, internal pull-up)
GPIO 2  (D2)  → LED_ACTION (Red)
GPIO 15 (D15) → LED_SYNC (Green)
GPIO 27 (D27) → LED_BATTERY (Blue)
GPIO 5  (D5)  → BUZZER
GPIO 34      → BATTERY_ADC (input only, no internal pull-up)
GND          → Common Ground
```

GPIO 34 is an ADC input with no internal pull-up and no output capability. The
battery divider feeds it.

### Wiring Diagram (Single Team Unit)

```
┌────────────────────────────────────────────┐
│          ESP32 DevKit (Team Unit)          │
│                                            │
│  GPIO4 ──────────┐                         │
│                  ├─→ Button               │
│  GND ─────────────┘                        │
│                                            │
│  GPIO2  ──[220Ω]─→ LED_ACTION (Red)        │
│  GPIO15 ──[220Ω]─→ LED_SYNC (Green)        │
│  GPIO27 ──[220Ω]─→ LED_BATTERY (Blue)      │
│  GND  ───────────→ LED cathodes            │
│                                            │
│  GPIO5 ──────────→ Buzzer (+)              │
│  GND ────────────→ Buzzer (-)              │
│                                            │
│  18650 (+) ──[10kΩ]──┐                      │
│                     ├──→ GPIO34 (ADC)      │
│              [10kΩ]─┤                      │
│                   GND                     │
│                                            │
└────────────────────────────────────────────┘

  Button, LED and buzzer behaviour:
    - Button press  → buzzer 1 s, red LED 2 s, packet sent to master
    - Connected     → green LED solid
    - No master     → green LED blinking
    - Battery ok    → blue LED off
    - Battery low   → blue LED blinking
```

The battery divider must be present: the firmware multiplies the GPIO 34 reading
by 2 to recover the cell voltage, which assumes two equal 10 kΩ resistors.

### Portable Case Assembly

**Materials:**
- 3D printed enclosure (or plastic box)
- Velcro strips
- 18650 battery holder
- Cable ties

**Steps:**
1. Mount ESP32 inside case with velcro
2. Attach battery holder at bottom
3. Mount push button on front
4. Attach LEDs on top (visible indicators)
5. Create ventilation holes for heat dissipation

---

## Testing the Hardware

### Master Unit Test

```
1. Power ON → OLED should display:
   ✓ "QUIZ BUZZER"
   ✓ "Initializing..."
   
2. If the power switch is OFF, OLED shows the standby screen:
   ✓ "STANDBY"
   
3. Flip the power switch ON → master starts:
   ✓ "CH: 1"
   ✓ "Teams: 0/10" (waiting for team units)
   ✓ Serial prints the AP IP and the master MAC
   
4. Check LEDs:
   ✓ LED_SYNC blinks while no team is connected
   ✓ LED_SYNC goes solid once a team connects
   ✓ LED_READY flashes for 1 s when RESET is pressed
   
5. Test Reset Button:
   ✓ Press once → OLED switches between LISTEN and READY
   ✓ Serial prints "LISTEN → READY mode" or the reverse
```

There is no buzzer on the master, so no sound is expected from it.

### Team Unit Test

```
1. Power ON → green LED blinks (searching for master)
   
2. Within 3 seconds of the master being up:
   ✓ green LED goes solid (connected to master)
   
3. Test Button:
   ✓ Press → buzzer sounds 1 s, red LED lights 2 s
   ✓ Serial prints "BUTTON PRESSED!"
   
4. Test battery indicator:
   ✓ blue LED off (battery healthy)
   ✓ blue LED blinking (battery low or critical)
   
5. Power OFF:
   ✓ all LEDs off
```

---

## Troubleshooting

### Master Won't Boot
```
Solution:
1. Check power supply (5V, 2A minimum)
2. Verify USB cable connection
3. Check OLED I2C address (0x3C)
4. Try uploading "Blink" sketch first
```

### OLED Display Not Showing
```
Solution:
1. Check I2C connections (SDA, SCL)
2. Verify address: 0x3C (use I2C Scanner)
3. Check power to OLED (3.3V)
4. Try different I2C port if available
```

### Team Units Not Connecting
```
Solution:
1. Master in AP mode, team units in STA mode
2. Both running ESP-NOW, both on channel 1
3. Check masterMAC on the unit matches the master's MAC
4. Check every unit has a unique TEAM_ID in 1-10
5. Restart the master first, then the units
```

### Reset Button Bouncing
```
Solution:
1. Already handled in firmware (100 ms ISR debounce, plus a 1 s cooldown
   before the phase actually changes)
2. If still occurs, add a 0.1µF capacitor across the button pins
3. Update the debounce delay in resetButtonISR()
```

### Buzzer Not Working
```
Solution:
1. Check the buzzer is on GPIO 5 of the team unit
2. The buzzer needs a separate 5V supply - the ESP32 pin only drives it
   through a transistor or relay; do not power a 5V buzzer directly from
   a GPIO
3. Verify polarity
```

---

## Bill of Materials (BOM)

### Master Unit
| Part | Quantity | Cost (USD) |
|------|----------|-----------|
| ESP32 DevKit | 1 | $8 |
| OLED 0.96" I2C | 1 | $3 |
| Push Button | 1 | $0.50 |
| Toggle Switch | 1 | $1 |
| LEDs (3x) | 3 | $0.50 |
| Resistors 220Ω (3x) | 3 | $0.30 |
| Breadboard | 1 | $3 |
| Power Supply 5V 2A | 1 | $8 |
| Jumper Wires | 1 pack | $2 |
| **Total Master** | | **~$26** |

### Per Team Unit
| Part | Quantity | Cost (USD) |
|------|----------|-----------|
| ESP32 DevKit | 1 | $8 |
| Push Button | 1 | $0.50 |
| LEDs (3x) | 3 | $0.50 |
| Resistors 220Ω (3x) | 3 | $0.30 |
| Resistors 10kΩ (2x) | 2 | $0.20 |
| Active Buzzer | 1 | $2 |
| 18650 Battery | 1 | $3 |
| Battery Holder | 1 | $1 |
| Case/Enclosure | 1 | $3 |
| Jumper Wires | 1 | $1 |
| **Total Per Team Unit** | | **~$20** |

### Full System (10 Teams)
```
Master Unit:           $26
10 Team Units:         $200 (10 × $20)
─────────────────────────────
Total Cost:            $226 USD
```

---

**✅ Hardware setup complete! Next: Software installation**
