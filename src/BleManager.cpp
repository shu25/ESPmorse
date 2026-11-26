#include "BleManager.h"
#include "CommandProcessor.h"
#include "Config.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLESecurity.h>
#include <WiFi.h>
#include "RuntimeSettings.h"
#include <string.h>

BleManager ble;
static BLECharacteristic* settingsCharacteristic = nullptr;
static BLECharacteristic* eventCharacteristic = nullptr;

// Custom ESPmorse service. The phone writes an ASCII command and reads/notifies
// the ASCII response on the same characteristic.
static const char* SERVICE_UUID = "6f3a0001-8f7a-4b3d-9d64-4553504d4f52";
static const char* SETTINGS_UUID = "6f3a0002-8f7a-4b3d-9d64-4553504d4f52";
static const char* EVENT_UUID = "6f3a0003-8f7a-4b3d-9d64-4553504d4f52";

class SettingsCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override {
        std::string value = characteristic->getValue();
        if (!value.empty()) ble.queueCommand(String(value.c_str()));
    }
};

class EspMorseServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* server) override {
        (void)server;
        DBG_PRINTLN("BLE client connected");
    }
    void onDisconnect(BLEServer* server) override {
        (void)server;
        DBG_PRINTLN("BLE client disconnected");
        BLEDevice::startAdvertising();
    }
};

BleManager::BleManager() {}

void BleManager::begin() {
    String macSuffix = WiFi.macAddress();
    macSuffix.replace(":", "");
    if (macSuffix.length() > 4) macSuffix = macSuffix.substring(macSuffix.length() - 4);
    String deviceName = String("ESPmorse-") + settings.get().callsign + "-" + macSuffix;
    BLEDevice::init(deviceName.c_str());

    BLEDevice::setMTU(247);
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    BLESecurity* security = new BLESecurity();
    security->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
    security->setCapability(ESP_IO_CAP_NONE);
    BLEServer* server = BLEDevice::createServer();
    server->setCallbacks(new EspMorseServerCallbacks());
    BLEService* service = server->createService(SERVICE_UUID);
    settingsCharacteristic = service->createCharacteristic(
        SETTINGS_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY);
    settingsCharacteristic->setCallbacks(new SettingsCallbacks());
    settingsCharacteristic->addDescriptor(new BLE2902());
    settingsCharacteristic->setValue(commands.status().c_str());
    eventCharacteristic = service->createCharacteristic(
        EVENT_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    eventCharacteristic->addDescriptor(new BLE2902());
    eventCharacteristic->setValue("READY");
    service->start();
    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->start();
    DBG_PRINTF("BLE settings ready: %s\n", deviceName.c_str());
}

void BleManager::update() {
    if (!hasInput()) return;
    String response = commands.execute(getInput());
    if (settingsCharacteristic != nullptr) {
        settingsCharacteristic->setValue(response.c_str());
        settingsCharacteristic->notify();
    }
    DBG_PRINTF("[BLE] %s\n", response.c_str());
}

bool BleManager::hasInput() {
    portENTER_CRITICAL(&commandMux);
    bool available = commandAvailable;
    portEXIT_CRITICAL(&commandMux);
    return available;
}

String BleManager::getInput() {
    char command[81];
    portENTER_CRITICAL(&commandMux);
    memcpy(command, pendingCommand, sizeof(command));
    pendingCommand[0] = '\0';
    commandAvailable = false;
    portEXIT_CRITICAL(&commandMux);
    return String(command);
}

void BleManager::queueCommand(const String& command) {
    portENTER_CRITICAL(&commandMux);
    strncpy(pendingCommand, command.c_str(), sizeof(pendingCommand) - 1);
    pendingCommand[sizeof(pendingCommand) - 1] = '\0';
    commandAvailable = true;
    portEXIT_CRITICAL(&commandMux);
}

void BleManager::publishEvent(const String& event) {
    if (eventCharacteristic == nullptr) return;
    eventCharacteristic->setValue(event.c_str());
    eventCharacteristic->notify();
}
