# Rover Sensor (Vishwakarma Awards 2026-27)

ESP8266 (NodeMCU 1.0) sketch for a sensor-fusion rover. It reads an ADXL345 accelerometer, a vibration sensor and a sound sensor, streams the readings as CSV over USB serial, and drives a DC motor through an L293D motor driver module.

## Hardware

| Part | Notes |
|---|---|
| NodeMCU ESP8266 (ESP-12E) | 3.3 V logic, not 5 V tolerant |
| ADXL345 accelerometer module | I2C, address 0x53 |
| Vibration sensor module | Digital output (DO) |
| Sound sensor module | Analog output (AO) used |
| L293D motor driver module | PWM goes directly on its input pins |
| DC gear motor and battery | Li-ion cells for the motor supply |

## Software setup

1. Install Arduino IDE 2.x.
2. In Preferences, add this to "Additional boards manager URLs": `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
3. In Boards Manager, install **esp8266 by ESP8266 Community**.
4. Select the board **NodeMCU 1.0 (ESP-12E Module)**.
5. In Library Manager, install **Adafruit ADXL345** and **Adafruit Unified Sensor** (accept the dependencies).
6. Open the sketch (`.ino` file), upload it, then open the Serial Monitor at **115200** baud.

## Wiring

### Signal pins

| Function | NodeMCU pin |
|---|---|
| Vibration sensor DO | D0 |
| Sound sensor AO | A0 |
| Motor 1 (L293D inputs A and B) | D5, D6 |
| Motor 2 (L293D inputs A and B) | D7, D8 |
| ADXL345 SDA / SCL | D2 / D1 |

The ESP8266 has only one analog pin, so the sound sensor uses A0 and the vibration sensor is read digitally on D0.

### Power and ground

- ADXL345: VCC to 3V3, GND to G, **CS to 3V3** (selects I2C mode), **SDO to G** (address 0x53).
- Vibration sensor: VCC to 3V3, GND to G.
- Sound sensor: + to 3V3, G to G. Leave its D0 pin unconnected.
- L293D module: **+5V to the NodeMCU VIN pin**, GND to G, battery positive to the **9/12V** pin.
- Join the battery negative, the L293D GND and the NodeMCU G (common ground).
- Power all sensors from 3.3 V only. Never connect the battery to the L293D +5V pin.

## Serial output

The sketch prints one CSV line about every 50 ms (roughly 20 per second). Lines starting with `#` are comments.

`t_ms,ax,ay,az,amag,vib_state,vib_edges,sound_pp`

| Column | Meaning |
|---|---|
| `t_ms` | Time since start, in milliseconds |
| `ax, ay, az` | Acceleration in m/s^2 |
| `amag` | Total acceleration (about 9.8 when still) |
| `vib_state` | Current level of the vibration sensor pin |
| `vib_edges` | Pin changes during the 50 ms window |
| `sound_pp` | Sound peak-to-peak level (0 to 1023) |

## Motor keys (Serial Monitor)

Type a key and press Enter (set the line ending to "Newline").

| Key | Action |
|---|---|
| `w` | Forward |
| `s` | Back |
| `a` | Left |
| `d` | Right |
| `x` | Stop |
| `+` / `-` | Faster / slower |

## Safety notes

- Lift the wheels for the first motor tests, and keep test runs short. The L293D is rated about 600 mA per channel.
- Power off before changing motor wiring.
- Keep the motor on its own battery, not on the USB or board power.
- If the board resets when the motor starts, add a 470-1000 uF capacitor across the motor supply.
- Do not leave Li-ion cells connected and unattended.

## Status

The sensors and motor driver are wired on a breadboard prototype. A Raspberry Pi 5 with a night-vision camera is planned as the next stage.
