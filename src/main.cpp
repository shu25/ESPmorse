#include <Arduino.h>
#include "Config.h"
#include "MorseEngine.h"
#include "BeepManager.h"
#include "LedManager.h"
#include "RadioManager.h"
#include "esp_system.h"
#include "RuntimeSettings.h"
#include "CommandProcessor.h"
#include "BleManager.h"
#include "Logger.h"
#include "BatteryManager.h"
#include <esp_sleep.h>

struct ButtonState {
    bool lastReading = HIGH;
    bool stableState = HIGH;
    uint32_t changedAt = 0;

    bool wasPressed(uint8_t pin) {
        bool reading = digitalRead(pin);
        if (reading != lastReading) {
            changedAt = millis();
            lastReading = reading;
        }
        if (millis() - changedAt >= KEY_DEBOUNCE_MS && reading != stableState) {
            stableState = reading;
            return stableState == LOW;
        }
        return false;
    }
};

ButtonState beepButton;

struct SendButtonState {
    bool lastReading = HIGH;
    bool stableState = HIGH;
    bool longHandled = false;
    uint32_t changedAt = 0;
    uint32_t pressedAt = 0;

    uint8_t update(uint8_t pin) {
        bool reading = digitalRead(pin);
        if (reading != lastReading) { changedAt = millis(); lastReading = reading; }
        if (millis() - changedAt >= KEY_DEBOUNCE_MS && reading != stableState) {
            stableState = reading;
            if (stableState == LOW) { pressedAt = millis(); longHandled = false; }
            else if (!longHandled) return 1;
        }
        if (stableState == LOW && !longHandled && millis() - pressedAt >= 800) {
            longHandled = true;
            return 2;
        }
        return 0;
    }
};

struct TextQueue {
    String items[8]; uint8_t head = 0, tail = 0, count = 0;
    bool push(const String& text) { if (count >= 8) return false; items[tail] = text; tail = (tail + 1) % 8; count++; return true; }
    String pop() { if (!count) return ""; String s = items[head]; items[head] = ""; head = (head + 1) % 8; count--; return s; }
    void clear() { while (count) pop(); }
};

SendButtonState sendButton;
TextQueue txQueue;
String pendingText;
String serialCommand;
uint32_t lastActivityAt = 0;

void queueText(const String& text) {
    if (text.length() > 0 && txQueue.push(text)) {
        led.setPending(true);
        DBG_PRINTF("[QUEUED]: %s\n", text.c_str());
    } else if (text.length() > 0) {
        beep.playErrorTone();
        ble.publishEvent("ERROR TX_QUEUE_FULL");
    }
}

void setup() {
#if ENABLE_SERIAL_LOG
    DBG_BEGIN(115200);
    delay(3000);
    DBG_PRINTF("\nESP32-C3 SuperMini Morse System\nReset reason: %d\n", esp_reset_reason());
#endif

    settings.begin();

    morse.begin();
    beep.begin();
    led.begin();
    pinMode(BEEP_OFF_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SEND_CONFIRM_BUTTON_PIN, INPUT_PULLUP);
    radio.begin();
    ble.begin();
    battery.begin();

    beep.playBootTone();
    DBG_PRINTLN(commands.status());
    lastActivityAt = millis();
}

