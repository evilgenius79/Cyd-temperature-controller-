# CYD Peltier Cooling Controller

ESP32-based water cooling controller using the CYD (Cheap Yellow Display, ESP32-2432S028) to monitor and control inline peltier cooling on a computer water loop.

## What It Does

- Reads 2x DS18B20 temperature sensors (inlet and outlet) with delta-T display
- Controls 2x 12V peltier modules via PWM through logic-level MOSFETs
- Controls 2x 40mm fans via PWM through logic-level MOSFETs
- Touchscreen UI with real-time temperature graphs and control bars
- **Auto mode**: proportional control ramps peltiers based on distance from target temp, fans scale with peltier load (min 30% when cooling)
- **Manual mode**: independently set each peltier and fan power level via +/- buttons
- Tracks min/max temperatures for each sensor with reset button
- High/low alarm limits with on-screen alert
- Settings screen to adjust target temp and alarm limits

## Hardware

- **CYD**: ESP32-2432S028 (Guition) - ESP32 with 2.8" ILI9341 touchscreen
- **Sensors**: 2x DS18B20 waterproof temperature probes
- **Cooling**: 2x 12V TEC1-12706 peltier modules
- **Fans**: 2x 40mm 12V fans on water block heat sink
- **Switching**: 4x IRLZ44N logic-level MOSFETs
- **Power**: 12V 10A+ DC power supply

See [WIRING.md](WIRING.md) for full parts list, pin assignments, wiring diagrams, and assembly instructions specific to the Guition ESP32-2432S028 board.

## Pin Map

| GPIO | Function | Access Point |
|------|----------|-------------|
| 27 | DS18B20 OneWire bus | JST connector |
| 22 | Fan 1 PWM | JST connector |
| 16 | Peltier 1 PWM | RGB LED pad (Green) |
| 17 | Peltier 2 PWM | RGB LED pad (Blue) |
| 4 | Fan 2 PWM | RGB LED pad (Red) |

## Building & Flashing

This is a [PlatformIO](https://platformio.org/) project.

```bash
# Build
pio run

# Flash
pio run --target upload

# Serial monitor
pio device monitor
```

Or open the project folder in VS Code with the PlatformIO extension and use the build/upload buttons.

## Project Structure

```
├── platformio.ini       # PlatformIO config (board, libs, display flags)
├── src/
│   ├── main.cpp         # Application code (UI, control logic, sensor reading)
│   └── config.h         # Pin assignments, PWM settings, defaults
├── WIRING.md            # Hardware wiring guide with diagrams
└── README.md
```

## License

MIT
