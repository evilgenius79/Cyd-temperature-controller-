# CYD Peltier Cooling Controller - Wiring & Parts Guide

## Your Board: ESP32-2432S028 (Guition)

Your CYD only exposes **2 GPIOs on the JST connector** (IO22 and IO27). To get
the other 3 signals we need, we repurpose the **RGB LED pins** (GPIO 4, 16, 17)
by soldering wires to the LED pads on the back of the board.

---

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
| JST-SH 1.0mm 4-pin cable (or Dupont wires) | 1 | Connects to CYD JST connector |
| 30AWG silicone wire | ~6 inches | Soldering to RGB LED pads |

### Why Logic-Level MOSFETs?

The ESP32 outputs 3.3V on its GPIO pins. Standard MOSFETs (like IRF540) need
10-12V on the gate to fully turn on. **Logic-level** MOSFETs (IRLZ44N, IRL540N,
IRLB8721) are designed to fully switch with 3.3-5V gate drive. This is critical.

---

## Pin Assignments

| ESP32 GPIO | Function | Where to Access It |
|------------|----------|-------------------|
| GPIO 27 | OneWire data (DS18B20 sensors) | **JST connector** (easy - just plug in) |
| GPIO 22 | Fan 1 PWM → MOSFET 3 gate | **JST connector** (easy - just plug in) |
| GPIO 16 | Peltier 1 PWM → MOSFET 1 gate | **RGB LED pad** (Green - solder wire) |
| GPIO 17 | Peltier 2 PWM → MOSFET 2 gate | **RGB LED pad** (Blue - solder wire) |
| GPIO 4  | Fan 2 PWM → MOSFET 4 gate | **RGB LED pad** (Red - solder wire) |
| 3.3V | Sensor power + pull-up | **JST connector** |
| GND | Common ground | **JST connector** + 12V PSU GND |

> **Important**: The CYD's ESP32 ground MUST be connected to the 12V power
> supply ground. This shared ground reference is essential for the MOSFETs to work.

---

## Accessing the GPIO Pins

### JST Connector (bottom-right of board) - Easy

Your CYD has a 4-pin JST-SH 1.0mm connector on the bottom-right with:

```
  ┌──────────────────┐
  │  GND  IO22  IO27  3.3V  │   (JST-SH 1.0mm, 4-pin)
  └──────────────────┘
```

Use a JST-SH cable or solder Dupont wires to a matching plug. This gives you:
- **IO27** → OneWire bus for both DS18B20 sensors
- **IO22** → Fan 1 MOSFET gate
- **3.3V** → Sensor VCC + 4.7kΩ pull-up
- **GND** → Common ground

### RGB LED Pads (back of board) - Requires Soldering

The RGB LED on the back of your board is driven by GPIO 4, 16, 17. To use
these pins for MOSFET control:

1. **Locate the RGB LED** - it's the clear/white SMD component near the
   bottom-right on the back of the board (near the ESP32 module)

2. **Desolder or cut traces** - either:
   - Remove the RGB LED entirely with a soldering iron, OR
   - Leave the LED in place (it will just glow with the PWM signals - harmless
     but may be distracting)

3. **Solder thin wires** to the pads/traces leading to each LED pin:
   - **GPIO 4** pad (Red LED)   → Fan 2 MOSFET gate signal
   - **GPIO 16** pad (Green LED) → Peltier 1 MOSFET gate signal
   - **GPIO 17** pad (Blue LED)  → Peltier 2 MOSFET gate signal

> **Tip**: Use 30AWG silicone wire - it's flexible and easy to solder to
> small pads. Follow the traces from the LED back toward the resistors
> (R19, R5, R16 area) - you can solder to the resistor pads instead of
> the LED pads if they're easier to access.

