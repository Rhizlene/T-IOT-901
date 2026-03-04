/*
 * The Conveyor - Firmware Principal
 * T-IOT-901 EPITECH Marseille
 * Nafissatou SYLLA & Rhizlene ALFARDOUS
 *
 * Composants:
 *   - M5Stack Core (ESP32)
 *   - RFID Unit 2 (MFRC522 I2C @ 0x28)
 *   - GoPlus2 Module (Servo @ 0x38)
 *   - GRBL Module 13.2 (Moteur @ 0x70)
 *
 * Machine d'etats: INIT -> READY -> DETECTING -> READING -> QUERYING -> ROUTING -> READY
 */

#include <M5Stack.h>
#include "state_machine.h"

void setup() {
    M5.begin();
    M5.Power.begin();
    Serial.begin(115200);

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.println("=== THE CONVEYOR ===");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("T-IOT-901 EPITECH");
    M5.Lcd.println("");
    M5.Lcd.println("Demarrage...");

    Serial.println("\n========================================");
    Serial.println("  THE CONVEYOR - Firmware (RFID Routing API)");
    Serial.println("  T-IOT-901 EPITECH Marseille");
    Serial.println("========================================\n");

    delay(1000);
    setState(STATE_INIT);
}

void loop() {
    M5.update();

    switch (currentState) {
        case STATE_INIT:      handleInit();      break;
        case STATE_READY:     handleReady();     break;
        case STATE_DETECTING: handleDetecting(); break;
        case STATE_READING:   handleReading();   break;
        case STATE_QUERYING:  handleQuerying();  break;
        case STATE_ROUTING:   handleRouting();   break;
        case STATE_ERROR:     handleError();     break;
    }

    delay(50);
}
