#include "CommandProcessor.h"
#include "RuntimeSettings.h"
#include "Logger.h"

CommandProcessor commands;

String CommandProcessor::status() const {
    const DeviceSettings& s = settings.get();
    String out = "OK CW_WPM=" + String(s.cwWpm);
    out += " CALLSIGN=" + String(s.callsign);
    out += " RX_WPM=" + String(s.rxWpm);
    out += " SIDETONE_HZ=" + String(s.sidetoneHz);
    out += " RX_TONE_HZ=" + String(s.rxToneHz);
    out += " TX_LOW_HZ=" + String(s.txLowHz);
    out += " TX_HIGH_HZ=" + String(s.txHighHz);
    out += " ACK_HZ=" + String(s.ackHz);
    out += " MUTED=" + String(s.muted ? 1 : 0);
    out += " TIMING=ITU_1_3_7";
    out += " BATTERY=" + String(s.batteryEnabled ? 1 : 0);
    out += " LOW_BATTERY_MV=" + String(s.lowBatteryMv);
    out += " BATTERY_DIV_X100=" + String(s.batteryDividerX100);
    out += " SLEEP_MIN=" + String(s.autoSleepMinutes);
    return out;
}

String CommandProcessor::execute(String command) {
    command.trim();
    command.toUpperCase();
    if (command == "GET" || command == "STATUS") return status();
    if (command == "HISTORY") return logger.getRecentHistory();
    if (command == "HISTORY_COUNT") return "OK HISTORY_COUNT " + String(logger.getHistoryCount());
    if (command.startsWith("HISTORY_ITEM=")) {
        return logger.getHistoryItem(command.substring(13).toInt());
    }
    if (command == "RESET") { settings.resetDefaults(); return status(); }

    if (command.startsWith("SEND=") || command.startsWith("SEND ")) {
        String text = command.substring(5);
        text.trim();
        if (text.length() == 0 || text.length() >= 32) return "ERR TEXT_LENGTH_1_31";
        if (sendRequest.length() > 0) return "ERR SEND_BUSY";
        sendRequest = text;
        return "OK SEND_QUEUED";
    }

    int separator = command.indexOf('=');
    if (separator < 0) separator = command.indexOf(' ');
    if (separator < 1) return "ERR FORMAT";

    String key = command.substring(0, separator);
    String rawValue = command.substring(separator + 1);
    rawValue.trim();
    if (rawValue.length() == 0) return "ERR VALUE";
    int value = rawValue.toInt();
    bool ok = false;

    if (key == "CW_WPM") ok = settings.setCwWpm(value);
    else if (key == "RX_WPM") ok = settings.setRxWpm(value);
    else if (key == "SIDETONE_HZ") ok = settings.setSidetoneHz(value);
    else if (key == "RX_TONE_HZ") ok = settings.setRxToneHz(value);
    else if (key == "TX_LOW_HZ" || key == "TX_HIGH_HZ" || key == "RX_NOTICE_HIGH_HZ" ||
             key == "RX_NOTICE_LOW_HZ" || key == "ACK_HZ" || key == "RETRY_HZ" || key == "ERROR_HZ")
        ok = settings.setAlertTone(key, value);
    else if (key == "MUTED" && (value == 0 || value == 1)) { settings.setMuted(value == 1); ok = true; }
    else if (key == "CALLSIGN") ok = settings.setCallsign(rawValue);
    else if (key == "BATTERY" && (value == 0 || value == 1)) ok = settings.setBatteryEnabled(value == 1);
    else if (key == "LOW_BATTERY_MV") ok = settings.setLowBatteryMv(value);
    else if (key == "BATTERY_DIV_X100") ok = settings.setBatteryDividerX100(value);
    else if (key == "SLEEP_MIN") ok = settings.setAutoSleepMinutes(value);
    else if (key == "BLE_PASSKEY") ok = settings.setBlePasskey(value);
    else if (key != "MUTED") return "ERR UNKNOWN";

    return ok ? status() : "ERR RANGE";
}

bool CommandProcessor::hasSendRequest() const { return sendRequest.length() > 0; }
String CommandProcessor::getSendRequest() { String value = sendRequest; sendRequest = ""; return value; }
