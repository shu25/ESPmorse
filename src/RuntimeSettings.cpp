#include "RuntimeSettings.h"
#include "Config.h"
#include <Preferences.h>

RuntimeSettings settings;

void RuntimeSettings::loadDefaults() {
    strncpy(values.callsign, CALLSIGN, sizeof(values.callsign));
    values.callsign[sizeof(values.callsign) - 1] = '\0';
    values.cwWpm = DEFAULT_CW_SPEED_WPM;
    values.rxWpm = DEFAULT_RX_CW_SPEED_WPM;
    values.sidetoneHz = DEFAULT_SIDETONE_FREQ;
    values.rxToneHz = DEFAULT_RX_TONE_FREQ;
    values.txLowHz = 750; values.txHighHz = 1250;
    values.rxNoticeHighHz = 1250; values.rxNoticeLowHz = 850;
    values.ackHz = 1500; values.retryHz = 500; values.errorHz = 320;
    values.muted = false;
    values.batteryEnabled = false;
    values.lowBatteryMv = DEFAULT_LOW_BATTERY_MV;
    values.batteryDividerX100 = (uint16_t)(DEFAULT_BATTERY_DIVIDER * 100);
    values.autoSleepMinutes = 0;
    values.blePasskey = 123456;
}

void RuntimeSettings::begin() {
    loadDefaults();
    Preferences prefs;
    // Open read/write so the namespace is created cleanly on first boot.
    if (!prefs.begin("morse", false)) return;
    values.cwWpm = prefs.getUChar("cwWpm", values.cwWpm);
    values.rxWpm = prefs.getUChar("rxWpm", values.rxWpm);
    values.sidetoneHz = prefs.getUShort("sideHz", values.sidetoneHz);
    values.rxToneHz = prefs.getUShort("rxHz", values.rxToneHz);
    values.txLowHz = prefs.getUShort("txLo", values.txLowHz);
    values.txHighHz = prefs.getUShort("txHi", values.txHighHz);
    values.rxNoticeHighHz = prefs.getUShort("rxNHi", values.rxNoticeHighHz);
    values.rxNoticeLowHz = prefs.getUShort("rxNLo", values.rxNoticeLowHz);
    values.ackHz = prefs.getUShort("ackHz", values.ackHz);
    values.retryHz = prefs.getUShort("retryHz", values.retryHz);
    values.errorHz = prefs.getUShort("errHz", values.errorHz);
    values.muted = prefs.getBool("muted", values.muted);
    if (prefs.isKey("callsign")) prefs.getString("callsign", values.callsign, sizeof(values.callsign));
    values.batteryEnabled = prefs.getBool("batEn", values.batteryEnabled);
    values.lowBatteryMv = prefs.getUShort("batLow", values.lowBatteryMv);
    values.batteryDividerX100 = prefs.getUShort("batDiv", values.batteryDividerX100);
    values.autoSleepMinutes = prefs.getUShort("sleepMin", values.autoSleepMinutes);
    values.blePasskey = prefs.getUInt("blePin", values.blePasskey);
    prefs.end();

    if (values.cwWpm < MIN_CW_SPEED_WPM || values.cwWpm > MAX_CW_SPEED_WPM) values.cwWpm = DEFAULT_CW_SPEED_WPM;
    if (values.rxWpm < MIN_CW_SPEED_WPM || values.rxWpm > MAX_CW_SPEED_WPM) values.rxWpm = DEFAULT_RX_CW_SPEED_WPM;
    if (values.sidetoneHz < MIN_TONE_FREQ || values.sidetoneHz > MAX_TONE_FREQ) values.sidetoneHz = DEFAULT_SIDETONE_FREQ;
    if (values.rxToneHz < MIN_TONE_FREQ || values.rxToneHz > MAX_TONE_FREQ) values.rxToneHz = DEFAULT_RX_TONE_FREQ;
}

const DeviceSettings& RuntimeSettings::get() const { return values; }

