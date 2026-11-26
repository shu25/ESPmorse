#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>

class CommandProcessor {
public:
    String execute(String command);
    String status() const;
    bool hasSendRequest() const;
    String getSendRequest();
private:
    String sendRequest;
};

extern CommandProcessor commands;

#endif
