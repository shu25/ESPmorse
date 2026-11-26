#include "RadioManager.h"
#include "Config.h"
#include "LedManager.h"
#include <string.h>
#include "RuntimeSettings.h"

RadioManager radio;

RadioManager::RadioManager() {
    txSequence = 0;
    callbackHead = callbackTail = callbackCount = 0;
    receiveHead = receiveTail = receiveCount = 0;
    memset(senderSequences, 0, sizeof(senderSequences));
    txEvent = TX_EVENT_NONE;
    txStatusPending = false;
    pendingTxStatus = ESP_NOW_SEND_SUCCESS;
    isWaitingAck = false;
    lastTxTime = 0;
    retryCount = 0;
}

void RadioManager::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.macAddress(localMac);

    if (esp_now_init() != ESP_OK) {
        DBG_PRINTLN("ESP-NOW init failed");
        return;
    }

    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_err_t peerResult = esp_now_add_peer(&peerInfo);
    if (peerResult != ESP_OK && peerResult != ESP_ERR_ESPNOW_EXIST) {
        DBG_PRINTF("ESP-NOW add peer failed: %d\n", peerResult);
        return;
    }

    esp_now_register_recv_cb(onDataRecv);
    esp_now_register_send_cb(onDataSent);

    DBG_PRINTF("ESP-NOW ready. MAC=%s CALLSIGN=%s\n",
               WiFi.macAddress().c_str(),
               settings.get().callsign);
}

void RadioManager::update() {
    if (txStatusPending) {
        esp_now_send_status_t status = pendingTxStatus;
        txStatusPending = false;
        DBG_PRINTF("ESP-NOW tx status: %s\n",
                   status == ESP_NOW_SEND_SUCCESS ? "success" : "fail");
    }

    while (true) {
        RxEnvelope envelope;
        bool available = false;
        portENTER_CRITICAL(&callbackMux);
        if (callbackCount > 0) {
            envelope = callbackQueue[callbackHead];
            callbackHead = (callbackHead + 1) % RX_QUEUE_SIZE;
            callbackCount--;
            available = true;
        }
        portEXIT_CRITICAL(&callbackMux);
        if (!available) break;
        processReceived(envelope);
    }

    if (isWaitingAck && RADIO_MODE == RADIO_MODE_BURST) {
        if (millis() - lastTxTime > ACK_TIMEOUT_MS) {
            if (retryCount < MAX_RETRIES) {
                retryCount++;
                txEvent = TX_EVENT_RETRY;
                DBG_PRINTF("ACK timeout. Retrying %u/%u\n", retryCount, MAX_RETRIES);
                lastTxTime = millis();
                esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&lastTxPacket, sizeof(Packet));
                if (result != ESP_OK) {
                    DBG_PRINTF("ESP-NOW retry send failed: %d\n", result);
                }
            } else {
                DBG_PRINTLN("Max retries reached. Transmission failed.");
                isWaitingAck = false;
                led.errorFlash();
                txEvent = TX_EVENT_FAILED;
            }
        }
    }
}

void RadioManager::sendPacket(PacketType type, const uint8_t* data, size_t dataLen) {
    Packet pkt;
    memset(&pkt, 0, sizeof(Packet));
    pkt.sequence = txSequence++;
    pkt.type = type;
    strncpy(pkt.sender, settings.get().callsign, sizeof(pkt.sender) - 1);
    strncpy(pkt.target, DEFAULT_TARGET, sizeof(pkt.target) - 1);

    if (data != nullptr && dataLen > 0) {
        size_t copyLen = dataLen > PACKET_MAX_DATA ? PACKET_MAX_DATA : dataLen;
        memcpy(pkt.data, data, copyLen);
        pkt.data[PACKET_MAX_DATA - 1] = '\0';
    }

    if (type == PKT_TEXT) {
        lastTxPacket = pkt;
        isWaitingAck = true;
        lastTxTime = millis();
        retryCount = 0;
        DBG_PRINTF("TX text seq=%u target=%s data=%s\n",
                   pkt.sequence,
                   pkt.target,
                   (const char*)pkt.data);
    }

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&pkt, sizeof(Packet));
    if (result != ESP_OK) {
        DBG_PRINTF("ESP-NOW send failed: %d\n", result);
        if (type == PKT_TEXT) {
            isWaitingAck = false;
            led.errorFlash();
        }
    }
}

void RadioManager::sendText(const String& text) {
    sendPacket(PKT_TEXT, (const uint8_t*)text.c_str(), text.length() + 1);
}

void RadioManager::sendDot()    { sendPacket(PKT_DOT); }
void RadioManager::sendDash()   { sendPacket(PKT_DASH); }
void RadioManager::sendLetter() { sendPacket(PKT_LETTER); }
void RadioManager::sendWord()   { sendPacket(PKT_WORD); }

bool RadioManager::hasReceived() {
    return receiveCount > 0;
}

Packet RadioManager::getReceived() {
    Packet packet = {};
    if (receiveCount == 0) return packet;
    packet = receiveQueue[receiveHead];
    receiveHead = (receiveHead + 1) % RX_QUEUE_SIZE;
    receiveCount--;
    return packet;
}

