#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>

class LedManager {
public:
    LedManager();
    void begin();
    void update();

    void txFlash();
    void rxFlash();
    void ackFlash();
    void errorFlash();
    void setPending(bool pending);
    void retryFlash();

private:
    uint32_t flashStartTime;
    uint32_t flashDuration;
    bool isFlashing;
    uint32_t lastBlinkTime;
    bool blinkState;
    bool pendingState;
    uint32_t statusFlashUntil;

    void ledOn();
    void ledOff();
    void statusWrite(bool on);
};

extern LedManager led;

#endif // LED_MANAGER_H
