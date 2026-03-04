#include "display.h"

// Variables globales partagees (definies dans state_machine.cpp)
extern ConveyorState currentState;
extern String currentUID;
extern String currentStore;
extern bool wifiOK;
extern bool rfidOK;
extern bool grblOK;
extern bool servoOK;

extern const char* stateNames[];

void displayStatus(const char* status, uint16_t color) {
    M5.Lcd.fillRect(0, 200, 320, 40, BLACK);
    M5.Lcd.setCursor(10, 210);
    M5.Lcd.setTextColor(color);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println(status);
}

void displayState() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);

    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.println("=== THE CONVEYOR ===");

    M5.Lcd.setCursor(10, 40);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.print("Etat: ");

    uint16_t stateColor = WHITE;
    switch (currentState) {
        case STATE_INIT:      stateColor = BLUE;    break;
        case STATE_READY:     stateColor = GREEN;   break;
        case STATE_DETECTING: stateColor = ORANGE;  break;
        case STATE_READING:   stateColor = MAGENTA; break;
        case STATE_QUERYING:  stateColor = CYAN;    break;
        case STATE_ROUTING:   stateColor = YELLOW;  break;
        case STATE_ERROR:     stateColor = RED;     break;
    }
    M5.Lcd.setTextColor(stateColor);
    M5.Lcd.println(stateNames[currentState]);

    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(10, 70);
    M5.Lcd.print("WiFi: ");
    M5.Lcd.setTextColor(wifiOK ? GREEN : RED);
    M5.Lcd.println(wifiOK ? "OK" : "KO");

    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(10, 90);
    M5.Lcd.print("RFID: ");
    M5.Lcd.setTextColor(rfidOK ? GREEN : RED);
    M5.Lcd.println(rfidOK ? "OK" : "KO");

    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(10, 110);
    M5.Lcd.print("GRBL: ");
    M5.Lcd.setTextColor(grblOK ? GREEN : RED);
    M5.Lcd.println(grblOK ? "OK" : "KO");

    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(10, 130);
    M5.Lcd.print("Servo: ");
    M5.Lcd.setTextColor(servoOK ? GREEN : RED);
    M5.Lcd.println(servoOK ? "OK" : "KO");

    if (currentUID.length() > 0) {
        M5.Lcd.setCursor(10, 160);
        M5.Lcd.setTextColor(YELLOW);
        M5.Lcd.print("UID: ");
        M5.Lcd.println(currentUID);
    }

    if (currentStore.length() > 0) {
        M5.Lcd.setCursor(10, 180);
        M5.Lcd.setTextColor(YELLOW);
        M5.Lcd.print("Magasin: ");
        M5.Lcd.println(currentStore);
    }
}