void loop() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        lastActivityAt = millis();
        if (c == '\n' || c == '\r') {
            if (serialCommand.length() > 0) {
                DBG_PRINTLN(commands.execute(serialCommand));
                serialCommand = "";
            }
        } else if (serialCommand.length() < 80) {
            serialCommand += c;
        }
    }
    morse.update();
    beep.update();
    led.update();
    radio.update();
    ble.update();
    battery.update();

    if (commands.hasSendRequest()) queueText(commands.getSendRequest());

    if (!radio.isBusy() && txQueue.count > 0) {
        String text = txQueue.pop();
        radio.sendText(text);
        logger.logTx(text);
        led.txFlash();
        beep.playSendTone();
        ble.publishEvent("TX " + text);
        DBG_PRINTF("[TX]: %s\n", text.c_str());
        lastActivityAt = millis();
    }

    RadioTxEvent txEvent = radio.getTxEvent();
    if (txEvent == TX_EVENT_RETRY) { beep.playRetryTone(); led.retryFlash(); ble.publishEvent("TX RETRY"); }
    else if (txEvent == TX_EVENT_ACK) { beep.playAckTone(); ble.publishEvent("TX ACK"); }
    else if (txEvent == TX_EVENT_FAILED) { beep.playErrorTone(); ble.publishEvent("TX FAILED"); }

    if (morse.isKeyDown()) {
        lastActivityAt = millis();
        beep.startSidetone();
    } else {
        beep.stopSidetone();
    }

    if (beepButton.wasPressed(BEEP_OFF_BUTTON_PIN)) {
        beep.toggleMuted();
        DBG_PRINTF("Beep %s\n", beep.isMuted() ? "OFF" : "ON");
    }

    uint8_t sendAction = sendButton.update(SEND_CONFIRM_BUTTON_PIN);
    if (sendAction == 1 && pendingText.length() > 0) {
        queueText(pendingText);
        pendingText = "";
        lastActivityAt = millis();
    } else if (sendAction == 2) {
        pendingText = "";
        txQueue.clear();
        led.setPending(false);
        beep.playErrorTone();
        ble.publishEvent("TX CANCELLED");
        DBG_PRINTLN("[TX CANCELLED]");
        lastActivityAt = millis();
    }

    while (morse.hasEvent()) {
        MorseEvent evt = morse.getEvent();
        switch (evt.type) {
            case EVT_DOT:
                DBG_PRINT(".");
                if (RADIO_MODE == RADIO_MODE_REALTIME) radio.sendDot();
                break;
            case EVT_DASH:
                DBG_PRINT("-");
                if (RADIO_MODE == RADIO_MODE_REALTIME) radio.sendDash();
                break;
            case EVT_LETTER:
                DBG_PRINTF(" -> %c\n", evt.letter);
                if (RADIO_MODE == RADIO_MODE_REALTIME) radio.sendLetter();
                break;
            case EVT_WORD:
                DBG_PRINTF("\n[WORD]: %s\n", evt.word.c_str());
                if (RADIO_MODE == RADIO_MODE_BURST) {
                    if (pendingText.length() > 0 && pendingText.length() + evt.word.length() + 1 < PACKET_MAX_DATA) pendingText += " ";
                    if (pendingText.length() + evt.word.length() < PACKET_MAX_DATA) pendingText += evt.word;
                    else { queueText(pendingText); pendingText = evt.word; }
                    led.setPending(true);
                    DBG_PRINTF("[READY TO SEND]: %s\n", pendingText.c_str());
                } else {
                    led.txFlash();
                    radio.sendWord();
                }
                break;
        }
    }

    while (radio.hasReceived()) {
        Packet pkt = radio.getReceived();

        switch (pkt.type) {
            case PKT_DOT:
                beep.playDot();
                led.rxFlash();
                break;
            case PKT_DASH:
                beep.playDash();
                led.rxFlash();
                break;
            case PKT_LETTER:
                break;
            case PKT_WORD:
                break;
            case PKT_TEXT:
                DBG_PRINTF("\n[RX from %s]: %s\n", pkt.sender, (char*)pkt.data);
                logger.logRx(String(pkt.sender), String((char*)pkt.data));
                beep.playReceivedText(String((char*)pkt.data));
                led.rxFlash();
                ble.publishEvent("RX " + String(pkt.sender) + " " + String((char*)pkt.data));
                lastActivityAt = millis();
                break;
            case PKT_ACK:
                led.ackFlash();
                break;
            default:
                break;
        }
    }

    led.setPending(pendingText.length() > 0 || txQueue.count > 0 || radio.isBusy());

    if (battery.takeLowAlert()) {
        beep.playErrorTone();
        led.errorFlash();
        ble.publishEvent("BATTERY LOW " + String(battery.millivolts()) + "mV");
    }

    uint16_t sleepMinutes = settings.get().autoSleepMinutes;
    if (sleepMinutes > 0 && pendingText.length() == 0 && txQueue.count == 0 && !radio.isBusy() &&
        millis() - lastActivityAt >= (uint32_t)sleepMinutes * 60000UL) {
        ble.publishEvent("SLEEP");
        delay(100);
        esp_deep_sleep_enable_gpio_wakeup(1ULL << KEY_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
        esp_deep_sleep_start();
    }
}
