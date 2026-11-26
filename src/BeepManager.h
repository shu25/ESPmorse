#ifndef BEEP_MANAGER_H
#define BEEP_MANAGER_H

#include <Arduino.h>

class BeepManager {
public:
    BeepManager();
    void begin();
    void update();

    void startSidetone();
    void stopSidetone();
    void setMuted(bool muted);
    void toggleMuted();
    bool isMuted() const;

    void playBootTone();
    void playReceiveTone();
    void playReceivedText(const String& text);
    void playSendTone();
    void playAckTone();
    void playRetryTone();
    void playErrorTone();
    void playDot();
    void playDash();

private:
    bool isSidetoneActive;
    bool muted;
    uint32_t playStartTime;
    uint32_t playDuration;
    bool isPlayingTone;

    enum PlaybackState {
        PLAY_IDLE,
        TX_TONE_LOW,
        TX_TONE_GAP,
        TX_TONE_HIGH,
        RX_TONE_HIGH,
        RX_TONE_GAP,
        RX_TONE_LOW,
        RX_BEFORE_MORSE,
        MORSE_TONE,
        MORSE_ELEMENT_GAP,
        MORSE_CHARACTER_GAP,
        MORSE_WORD_GAP
    };
    PlaybackState playbackState;
    uint32_t stateStartTime;
    uint32_t stateDuration;
    String playbackText;
    size_t playbackCharIndex;
    uint8_t playbackSymbolIndex;
    uint8_t playbackPatternLength;
    uint8_t playbackPatternCode;

    void toneOn(uint32_t freq);
    void toneOff();
    void setPlaybackState(PlaybackState state, uint32_t duration);
    void startNextMorseCharacter();
    void startCurrentMorseElement();
    void finishMorseElement();
    void stopPlayback();
};

extern BeepManager beep;

#endif // BEEP_MANAGER_H
