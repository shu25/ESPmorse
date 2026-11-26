#include "BeepManager.h"
#include "Config.h"
#include "RuntimeSettings.h"
#include "MorseTable.h"

BeepManager beep;

BeepManager::BeepManager() {
    isSidetoneActive = false;
    muted = false;
    isPlayingTone = false;
    playStartTime = 0;
    playDuration = 0;
    playbackState = PLAY_IDLE;
    stateStartTime = 0;
    stateDuration = 0;
    playbackCharIndex = 0;
    playbackSymbolIndex = 0;
    playbackPatternLength = 0;
    playbackPatternCode = 0;
}

void BeepManager::begin() {
    muted = settings.get().muted;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(BUZZER_PIN, settings.get().sidetoneHz, BEEP_RESOLUTION);
    ledcWrite(BUZZER_PIN, 0);
#else
    ledcSetup(BEEP_CHANNEL, settings.get().sidetoneHz, BEEP_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, BEEP_CHANNEL);
    ledcWrite(BEEP_CHANNEL, 0);
#endif
}

void BeepManager::update() {
    if (muted != settings.get().muted) {
        muted = settings.get().muted;
        if (muted) toneOff();
        else if (isSidetoneActive) toneOn(settings.get().sidetoneHz);
    }
    if (playbackState != PLAY_IDLE && !isSidetoneActive && millis() - stateStartTime >= stateDuration) {
        switch (playbackState) {
            case TX_TONE_LOW:
                toneOff();
                setPlaybackState(TX_TONE_GAP, 55);
                break;
            case TX_TONE_GAP:
                toneOn(settings.get().txHighHz);
                setPlaybackState(TX_TONE_HIGH, 110);
                break;
            case TX_TONE_HIGH:
                stopPlayback();
                break;
            case RX_TONE_HIGH:
                toneOff();
                setPlaybackState(RX_TONE_GAP, 70);
                break;
            case RX_TONE_GAP:
                toneOn(settings.get().rxNoticeLowHz);
                setPlaybackState(RX_TONE_LOW, 110);
                break;
            case RX_TONE_LOW:
                toneOff();
                setPlaybackState(RX_BEFORE_MORSE, 180);
                break;
            case RX_BEFORE_MORSE:
                startNextMorseCharacter();
                break;
            case MORSE_TONE:
                finishMorseElement();
                break;
            case MORSE_ELEMENT_GAP:
                startCurrentMorseElement();
                break;
            case MORSE_CHARACTER_GAP:
            case MORSE_WORD_GAP:
                startNextMorseCharacter();
                break;
            default:
                stopPlayback();
                break;
        }
    }

    if (isPlayingTone && playbackState == PLAY_IDLE && !isSidetoneActive) {
        if (millis() - playStartTime >= playDuration) {
            toneOff();
            isPlayingTone = false;
        }
    }
}

void BeepManager::startSidetone() {
    if (isSidetoneActive) return;

    isSidetoneActive = true;
    isPlayingTone = false;
    playbackState = PLAY_IDLE;
    if (!muted) {
        toneOn(settings.get().sidetoneHz);
    }
}

void BeepManager::stopSidetone() {
    if (!isSidetoneActive) return;

    isSidetoneActive = false;
    if (!isPlayingTone) {
        toneOff();
    }
}

void BeepManager::setMuted(bool value) {
    muted = value;
    settings.setMuted(value);
    if (muted) {
        toneOff();
    } else if (isSidetoneActive) {
        toneOn(settings.get().sidetoneHz);
    }
}

void BeepManager::toggleMuted() {
    setMuted(!muted);
}

bool BeepManager::isMuted() const {
    return muted;
}

void BeepManager::playBootTone() {
    toneOn(880);
    delay(100);
    toneOn(1046);
    delay(100);
    toneOn(1318);
    delay(150);
    toneOff();
}

void BeepManager::playReceiveTone() {
    if (muted) return;
    toneOn(settings.get().rxToneHz + 400);
    playStartTime = millis();
    playDuration = 100;
    isPlayingTone = true;
}

