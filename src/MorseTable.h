#ifndef MORSE_TABLE_H
#define MORSE_TABLE_H

#include <Arduino.h>

// Morse pattern table.
// code uses DOT=0 and DASH=1, stored LSB first.
struct MorseNode {
    char letter;
    uint8_t len;
    uint8_t code;
};

const MorseNode morseTable[] = {
    {'A', 2, 0b00000010}, // .-
    {'B', 4, 0b00000001}, // -...
    {'C', 4, 0b00000101}, // -.-.
    {'D', 3, 0b00000001}, // -..
    {'E', 1, 0b00000000}, // .
    {'F', 4, 0b00000100}, // ..-.
    {'G', 3, 0b00000011}, // --.
    {'H', 4, 0b00000000}, // ....
    {'I', 2, 0b00000000}, // ..
    {'J', 4, 0b00001110}, // .---
    {'K', 3, 0b00000101}, // -.-
    {'L', 4, 0b00000010}, // .-..
    {'M', 2, 0b00000011}, // --
    {'N', 2, 0b00000001}, // -.
    {'O', 3, 0b00000111}, // ---
    {'P', 4, 0b00000110}, // .--.
    {'Q', 4, 0b00001011}, // --.-
    {'R', 3, 0b00000010}, // .-.
    {'S', 3, 0b00000000}, // ...
    {'T', 1, 0b00000001}, // -
    {'U', 3, 0b00000100}, // ..-
    {'V', 4, 0b00001000}, // ...-
    {'W', 3, 0b00000110}, // .--
    {'X', 4, 0b00001001}, // -..-
    {'Y', 4, 0b00001101}, // -.--
    {'Z', 4, 0b00000011}, // --..

    {'1', 5, 0b00011110}, // .----
    {'2', 5, 0b00011100}, // ..---
    {'3', 5, 0b00011000}, // ...--
    {'4', 5, 0b00010000}, // ....-
    {'5', 5, 0b00000000}, // .....
    {'6', 5, 0b00000001}, // -....
    {'7', 5, 0b00000011}, // --...
    {'8', 5, 0b00000111}, // ---..
    {'9', 5, 0b00001111}, // ----.
    {'0', 5, 0b00011111}, // -----

    {'.', 6, 0b00101010}, // .-.-.-
    {',', 6, 0b00110011}, // --..--
    {'?', 6, 0b00001100}, // ..--..
    {'\'', 6, 0b00011110}, // .----.
    {'-', 6, 0b00100001}, // -....-
    {'/', 5, 0b00001001}, // -..-.
    {'(', 5, 0b00001101}, // -.--.
    {')', 6, 0b00101101}, // -.--.-
    {':', 6, 0b00000111}, // ---...
    {'=', 5, 0b00010001}, // -...-
    {'+', 5, 0b00001010}, // .-.-.
    {'@', 6, 0b00010110}, // .--.-.
};

const int MORSE_TABLE_SIZE = sizeof(morseTable) / sizeof(morseTable[0]);

inline char decodeMorse(uint8_t len, uint8_t code) {
    if (len == 0) return '?';

    for (int i = 0; i < MORSE_TABLE_SIZE; i++) {
        if (morseTable[i].len == len && morseTable[i].code == code) {
            return morseTable[i].letter;
        }
    }

    return '?';
}

inline bool encodeMorse(char value, uint8_t& len, uint8_t& code) {
    for (int i = 0; i < MORSE_TABLE_SIZE; i++) {
        if (morseTable[i].letter == value) {
            len = morseTable[i].len;
            code = morseTable[i].code;
            return true;
        }
    }
    return false;
}

#endif // MORSE_TABLE_H
