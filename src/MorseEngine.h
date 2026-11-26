#ifndef MORSE_ENGINE_H
#define MORSE_ENGINE_H

#include <Arduino.h>
#include "Config.h"

enum MorseEventType {
    EVT_DOT,
    EVT_DASH,
    EVT_LETTER,
    EVT_WORD
};

struct MorseEvent {
    MorseEventType type;
    char letter;
    String word;
};

class MorseEngine {
public:
    MorseEngine();
    void begin();
    void update();
    bool hasEvent();
    MorseEvent getEvent();
    bool hasWord();
    String getWord();
    const String& getBuffer() const;
    bool isKeyDown() const;

private:
    static const uint8_t EVENT_QUEUE_SIZE = 16;
    enum State {
        IDLE,
        KEY_DOWN,
        KEY_UP,
        WAIT_NEXT_INPUT,
        LETTER_DONE,
        WORD_DONE
    };

    State currentState;
    MorseEvent eventQueue[EVENT_QUEUE_SIZE];
    uint8_t eventHead;
    uint8_t eventTail;
    uint8_t eventCount;

    uint32_t lastDebounceTime;
    bool lastPinState;
    bool currentPinState;
    const uint32_t DEBOUNCE_DELAY = KEY_DEBOUNCE_MS;

    uint32_t keyDownTime;
    uint32_t keyUpTime;

    uint8_t currentPatternLen;
    uint8_t currentPatternCode;

    String wordBuffer;
    bool wordCompleted;
    String completedWord;

    void processKeyDown();
    void processKeyUp();
    void checkTimeouts();
    void pushEvent(MorseEventType type, char letter = '\0', String word = "");
    void appendToBuffer(char c);
};

extern MorseEngine morse;

#endif // MORSE_ENGINE_H
