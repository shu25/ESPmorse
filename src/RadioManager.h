#ifndef RADIO_MANAGER_H
#define RADIO_MANAGER_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "Packet.h"

enum RadioTxEvent : uint8_t { TX_EVENT_NONE, TX_EVENT_RETRY, TX_EVENT_ACK, TX_EVENT_FAILED };

class RadioManager {
public:
    RadioManager();
    void begin();
    void update();

    void sendText(const String& text);

    void sendDot();
    void sendDash();
    void sendLetter();
    void sendWord();

    bool hasReceived();
    Packet getReceived();
    RadioTxEvent getTxEvent();
    bool isBusy() const;

private:
    struct RxEnvelope {
        Packet packet;
        uint8_t mac[6];
    };
    static const uint8_t RX_QUEUE_SIZE = 8;

    uint16_t txSequence;
    RxEnvelope callbackQueue[RX_QUEUE_SIZE];
    volatile uint8_t callbackHead;
    volatile uint8_t callbackTail;
    volatile uint8_t callbackCount;
    portMUX_TYPE callbackMux = portMUX_INITIALIZER_UNLOCKED;
    Packet receiveQueue[RX_QUEUE_SIZE];
    uint8_t receiveHead;
    uint8_t receiveTail;
    uint8_t receiveCount;
    struct SenderSequence { char sender[8]; uint16_t sequence; bool used; };
    SenderSequence senderSequences[8];
    volatile RadioTxEvent txEvent;
    volatile bool txStatusPending;
    volatile esp_now_send_status_t pendingTxStatus;

    bool isWaitingAck;
    uint32_t lastTxTime;
    uint8_t retryCount;
    Packet lastTxPacket;

    uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t localMac[6] = {0, 0, 0, 0, 0, 0};

    void sendPacket(PacketType type, const uint8_t* data = nullptr, size_t dataLen = 0);
    void sendAck(const uint8_t* mac_addr, const Packet& pkt);
    bool ensurePeer(const uint8_t* mac_addr);

    static void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int data_len);
    static void onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status);

    void handleRxData(const uint8_t* mac_addr, const uint8_t* data, int data_len);
    void processReceived(const RxEnvelope& envelope);
    void handleTxStatus(const uint8_t* mac_addr, esp_now_send_status_t status);
};

extern RadioManager radio;

#endif // RADIO_MANAGER_H
