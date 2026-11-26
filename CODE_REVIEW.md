# ESPmorse Code Review

## Current Shape

This is an ESP32-C3 PlatformIO project for Morse input and ESP-NOW transport.

- `main.cpp` wires together Morse input, sidetone, LED feedback, and radio TX/RX.
- `MorseEngine` debounces the key, classifies dot/dash timing, decodes letters, and emits word events.
- `RadioManager` sends ESP-NOW packets and handles ACK/retry for burst text mode.
- `BeepManager` and `LedManager` provide local feedback.
- `BleManager`, `DisplayManager`, `Logger`, and `Compression` are present but not currently connected to the main flow.

## Fixed During Cleanup

- Repaired mojibake comments that had swallowed declarations in `MorseTable.h`, `Logger.h`, and `BleManager.h`.
- Removed unused broken Morse macro scaffolding from `MorseTable.h`.
- Cleaned placeholder headers so they declare the methods implemented by their `.cpp` files.

## Review Notes

- `MorseEngine` exposes only one pending event. If dot/dash/letter/word events are produced before the main loop consumes the previous event, the older event can be overwritten. A small ring buffer would make this more robust.
- `RadioManager` has only one receive slot. Multiple incoming packets before `main.cpp` drains them can overwrite each other.
- ESP-NOW receive/send callbacks call into manager code that touches shared state and LED methods. For long-term stability, callbacks should only enqueue small events and let `update()` process side effects.
- `TARGET_MODE` exists but `RadioManager` always fills `DEFAULT_TARGET`; direct targeting is not implemented yet.
- `Logger`, `DisplayManager`, `BleManager`, and `Compression` look like planned features. They should either be wired into the product flow or moved out until needed.
- `PACKET_MAX_DATA` is 32 bytes, so burst words longer than 31 visible characters are truncated. That may be okay for short Morse words, but it should be intentional.
- `String` is used in several long-lived paths. On small microcontrollers this can fragment heap over time; fixed buffers are worth considering once behavior settles.

## Suggested Next Steps

1. Add a small event queue to `MorseEngine` and `RadioManager`.
2. Move ESP-NOW callback side effects into `RadioManager::update()`.
3. Decide whether this device should be burst-only first, then remove or finish realtime/direct-mode branches accordingly.
4. Wire `Logger` into TX/RX events if serial history is useful.
5. Add a serial debug mode command or BLE input only after the core Morse/radio path is stable.
6. Install or configure PlatformIO CLI locally and run `pio run` as the baseline build check.
