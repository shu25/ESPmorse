# ESPmorse

ESP32-C3 Morse input and ESP-NOW transport.

## Morse timing

Timing follows ITU-R M.1677: dot 1 unit, dash 3 units, spacing inside a
character 1 unit, between characters 3 units, and between words 7 units.
Changing WPM changes the unit duration (`1200 / WPM`) without changing these
ratios.

## Runtime settings

Settings are stored in ESP32 NVS and survive a restart. Commands can be sent
over USB serial (115200 baud) or the BLE settings characteristic.

Commands:

```text
STATUS
CW_WPM=10
RX_WPM=18
SIDETONE_HZ=700
RX_TONE_HZ=600
MUTED=0
CALLSIGN=JA25TS
SEND=HELLO
HISTORY
BATTERY=0
LOW_BATTERY_MV=3400
BATTERY_DIV_X100=200
SLEEP_MIN=0
BLE_PASSKEY=123456
RESET
```

WPM range is 5-40. Tone range is 300-1200 Hz. Each command returns either the
complete current status or an `ERR` response.

## BLE interface

- Device name: `ESPmorse-JA25TS`
- Service UUID: `6f3a0001-8f7a-4b3d-9d64-4553504d4f52`
- Settings characteristic: `6f3a0002-8f7a-4b3d-9d64-4553504d4f52`
- Event/history characteristic: `6f3a0003-8f7a-4b3d-9d64-4553504d4f52`
- Settings properties: read, write, notify
- Event properties: read, notify
- Encoding: UTF-8/ASCII command text
- Pairing: OS-managed encrypted bonding (no PIN prompt)

A future phone app only needs to write one of the commands above and display
the response received by notification or read.

`SEND=` accepts 1-31 characters. Live `TX`, `RX`, `ACK`, retry, failure,
battery, and sleep events are sent through the event characteristic. Callsign
and BLE PIN changes take full effect after restart.

## Pins

- GPIO4: Morse key to GND
- GPIO5: passive piezo buzzer
- GPIO6: TX/RX activity LED through a resistor
- GPIO3: pending/ACK/error status LED through a resistor
- GPIO7: mute button to GND
- GPIO10: send button to GND (short press sends, 0.8 s hold cancels queued text)
- GPIO0: optional battery divider input

Battery monitoring is disabled by default. Never connect a battery directly to
GPIO0. Use a resistor divider that keeps the pin at or below 3.3 V, then set
`BATTERY_DIV_X100` to the divider ratio multiplied by 100. For a 1:1 divider,
the ratio is 2.00 and the command value is `200`.

`SLEEP_MIN=0` disables sleep. A non-zero value enters deep sleep after that many
inactive minutes, only when no text is pending and no ACK is outstanding. Press
the Morse key to wake the unit.
