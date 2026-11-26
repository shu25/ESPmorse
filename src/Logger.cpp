#include "Logger.h"
#include "Config.h"

Logger logger;

Logger::Logger() {
    logIndex = 0;
    logCount = 0;
}

void Logger::logTx(const String& text) {
    logs[logIndex].timestamp = millis();
    logs[logIndex].isTx = true;
    logs[logIndex].sender = "ME";
    logs[logIndex].text = text;
    
    logIndex = (logIndex + 1) % MAX_LOGS;
    if (logCount < MAX_LOGS) logCount++;
}

void Logger::logRx(const String& sender, const String& text) {
    logs[logIndex].timestamp = millis();
    logs[logIndex].isTx = false;
    logs[logIndex].sender = sender;
    logs[logIndex].text = text;
    
    logIndex = (logIndex + 1) % MAX_LOGS;
    if (logCount < MAX_LOGS) logCount++;
}

void Logger::printHistory() {
    DBG_PRINTLN("\n--- Communication History ---");
    for (int i = 0; i < MAX_LOGS; i++) {
        int idx = (logIndex + i) % MAX_LOGS;
        if (logs[idx].text.length() > 0) {
            DBG_PRINTF("[%lu] %s: %s\n",
                logs[idx].timestamp,
                logs[idx].isTx ? "TX" : ("RX (" + logs[idx].sender + ")").c_str(),
                logs[idx].text.c_str());
        }
    }
    DBG_PRINTLN("---------------------------");
}

String Logger::getRecentHistory(int limit) const {
    if (limit < 1) limit = 1;
    if (limit > MAX_LOGS) limit = MAX_LOGS;
    String result = "OK HISTORY";
    int added = 0;
    for (int offset = 1; offset <= MAX_LOGS && added < limit; offset++) {
        int idx = (logIndex - offset + MAX_LOGS) % MAX_LOGS;
        if (logs[idx].text.length() == 0) continue;
        result += "\n";
        result += logs[idx].isTx ? "TX " : "RX ";
        if (!logs[idx].isTx) { result += logs[idx].sender; result += " "; }
        result += logs[idx].text;
        added++;
    }
    return result;
}

int Logger::getHistoryCount() const {
    return logCount;
}

String Logger::getHistoryItem(int position) const {
    if (position < 0 || position >= logCount) return "ERR HISTORY_RANGE";
    int oldest = (logIndex - logCount + MAX_LOGS) % MAX_LOGS;
    int idx = (oldest + position) % MAX_LOGS;
    const LogEntry& entry = logs[idx];
    String result = "OK HISTORY_ITEM ";
    result += String(position);
    result += " ";
    result += String(entry.timestamp);
    result += entry.isTx ? " TX ME " : " RX ";
    if (!entry.isTx) { result += entry.sender; result += " "; }
    result += entry.text;
    return result;
}
