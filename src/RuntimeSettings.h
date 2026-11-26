#ifndef RUNTIME_SETTINGS_H
#define RUNTIME_SETTINGS_H

#include <Arduino.h>

struct DeviceSettings {
    char callsign[8];
    uint8_t cwWpm;
    uint8_t rxWpm;
    uint16_t sidetoneHz;
    uint16_t rxToneHz;
    uint16_t txLowHz, txHighHz, rxNoticeHighHz, rxNoticeLowHz, ackHz, retryHz, errorHz;
    bool muted;
    bool batteryEnabled;
    uint16_t lowBatteryMv;
    uint16_t batteryDividerX100;
    uint16_t autoSleepMinutes;
    uint32_t blePasskey;
};

class RuntimeSettings {
public:
    void begin();
    const DeviceSettings& get() const;
    bool setCwWpm(int value);
    bool setRxWpm(int value);
    bool setSidetoneHz(int value);
    bool setRxToneHz(int value);
    bool setAlertTone(const String& key, int value);
    bool setCallsign(const String& value);
    bool setBatteryEnabled(bool value);
    bool setLowBatteryMv(int value);
    bool setBatteryDividerX100(int value);
    bool setAutoSleepMinutes(int value);
    bool setBlePasskey(int value);
    void setMuted(bool value);
    void resetDefaults();
    uint32_t dotMs() const;
    uint32_t rxDotMs() const;

private:
    DeviceSettings values;
    void loadDefaults();
    void save();
};

extern RuntimeSettings settings;

#endif
