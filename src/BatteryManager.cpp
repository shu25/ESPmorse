#include "BatteryManager.h"
#include "Config.h"
#include "RuntimeSettings.h"

BatteryManager battery;

void BatteryManager::begin() {
    pinMode(BATTERY_PIN, INPUT);
    lastReadAt = millis() - 60000UL;
}

void BatteryManager::update() {
    if (!settings.get().batteryEnabled || millis() - lastReadAt < 60000UL) return;
    lastReadAt = millis();
    uint32_t pinMv = analogReadMilliVolts(BATTERY_PIN);
    lastMillivolts = (pinMv * settings.get().batteryDividerX100) / 100U;
    if (lastMillivolts > 0 && lastMillivolts < settings.get().lowBatteryMv) lowAlert = true;
}

uint16_t BatteryManager::millivolts() const { return lastMillivolts; }
bool BatteryManager::takeLowAlert() { bool value = lowAlert; lowAlert = false; return value; }
