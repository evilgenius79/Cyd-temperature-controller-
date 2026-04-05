# CYD Peltier Cooling Controller - Wiring & Parts Guide

## What You Need (besides the CYD, sensors, peltiers, and fans)

### Required Components

| Component | Qty | Purpose |
|-----------|-----|---------|
| IRLZ44N (or IRL540N) logic-level N-channel MOSFET | 4 | Switch 12V peltiers and fans from ESP32 3.3V signals |
| 1N4007 diode (or 1N5819 Schottky) | 2 | Flyback protection across fan motors |
| 4.7kΩ resistor | 1 | Pull-up for DS18B20 OneWire bus |
| 10kΩ resistor | 4 | Gate pull-down on each MOSFET |
| 12V DC power supply (10A+ recommended) | 1 | Power peltiers, fans. Each peltier can draw 3-6A |
| Screw terminal breakout / perfboard | 1 | Clean wiring |
| JST-XH or Dupont connectors | misc | Sensor and signal wiring |

### Why Logic-Level MOSFETs?

The ESP32 outputs 3.3V on its GPIO pins. Standard MOSFETs (like IRF540) need
10-12V on the gate to fully turn on. **Logic-level** MOSFETs (IRLZ44N, IRL540N,
IRLB8721) are designed to fully switch with 3.3-5V gate drive. This is critical.

---

## Pin Assignments (on the CYD's P3/CN1 expansion header)

| ESP32 GPIO | Function | Connects To |
|------------|----------|-------------|
| GPIO 27 | OneWire data | DS18B20 data pins (yellow wire) |
| GPIO 16 | Peltier 1 PWM | MOSFET 1 gate |
| GPIO 17 | Peltier 2 PWM | MOSFET 2 gate |
| GPIO 22 | Fan 1 PWM | MOSFET 3 gate |
| GPIO 26 | Fan 2 PWM | MOSFET 4 gate |
| GND | Common ground | All MOSFET sources, sensor GND, 12V supply GND |

> **Important**: The CYD's ESP32 ground MUST be connected to the 12V power
> supply ground. This shared ground reference is essential for the MOSFETs to work.

---

## Wiring Diagrams

### DS18B20 Temperature Sensors (both on same bus)

```
3.3V (from CYD) ──┬──[4.7kΩ]──┬── GPIO 27
                   │            │
              ┌────┴───┐  ┌────┴───┐
              │DS18B20 │  │DS18B20 │
              │ VCC DQ │  │ VCC DQ │
              │  GND   │  │  GND   │
              └───┬────┘  └───┬────┘
                  │            │
                 GND          GND
```

DS18B20 wires (typical):
- **Red** = VCC (3.3V)
- **Yellow/White** = Data (GPIO 27)
- **Black** = GND

### MOSFET Circuit (repeat for each peltier and fan)

```
ESP32 GPIO ──────┬──── MOSFET Gate (G)
                 │
              [10kΩ]     ┌─── 12V+
                 │        │
                GND    [LOAD]  (Peltier or Fan)
                          │
                   MOSFET Drain (D)
                          │
                   MOSFET Source (S) ──── GND

For FANS only, add flyback diode across the fan:

         12V+ ──┬──►|──┬── MOSFET Drain
                │       │
              [FAN]     │
                │       │
                └───────┘
         (diode cathode stripe toward 12V+)
```

### Full System Overview

```
                    ┌─────────────────┐
  12V PSU ─────────┤ 12V+        GND ├───── Common GND
                    └──┬──┬──┬──┬────┘
                       │  │  │  │
                    ┌──┴──┴──┴──┴────────────────────┐
                    │  P1   P2   F1   F2  (12V loads)│
                    │  │    │    │    │               │
                    │ [M1] [M2] [M3] [M4] MOSFETs    │
                    │  │    │    │    │               │
                    └──┼────┼────┼────┼──────────────-┘
                       │    │    │    │
    ┌──────────┐    G16  G17  G22  G26
    │   CYD    │───────────────────────
    │ ESP32    │
    │ Display  │─── G27 ──── DS18B20 sensors (OneWire)
    │          │
    │          │─── 3.3V ─── Sensor VCC + pull-up
    │          │─── GND ──── Common GND
    └──────────┘
```

---

## CYD (ESP32-2432S028) Pinout Notes

The CYD exposes a limited number of GPIOs on its expansion header. The pins
chosen above (16, 17, 22, 26, 27) are typically available on the P3/CN1
connector. **Check your specific CYD variant** — some have slightly different
header layouts.

### Pins to AVOID (already used by the CYD's display/touch):
- GPIO 2 (TFT DC)
- GPIO 4 (RGB LED on some variants)
- GPIO 12, 13, 14, 15 (SPI for display)
- GPIO 21 (TFT backlight)
- GPIO 25 (some CYDs use for audio)
- GPIO 33 (touch CS)

---

## Power Budget

| Load | Current Draw | Notes |
|------|-------------|-------|
| Peltier module (TEC1-12706) | 3-6A each | At 12V, full power ~72W each |
| 40mm fan | 0.1-0.2A each | Negligible |
| DS18B20 | ~1mA each | Powered from CYD 3.3V |
| **Total 12V** | **~7-13A** | **Use a 12V 10-15A PSU** |

> **Warning**: Peltiers draw a LOT of current. A cheap 5A supply will NOT work.
> Use a quality 12V 10A+ supply (an old ATX computer PSU works great - use the
> 12V rail).

---

## Assembly Tips

1. **Test sensors first** - wire just the DS18B20s, flash the code, and confirm
   you see valid temperatures on the display before wiring MOSFETs.

2. **Heat sinks on MOSFETs** - if running peltiers at high duty cycles, the
   IRLZ44N MOSFETs may get warm. Small clip-on heat sinks help.

3. **Thick wires for peltiers** - use 16-18 AWG wire for the 12V power to
   peltiers. Thin jumper wires will overheat and melt.

4. **Waterproof the DS18B20s** - if mounting in/near the water cooling loop,
   use the waterproof probe versions, not bare TO-92 packages.

5. **Peltier orientation** - the cold side has printing on it. Make sure cold
   side faces the water block, hot side faces a heat sink with airflow.

6. **Common ground** - the single most common failure. The ESP32 GND, 12V PSU
   GND, MOSFET sources, and sensor GND must ALL be connected together.
