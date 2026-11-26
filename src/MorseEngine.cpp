#include "MorseEngine.h"
#include "Config.h"
#include "MorseTable.h"
#include "RuntimeSettings.h"

MorseEngine morse;

MorseEngine::MorseEngine() {
    currentState = IDLE;
    eventHead = eventTail = eventCount = 0;
    lastDebounceTime = 0;
    lastPinState = HIGH;
    currentPinState = HIGH;
    keyDownTime = 0;
    keyUpTime = 0;
    currentPatternLen = 0;
    currentPatternCode = 0;
    wordCompleted = false;
}

void MorseEngine::begin() {
    pinMode(KEY_PIN, INPUT_PULLUP);
    lastPinState = digitalRead(KEY_PIN);
    currentPinState = lastPinState;
}

void MorseEngine::update() {
    bool reading = digitalRead(KEY_PIN);

    if (reading != lastPinState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        if (reading != currentPinState) {
            currentPinState = reading;

            if (currentPinState == LOW) {
                processKeyDown();
            } else {
                processKeyUp();
            }
        }
    }
    lastPinState = reading;

    checkTimeouts();
}

void MorseEngine::processKeyDown() {
    if (currentState == WAIT_NEXT_INPUT) {
        uint32_t idleTime = millis() - keyUpTime;
        if (currentPatternLen > 0 && idleTime >= settings.dotMs() * 3UL) {
            char c = decodeMorse(currentPatternLen, currentPatternCode);
            appendToBuffer(c);
            pushEvent(EVT_LETTER, c);
            currentPatternLen = 0;
            currentPatternCode = 0;
        }
        if (idleTime >= settings.dotMs() * 7UL && wordBuffer.length() > 0) {
            completedWord = wordBuffer;
            wordCompleted = true;
            pushEvent(EVT_WORD, '\0', wordBuffer);
            wordBuffer = "";
        }
    }
    keyDownTime = millis();
    currentState = KEY_DOWN;
}

void MorseEngine::processKeyUp() {
    if (currentState != KEY_DOWN) return;

    keyUpTime = millis();
    uint32_t duration = keyUpTime - keyDownTime;

    if (duration > 10) {
        if (currentPatternLen < 8) {
            if (duration < settings.dotMs() * DOT_DASH_THRESHOLD_UNITS) {
                currentPatternCode &= ~(1 << currentPatternLen);
                currentPatternLen++;
                pushEvent(EVT_DOT);
            } else {
                currentPatternCode |= (1 << currentPatternLen);
                currentPatternLen++;
                pushEvent(EVT_DASH);
            }
        }
    }

    currentState = WAIT_NEXT_INPUT;
}

void MorseEngine::checkTimeouts() {
    if (currentState == WAIT_NEXT_INPUT) {
        uint32_t idleTime = millis() - keyUpTime;

        if (idleTime >= settings.dotMs() * 7UL) {
            if (currentPatternLen > 0) {
                char c = decodeMorse(currentPatternLen, currentPatternCode);
                appendToBuffer(c);
                currentPatternLen = 0;
                currentPatternCode = 0;
            }
            if (wordBuffer.length() > 0) {
                completedWord = wordBuffer;
                wordCompleted = true;
                pushEvent(EVT_WORD, '\0', wordBuffer);
                wordBuffer = "";
            }
            currentState = IDLE;
        } else if (idleTime >= settings.dotMs() * 3UL) {
            if (currentPatternLen > 0) {
                char c = decodeMorse(currentPatternLen, currentPatternCode);
                appendToBuffer(c);
                pushEvent(EVT_LETTER, c);
                currentPatternLen = 0;
                currentPatternCode = 0;
            }
        }
    }
}

void MorseEngine::appendToBuffer(char c) {
    if (c != '?') {
        wordBuffer += c;
    }
}

void MorseEngine::pushEvent(MorseEventType type, char letter, String word) {
    if (eventCount >= EVENT_QUEUE_SIZE) return;
    eventQueue[eventTail] = {type, letter, word};
    eventTail = (eventTail + 1) % EVENT_QUEUE_SIZE;
    eventCount++;
}

bool MorseEngine::hasEvent() {
    return eventCount > 0;
}

MorseEvent MorseEngine::getEvent() {
    if (eventCount == 0) return {EVT_DOT, '\0', ""};
    MorseEvent event = eventQueue[eventHead];
    eventHead = (eventHead + 1) % EVENT_QUEUE_SIZE;
    eventCount--;
    return event;
}

bool MorseEngine::hasWord() {
    return wordCompleted;
}

String MorseEngine::getWord() {
    wordCompleted = false;
    return completedWord;
}

const String& MorseEngine::getBuffer() const {
    return wordBuffer;
}

bool MorseEngine::isKeyDown() const {
    return currentState == KEY_DOWN;
}
