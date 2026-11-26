#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>

class BleManager {
public:
    BleManager();
    void begin();
    void update();

    bool hasInput();
    String getInput();
    void queueCommand(const String& command);
    void publishEvent(const String& event);

private:
    char pendingCommand[81] = {0};
    bool commandAvailable = false;
    portMUX_TYPE commandMux = portMUX_INITIALIZER_UNLOCKED;
};

extern BleManager ble;

#endif // BLE_MANAGER_H
