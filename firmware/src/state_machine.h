#pragma once

#include <Arduino.h>
#include "config.h"

// Variables globales de l'etat du systeme
extern ConveyorState currentState;
extern String lastError;
extern String currentUID;
extern String currentStore;
extern int targetWarehouse;
extern int targetServoAngle;
extern bool rfidOK;
extern bool grblOK;
extern bool servoOK;
extern bool wifiOK;

extern const char* stateNames[];

// Transition d'etat
void setState(ConveyorState newState);

// Gestionnaires de chaque etat
void handleInit();
void handleReady();
void handleDetecting();
void handleReading();
void handleQuerying();
void handleRouting();
void handleError();