void RuntimeSettings::save() {
    Preferences prefs;
    if (!prefs.begin("morse", false)) return;
    prefs.putUChar("cwWpm", values.cwWpm);
    prefs.putUChar("rxWpm", values.rxWpm);
    prefs.putUShort("sideHz", values.sidetoneHz);
    prefs.putUShort("rxHz", values.rxToneHz);
    prefs.putUShort("txLo", values.txLowHz); prefs.putUShort("txHi", values.txHighHz);
    prefs.putUShort("rxNHi", values.rxNoticeHighHz); prefs.putUShort("rxNLo", values.rxNoticeLowHz);
    prefs.putUShort("ackHz", values.ackHz); prefs.putUShort("retryHz", values.retryHz); prefs.putUShort("errHz", values.errorHz);
    prefs.putBool("muted", values.muted);
    prefs.putString("callsign", values.callsign);
    prefs.putBool("batEn", values.batteryEnabled);
    prefs.putUShort("batLow", values.lowBatteryMv);
    prefs.putUShort("batDiv", values.batteryDividerX100);
    prefs.putUShort("sleepMin", values.autoSleepMinutes);
    prefs.putUInt("blePin", values.blePasskey);
    prefs.end();
}

bool RuntimeSettings::setCwWpm(int value) {
    if (value < MIN_CW_SPEED_WPM || value > MAX_CW_SPEED_WPM) return false;
    values.cwWpm = value; save(); return true;
}
bool RuntimeSettings::setRxWpm(int value) {
    if (value < MIN_CW_SPEED_WPM || value > MAX_CW_SPEED_WPM) return false;
    values.rxWpm = value; save(); return true;
}
bool RuntimeSettings::setSidetoneHz(int value) {
    if (value < MIN_TONE_FREQ || value > MAX_TONE_FREQ) return false;
    values.sidetoneHz = value; save(); return true;
}
bool RuntimeSettings::setRxToneHz(int value) {
    if (value < MIN_TONE_FREQ || value > MAX_TONE_FREQ) return false;
    values.rxToneHz = value; save(); return true;
}
bool RuntimeSettings::setAlertTone(const String& key, int value) {
    if (value < MIN_TONE_FREQ || value > 2000) return false;
    if (key == "TX_LOW_HZ") values.txLowHz = value;
    else if (key == "TX_HIGH_HZ") values.txHighHz = value;
    else if (key == "RX_NOTICE_HIGH_HZ") values.rxNoticeHighHz = value;
    else if (key == "RX_NOTICE_LOW_HZ") values.rxNoticeLowHz = value;
    else if (key == "ACK_HZ") values.ackHz = value;
    else if (key == "RETRY_HZ") values.retryHz = value;
    else if (key == "ERROR_HZ") values.errorHz = value;
    else return false;
    save(); return true;
}
bool RuntimeSettings::setCallsign(const String& value) {
    if (value.length() < 1 || value.length() > 7) return false;
    for (size_t i = 0; i < value.length(); i++) {
        if (!isAlphaNumeric(value[i]) && value[i] != '-') return false;
    }
    value.toCharArray(values.callsign, sizeof(values.callsign)); save(); return true;
}
bool RuntimeSettings::setBatteryEnabled(bool value) { values.batteryEnabled = value; save(); return true; }
bool RuntimeSettings::setLowBatteryMv(int value) {
    if (value < 2500 || value > 4500) return false;
    values.lowBatteryMv = value; save(); return true;
}
bool RuntimeSettings::setBatteryDividerX100(int value) {
    if (value < 100 || value > 500) return false;
    values.batteryDividerX100 = value; save(); return true;
}
bool RuntimeSettings::setAutoSleepMinutes(int value) {
    if (value < 0 || value > 1440) return false;
    values.autoSleepMinutes = value; save(); return true;
}
bool RuntimeSettings::setBlePasskey(int value) {
    if (value < 100000 || value > 999999) return false;
    values.blePasskey = value; save(); return true;
}
void RuntimeSettings::setMuted(bool value) { values.muted = value; save(); }
void RuntimeSettings::resetDefaults() { loadDefaults(); save(); }
uint32_t RuntimeSettings::dotMs() const { return 1200UL / values.cwWpm; }
uint32_t RuntimeSettings::rxDotMs() const { return 1200UL / values.rxWpm; }