void BeepManager::playSendTone() {
    if (muted) return;
    isPlayingTone = false;
    playbackText = "";
    toneOn(settings.get().txLowHz);
    setPlaybackState(TX_TONE_LOW, 90);
}

void BeepManager::playAckTone() {
    if (muted) return;
    stopPlayback();
    toneOn(settings.get().ackHz); playStartTime = millis(); playDuration = 90; isPlayingTone = true;
}

void BeepManager::playRetryTone() {
    if (muted) return;
    stopPlayback();
    toneOn(settings.get().retryHz); playStartTime = millis(); playDuration = 45; isPlayingTone = true;
}

void BeepManager::playErrorTone() {
    if (muted) return;
    stopPlayback();
    toneOn(settings.get().errorHz); playStartTime = millis(); playDuration = 350; isPlayingTone = true;
}

void BeepManager::playReceivedText(const String& text) {
    if (muted || text.length() == 0) return;
    isPlayingTone = false;
    playbackText = text;
    playbackText.toUpperCase();
    playbackCharIndex = 0;
    playbackSymbolIndex = 0;
    toneOn(settings.get().rxNoticeHighHz);
    setPlaybackState(RX_TONE_HIGH, 90);
}

void BeepManager::setPlaybackState(PlaybackState state, uint32_t duration) {
    playbackState = state;
    stateStartTime = millis();
    stateDuration = duration;
}

void BeepManager::startNextMorseCharacter() {
    while (playbackCharIndex < playbackText.length()) {
        char value = playbackText[playbackCharIndex++];
        if (value == ' ') {
            toneOff();
            while (playbackCharIndex < playbackText.length() && playbackText[playbackCharIndex] == ' ') playbackCharIndex++;
            setPlaybackState(MORSE_WORD_GAP, settings.rxDotMs() * 7UL);
            return;
        }
        if (encodeMorse(value, playbackPatternLength, playbackPatternCode)) {
            playbackSymbolIndex = 0;
            startCurrentMorseElement();
            return;
        }
    }
    stopPlayback();
}

void BeepManager::startCurrentMorseElement() {
    bool dash = (playbackPatternCode & (1U << playbackSymbolIndex)) != 0;
    toneOn(settings.get().rxToneHz);
    setPlaybackState(MORSE_TONE, settings.rxDotMs() * (dash ? 3UL : 1UL));
}

void BeepManager::finishMorseElement() {
    toneOff();
    playbackSymbolIndex++;
    if (playbackSymbolIndex < playbackPatternLength) {
        setPlaybackState(MORSE_ELEMENT_GAP, settings.rxDotMs());
        return;
    }

    if (playbackCharIndex < playbackText.length() && playbackText[playbackCharIndex] == ' ') {
        while (playbackCharIndex < playbackText.length() && playbackText[playbackCharIndex] == ' ') playbackCharIndex++;
        setPlaybackState(MORSE_WORD_GAP, settings.rxDotMs() * 7UL);
    } else if (playbackCharIndex < playbackText.length()) {
        setPlaybackState(MORSE_CHARACTER_GAP, settings.rxDotMs() * 3UL);
    } else {
        stopPlayback();
    }
}

void BeepManager::stopPlayback() {
    toneOff();
    playbackState = PLAY_IDLE;
    playbackText = "";
}

void BeepManager::playDot() {
    if (muted) return;
    toneOn(settings.get().rxToneHz);
    playStartTime = millis();
    playDuration = settings.rxDotMs();
    isPlayingTone = true;
}

void BeepManager::playDash() {
    if (muted) return;
    toneOn(settings.get().rxToneHz);
    playStartTime = millis();
    playDuration = settings.rxDotMs() * 3;
    isPlayingTone = true;
}

void BeepManager::toneOn(uint32_t freq) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWriteTone(BUZZER_PIN, freq);
    ledcWrite(BUZZER_PIN, BEEP_VOLUME);
#else
    ledcWriteTone(BEEP_CHANNEL, freq);
    ledcWrite(BEEP_CHANNEL, BEEP_VOLUME);
#endif
}

void BeepManager::toneOff() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(BUZZER_PIN, 0);
#else
    ledcWrite(BEEP_CHANNEL, 0);
#endif
}
