#ifndef PACKET_H
#define PACKET_H

#include <Arduino.h>

enum PacketType : uint8_t {
    PKT_TEXT,
    PKT_DOT,
    PKT_DASH,
    PKT_LETTER,
    PKT_WORD,
    PKT_ACK,
    PKT_PING
};

struct __attribute__((packed)) Packet {
    uint16_t sequence;
    uint8_t  type;
    char     sender[8];
    char     target[8];
    uint8_t  data[32];
};

#endif // PACKET_H
