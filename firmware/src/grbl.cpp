#include "grbl.h"
#include <Arduino.h>

bool conveyorRunning = false;

void sendGcode(const char* cmd) {
    Serial.print("GRBL TX: ");
    Serial.println(cmd);

    Wire.beginTransmission(GRBL_I2C_ADDR);
    for (size_t i = 0; i < strlen(cmd); i++) Wire.write(cmd[i]);
    Wire.write('\r');
    Wire.write('\n');
    Wire.endTransmission();

    delay(100);

    Wire.requestFrom((uint8_t)GRBL_I2C_ADDR, (uint8_t)32);
    String response = "";
    while (Wire.available()) {
        char c = Wire.read();
        if (c >= 32 && c < 127) response += c;
    }
    if (response.length() > 0) {
        Serial.print("GRBL RX: ");
        Serial.println(response);
    }
}

bool initGRBL() {
    Wire.beginTransmission(GRBL_I2C_ADDR);
    if (Wire.endTransmission() == 0) {
        delay(500);
        sendGcode("$X");        // Unlock
        delay(200);
        sendGcode("$102=80");   // Z steps/mm
        delay(100);
        sendGcode("$112=500");  // Z max rate mm/min
        delay(100);
        sendGcode("$122=50");   // Z acceleration mm/sec^2 (evite les vibrations)
        delay(100);
        sendGcode("G21");       // Millimeters
        delay(100);
        sendGcode("G90");       // Absolute
        delay(100);
        sendGcode("G92 Z0");    // Set zero
        Serial.println("GRBL Module OK @ 0x70");
        return true;
    }
    Serial.println("GRBL Module NOT FOUND!");
    return false;
}

// Commande temps-reel GRBL (sans \r\n)
void sendRealtimeCmd(char cmd) {
    Wire.beginTransmission(GRBL_I2C_ADDR);
    Wire.write(cmd);
    Wire.endTransmission();
    delay(150);
}

// Demarrer le tapis en mode continu lent (non bloquant)
void conveyorStartSlow() {
    char cmd[32];
    sendGcode("G91");
    sprintf(cmd, "G1 Z5000 F%d", CONVEYOR_SLOW_SPEED);
    sendGcode(cmd);
    sendGcode("G90");
    conveyorRunning = true;
    Serial.println("Tapis: DEMARRAGE LENT");
}

// Arret tapis - soft reset GRBL (vide le buffer et annule tout mouvement)
void conveyorStop() {
    sendRealtimeCmd(0x18);  // Ctrl+X : soft reset GRBL
    delay(800);             // Attendre que GRBL redémarre
    sendGcode("$X");        // Unlock après reset
    delay(100);
    sendGcode("$122=50");   // Reconfirmer acceleration (anti-vibrations)
    delay(100);
    sendGcode("G21");       // Remettre en mode mm
    sendGcode("G90");       // Remettre en mode absolu
    conveyorRunning = false;
    Serial.println("Tapis: STOP (Soft Reset)");
}

void conveyorForward(int distance_mm) {
    char cmd[32];
    sendGcode("G91");
    sprintf(cmd, "G1 Z%d F%d", distance_mm, CONVEYOR_EJECT_SPEED);
    sendGcode(cmd);
    sendGcode("G90");
    Serial.printf("conveyorForward: %dmm (non-bloquant)\n", distance_mm);
}

void conveyorBackward(int distance_mm) {
    char cmd[32];
    sendGcode("G91");
    sprintf(cmd, "G1 Z-%d F%d", distance_mm, CONVEYOR_EJECT_SPEED);
    sendGcode(cmd);
    sendGcode("G90");
    int moveTime = (int)((float)distance_mm / CONVEYOR_EJECT_SPEED * 60000.0f * 1.2f);
    if (moveTime < 500) moveTime = 500;
    Serial.printf("conveyorBackward: %dmm, attente %dms\n", distance_mm, moveTime);
    delay(moveTime);
}
