#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>

class DisplayManager {
public:
    DisplayManager();
    void begin();
    void update();

    void setTxText(const String& text);
    void setRxText(const String& text);
    void setMode(const String& mode);
};

extern DisplayManager display;

#endif // DISPLAY_MANAGER_H