```
  Back of CYD (near bottom-right):

      R19          R5          R16
   ┌──┤├──┐    ┌──┤├──┐    ┌──┤├──┐
   │ GPIO4 │    │GPIO16│    │GPIO17│
   │ (Red) │    │(Green│    │(Blue)│
   └───┬───┘    └───┬──┘    └───┬──┘
       │            │           │
       └────────┬───┴───────────┘
            [RGB LED]

   Solder your wires at the resistor pads (marked with X):

       X──[R19]──LED    → wire to Fan 2 MOSFET
       X──[R5 ]──LED    → wire to Peltier 1 MOSFET
       X──[R16]──LED    → wire to Peltier 2 MOSFET
```

---

## Wiring Diagrams

### DS18B20 Temperature Sensors (both on same OneWire bus)

```
3.3V (from JST) ──┬──[4.7kΩ]──┬── GPIO 27 (JST connector)
                   │            │
              ┌────┴───┐  ┌────┴───┐
              │DS18B20 │  │DS18B20 │
              │ VCC DQ │  │ VCC DQ │
              │  GND   │  │  GND   │
              └───┬────┘  └───┬────┘
                  │            │
                 GND          GND (from JST)
```

DS18B20 wires (waterproof probe type):
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
  12V PSU ─────────┤ 12V+        GND ├─────── Common GND
                    └──┬──┬──┬──┬────┘
                       │  │  │  │
                    ┌──┴──┴──┴──┴────────────────────┐
                    │  P1   P2   F1   F2  (12V loads)│
                    │  │    │    │    │               │
                    │ [M1] [M2] [M3] [M4] MOSFETs    │
                    │  │    │    │    │               │
                    └──┼────┼────┼────┼───────────────┘
                       │    │    │    │
                      G16  G17  G22  G4
                       │    │    │    │
    ┌──────────┐       │    │    │    │
    │   CYD    │       │    │    │    │
    │ ESP32    │───────┘    │    │    │
    │ Display  │────────────┘    │    │
    │          │  (RGB LED pads) │    │
    │          │                 │    │
    │   [JST]──┤── G22 ─────────┘    │
    │          ├── G27 ──── DS18B20 sensors (OneWire)
    │          ├── 3.3V ─── Sensor VCC + pull-up
    │          ├── GND ──── Common GND
    │          │                      │
    │ RGB LED──┤── G4 ───────────────-┘
    │   pads   │
    └──────────┘
```

---

## Pins Used by the CYD (DO NOT USE)

| GPIO | Used For |
|------|----------|
| 2 | TFT DC (data/command) |
| 12 | TFT SPI MISO |
| 13 | TFT SPI MOSI |
| 14 | TFT SPI CLK |
| 15 | TFT SPI CS |
| 21 | TFT backlight |
| 25 | DAC / audio |
| 33 | Touch SPI CS (XPT2046) |
| 34 | LDR (light sensor, input only) |
| 35 | Input only (some boards) |

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

1. **Test sensors first** - wire just the DS18B20s via the JST connector, flash
   the code, and confirm you see valid temperatures on the display before
   soldering RGB LED pads or wiring MOSFETs.

2. **Start with the JST connector** - plug in a cable and wire up the DS18B20
   sensors + Fan 1 MOSFET. Get those working before soldering to the LED pads.

3. **Heat sinks on MOSFETs** - if running peltiers at high duty cycles, the
   IRLZ44N MOSFETs may get warm. Small clip-on heat sinks help.

4. **Thick wires for peltiers** - use 16-18 AWG wire for the 12V power to
   peltiers. Thin jumper wires will overheat and melt.

5. **Waterproof the DS18B20s** - since you're mounting near a water cooling
   loop, use the waterproof stainless steel probe versions, not bare TO-92.

6. **Peltier orientation** - the cold side has printing on it. Make sure cold
   side faces the water block, hot side faces a heat sink with airflow.

7. **Common ground** - the single most common failure. The ESP32 GND, 12V PSU
   GND, MOSFET sources, and sensor GND must ALL be connected together.

8. **RGB LED** - you can leave the LED in place. It will glow with the PWM
   signals (blue/green pulses when peltiers are on, red when fan 2 is on).
   It actually makes a handy status indicator! Only remove it if you want
   a cleaner signal or it annoys you.
