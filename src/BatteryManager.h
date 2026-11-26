#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Arduino.h>

class BatteryManager {
public:
    void begin();
    void update();
    uint16_t millivolts() const;
    bool takeLowAlert();
private:
    uint16_t lastMillivolts = 0;
    uint32_t lastReadAt = 0;
    bool lowAlert = false;
};

extern BatteryManager battery;
#endif
