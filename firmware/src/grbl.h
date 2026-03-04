#pragma once

#include <Wire.h>
#include "config.h"

extern bool conveyorRunning;

bool initGRBL();
void sendGcode(const char* cmd);
void sendRealtimeCmd(char cmd);
void conveyorStartSlow();
void conveyorStop();
void conveyorForward(int distance_mm);
void conveyorBackward(int distance_mm);