RadioTxEvent RadioManager::getTxEvent() {
    RadioTxEvent event = txEvent;
    txEvent = TX_EVENT_NONE;
    return event;
}

bool RadioManager::isBusy() const { return isWaitingAck; }

void RadioManager::sendAck(const uint8_t* mac_addr, const Packet& pkt) {
    if (!ensurePeer(mac_addr)) {
        DBG_PRINTLN("ACK peer registration failed");
        return;
    }

    Packet ackPkt;
    memset(&ackPkt, 0, sizeof(Packet));
    ackPkt.sequence = txSequence++;
    ackPkt.type = PKT_ACK;
    strncpy(ackPkt.sender, settings.get().callsign, sizeof(ackPkt.sender) - 1);
    strncpy(ackPkt.target, pkt.sender, sizeof(ackPkt.target) - 1);
    memcpy(ackPkt.data, &pkt.sequence, sizeof(pkt.sequence));

    esp_err_t result = esp_now_send(mac_addr, (uint8_t*)&ackPkt, sizeof(Packet));
    if (result != ESP_OK) {
        DBG_PRINTF("ACK send failed: %d\n", result);
    }
}

bool RadioManager::ensurePeer(const uint8_t* mac_addr) {
    if (esp_now_is_peer_exist(mac_addr)) return true;

    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, mac_addr, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_err_t result = esp_now_add_peer(&peerInfo);
    if (result == ESP_OK || result == ESP_ERR_ESPNOW_EXIST) return true;

    DBG_PRINTF("ESP-NOW add ACK peer failed: %d\n", result);
    return false;
}

void RadioManager::handleRxData(const uint8_t* mac_addr, const uint8_t* data, int data_len) {
    if (data_len != sizeof(Packet)) return;
    RxEnvelope envelope;
    memcpy(&envelope.packet, data, sizeof(Packet));
    memcpy(envelope.mac, mac_addr, sizeof(envelope.mac));

    portENTER_CRITICAL(&callbackMux);
    if (callbackCount < RX_QUEUE_SIZE) {
        callbackQueue[callbackTail] = envelope;
        callbackTail = (callbackTail + 1) % RX_QUEUE_SIZE;
        callbackCount++;
    }
    portEXIT_CRITICAL(&callbackMux);
}

void RadioManager::processReceived(const RxEnvelope& envelope) {
    Packet pkt = envelope.packet;
    pkt.sender[sizeof(pkt.sender) - 1] = '\0';
    pkt.target[sizeof(pkt.target) - 1] = '\0';
    pkt.data[PACKET_MAX_DATA - 1] = '\0';

    if (strcmp(pkt.target, "ALL") != 0 && strcmp(pkt.target, settings.get().callsign) != 0) {
        return;
    }

    if (memcmp(envelope.mac, localMac, sizeof(localMac)) == 0) {
        DBG_PRINTLN("Ignored loopback packet from local MAC");
        return;
    }

    if (pkt.type == PKT_ACK) {
        uint16_t acknowledgedSequence = 0;
        memcpy(&acknowledgedSequence, pkt.data, sizeof(acknowledgedSequence));
        if (isWaitingAck && acknowledgedSequence == lastTxPacket.sequence) {
            isWaitingAck = false;
            DBG_PRINTF("ACK received from %s\n", pkt.sender);
            led.ackFlash();
            txEvent = TX_EVENT_ACK;
        }
        return;
    }

    if (pkt.type == PKT_TEXT && RADIO_MODE == RADIO_MODE_BURST) {
        sendAck(envelope.mac, pkt);
        int freeSlot = -1;
        for (int i = 0; i < 8; i++) {
            if (!senderSequences[i].used && freeSlot < 0) freeSlot = i;
            if (senderSequences[i].used && strcmp(senderSequences[i].sender, pkt.sender) == 0) {
                if (senderSequences[i].sequence == pkt.sequence) {
                    DBG_PRINTF("Duplicate text ignored: %s seq=%u\n", pkt.sender, pkt.sequence);
                    return;
                }
                senderSequences[i].sequence = pkt.sequence;
                freeSlot = -2;
                break;
            }
        }
        if (freeSlot >= 0) {
            senderSequences[freeSlot].used = true;
            senderSequences[freeSlot].sequence = pkt.sequence;
            strncpy(senderSequences[freeSlot].sender, pkt.sender, sizeof(senderSequences[freeSlot].sender) - 1);
        }
    }

    if (receiveCount < RX_QUEUE_SIZE) {
        receiveQueue[receiveTail] = pkt;
        receiveTail = (receiveTail + 1) % RX_QUEUE_SIZE;
        receiveCount++;
    } else {
        DBG_PRINTLN("RX queue full; packet dropped");
    }
}

void RadioManager::handleTxStatus(const uint8_t* mac_addr, esp_now_send_status_t status) {
    (void)mac_addr;
    pendingTxStatus = status;
    txStatusPending = true;
}

void RadioManager::onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int data_len) {
    radio.handleRxData(mac_addr, data, data_len);
}

void RadioManager::onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
    radio.handleTxStatus(mac_addr, status);
}
