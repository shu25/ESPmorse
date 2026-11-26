#include "LedManager.h"
#include "Config.h"

LedManager led;

LedManager::LedManager() {
    flashStartTime = 0;
    flashDuration = 0;
    isFlashing = false;
    lastBlinkTime = 0;
    blinkState = false;
    pendingState = false;
    statusFlashUntil = 0;
}

void LedManager::begin() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(STATUS_LED_PIN, OUTPUT);
    ledOff();
    statusWrite(false);
}

void LedManager::update() {
    uint32_t now = millis();
    statusWrite(statusFlashUntil > now || pendingState);

    if (isFlashing) {
        if (now - flashStartTime >= flashDuration) {
            isFlashing = false;
            ledOff();
        } else {
            return;
        }
    }

#if LED_IDLE_BLINK
    if (now - lastBlinkTime > 2000) {
        lastBlinkTime = now;
        ledOn();
        blinkState = true;
    } else if (blinkState && (now - lastBlinkTime > 100)) {
        ledOff();
        blinkState = false;
    }
#endif
}

void LedManager::txFlash() {
    ledOn();
    flashStartTime = millis();
    flashDuration = 200;
    isFlashing = true;
}

void LedManager::rxFlash() {
    ledOn();
    flashStartTime = millis();
    flashDuration = 100;
    isFlashing = true;
}

void LedManager::ackFlash() {
    ledOn();
    flashStartTime = millis();
    flashDuration = 50;
    isFlashing = true;
}

void LedManager::errorFlash() {
    ledOn();
    flashStartTime = millis();
    flashDuration = 1000;
    isFlashing = true;
    statusFlashUntil = millis() + 1000;
}

void LedManager::setPending(bool pending) { pendingState = pending; }
void LedManager::retryFlash() { statusFlashUntil = millis() + 120; }

void LedManager::ledOn() {
    digitalWrite(LED_PIN, LED_ACTIVE_LOW ? LOW : HIGH);
}

void LedManager::ledOff() {
    digitalWrite(LED_PIN, LED_ACTIVE_LOW ? HIGH : LOW);
}

void LedManager::statusWrite(bool on) {
    digitalWrite(STATUS_LED_PIN, on ? HIGH : LOW);
}
