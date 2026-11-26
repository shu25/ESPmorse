#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// User settings
#define CALLSIGN        "JA25TS"
#define RADIO_MODE      1 // 0: REALTIME, 1: BURST
#define TARGET_MODE     0 // 0: BROADCAST, 1: DIRECT
#define DEFAULT_TARGET  "ALL"
#define ENABLE_SERIAL_LOG 1

#if ENABLE_SERIAL_LOG
#define DBG_BEGIN(baud) Serial.begin(baud)
#define DBG_PRINT(...) Serial.print(__VA_ARGS__)
#define DBG_PRINTLN(...) Serial.println(__VA_ARGS__)
#define DBG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DBG_BEGIN(baud) do {} while (0)
#define DBG_PRINT(...) do {} while (0)
#define DBG_PRINTLN(...) do {} while (0)
#define DBG_PRINTF(...) do {} while (0)
#endif

#define RADIO_MODE_REALTIME 0
#define RADIO_MODE_BURST    1
#define TARGET_MODE_BROADCAST 0
#define TARGET_MODE_DIRECT    1

// CW defaults. International Morse timing is always 1:3:7.
#define DEFAULT_CW_SPEED_WPM  10
#define MIN_CW_SPEED_WPM       5
#define MAX_CW_SPEED_WPM      40
#define KEY_DEBOUNCE_MS       5
#define DOT_DASH_THRESHOLD_UNITS 2

// Audio
#define DEFAULT_SIDETONE_FREQ    700
#define DEFAULT_RX_TONE_FREQ     600
#define DEFAULT_RX_CW_SPEED_WPM  18
#define MIN_TONE_FREQ            300
#define MAX_TONE_FREQ           1200
#define BEEP_CHANNEL     0
#define BEEP_RESOLUTION  8
#define BEEP_VOLUME      128 // 0-255

// Pins
#define KEY_PIN     4
#define BUZZER_PIN  5
#define LED_PIN     6
#define STATUS_LED_PIN 3
#define BATTERY_PIN 0
#define BEEP_OFF_BUTTON_PIN 7
#define SEND_CONFIRM_BUTTON_PIN 10
#define LED_ACTIVE_LOW 0
#define LED_IDLE_BLINK 0

#define DEFAULT_LOW_BATTERY_MV 3400
#define DEFAULT_BATTERY_DIVIDER 2.0f

// Radio
#define ACK_TIMEOUT_MS   500
#define MAX_RETRIES      3
#define PACKET_MAX_DATA  32

#endif // CONFIG_H
