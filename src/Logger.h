#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

struct LogEntry {
    uint32_t timestamp;
    bool isTx;
    String sender;
    String text;
};

class Logger {
public:
    Logger();
    void logTx(const String& text);
    void logRx(const String& sender, const String& text);
    void printHistory();
    String getRecentHistory(int limit = 5) const;
    int getHistoryCount() const;
    String getHistoryItem(int position) const;

private:
    static const int MAX_LOGS = 5;
    LogEntry logs[MAX_LOGS];
    int logIndex;
    int logCount;
};

extern Logger logger;

#endif // LOGGER_H
