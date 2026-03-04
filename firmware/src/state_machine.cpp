#include "state_machine.h"

#include <M5Stack.h>
#include <Wire.h>
#include "display.h"
#include "servo.h"
#include "grbl.h"
#include "rfid_reader.h"
#include "wifi_manager.h"
#include "routing_api.h"

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

ConveyorState currentState = STATE_INIT;
String lastError = "";

String currentUID = "";
String currentStore = "";
int targetWarehouse  = 2;           // 1=A, 2=B, 3=C (B par defaut)
int targetServoAngle = DEFAULT_ANGLE; // angle servo (depuis API ou cache)

bool rfidOK = false;
bool grblOK = false;
bool servoOK = false;
bool wifiOK = false;

const char* stateNames[] = {
    "INIT", "READY", "DETECTING", "READING", "QUERYING", "ROUTING", "ERROR"
};

// ============================================================================
// MACHINE D'ETATS
// ============================================================================

void setState(ConveyorState newState) {
    Serial.printf("State: %s -> %s\n", stateNames[currentState], stateNames[newState]);
    currentState = newState;
    displayState();
}

void handleInit() {
    displayStatus("Initialisation...", BLUE);

    Wire.begin(21, 22);
    Wire.setClock(100000);
    delay(100);

    Serial.println("\n=== I2C Scan ===");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  Device @ 0x%02X\n", addr);
        }
    }

    rfidOK = initRFID();
    grblOK = initGRBL();
    servoOK = initServo();

    displayStatus("Connexion WiFi...", BLUE);
    wifiOK = connectWiFi();
    if (!wifiOK) Serial.println("WiFi KO - Mode degrade actif");

    displayState();

    if (rfidOK && grblOK && servoOK) {
        setState(STATE_READY);
        M5.Speaker.tone(1000, 100);
    } else {
        lastError = "Init peripheriques";
        setState(STATE_ERROR);
        M5.Speaker.tone(500, 500);
    }
}

void handleReady() {
    // Demarrer le tapis lentement si pas deja en marche
    if (!conveyorRunning) {
        conveyorStartSlow();
        displayStatus("PRET - Tapis en marche", GREEN);
    }

    // Scanner RFID pendant que le tapis tourne
    if (rfid.PICC_IsNewCardPresent()) {
        M5.Speaker.tone(800, 100);
        setState(STATE_READING);
        return;
    }

    // Bouton A = arret + avancer manuellement
    if (M5.BtnA.wasPressed()) {
        if (conveyorRunning) conveyorStop();
        displayStatus("Manuel: Avancer", YELLOW);
        conveyorForward(50);
        conveyorRunning = false;
    }

    // Bouton B = arret + reculer manuellement
    if (M5.BtnB.wasPressed()) {
        if (conveyorRunning) conveyorStop();
        displayStatus("Manuel: Reculer", YELLOW);
        conveyorBackward(50);
        conveyorRunning = false;
    }

    // Bouton C = test servo (sans arreter le tapis)
    if (M5.BtnC.wasPressed()) {
        static int testAngle = 0;
        testAngle = (testAngle + 5) % 31;
        displayStatus("Test Servo", CYAN);
        setServoAngle(SERVO_CH1, testAngle);
        M5.Speaker.tone(800 + testAngle * 10, 50);
    }
}

void handleDetecting() {
    // Cet etat n'est plus utilise dans le flux principal
    // Le tapis roule en continu et la detection se fait dans handleReady()
    setState(STATE_READING);
}

void handleReading() {
    displayStatus("Lecture RFID...", MAGENTA);

    unsigned long start = millis();
    currentUID = "";
    currentStore = "";
    int attempts = 0;

    while (millis() - start < RFID_SCAN_TIMEOUT) {
        attempts++;

        currentUID = readRFIDTag();
        if (currentUID.length() > 0) {
            Serial.println("UID lu: " + currentUID);
            M5.Speaker.tone(1200, 100);

            // Arreter le tapis avant l'appel API
            conveyorStop();
            displayStatus("Tag lu - Appel API...", CYAN);

            displayState();
            setState(STATE_QUERYING);
            return;
        }

        delay(250);
    }

    Serial.printf("RFID: Echec apres %d tentatives\n", attempts);
    lastError = "E030: Tag illisible";
    setState(STATE_ERROR);
}

void handleQuerying() {
    displayStatus("Routing API...", CYAN);

    RouteResult route = queryRoute(currentUID);

    targetWarehouse  = route.warehouseId;
    currentStore     = route.store;
    targetServoAngle = route.servoAngle;

    switch (route.status) {
        case QUERY_OK:
            Serial.printf("API OK: Entrepot %d (%s) angle=%d\n",
                          route.warehouseId, route.store.c_str(), route.servoAngle);
            break;

        case QUERY_CACHE_HIT:
            Serial.printf("MODE CACHE: Entrepot %d (%s) angle=%d\n",
                          route.warehouseId, route.store.c_str(), route.servoAngle);
            displayStatus("Mode cache (API KO)", ORANGE);
            delay(1500);
            M5.Speaker.tone(600, 200);
            break;

        case QUERY_UNKNOWN_UID:
            Serial.println("UID inconnu (API 404) -> fallback Entrepot B");
            displayStatus("UID inconnu -> Depot B", ORANGE);
            delay(1500);
            M5.Speaker.tone(600, 200);
            break;

        case QUERY_NO_API_NO_CACHE:
            Serial.println("API KO + cache vide -> fallback Entrepot B");
            displayStatus("API KO + cache vide -> B", RED);
            delay(1500);
            M5.Speaker.tone(400, 400);
            break;
    }

    setState(STATE_ROUTING);
}

void handleRouting() {
    displayStatus("Aiguillage...", YELLOW);

    // 1. Positionner le servo AVANT de redemarrer le tapis
    // L'angle vient directement de l'API ou du cache (plus besoin du switch local)
    Serial.printf("Entrepot %d (%s) -> Servo %d deg\n",
                  targetWarehouse, currentStore.c_str(), targetServoAngle);
    setServoAngle(SERVO_CH1, targetServoAngle);
    delay(SERVO_MOVE_DELAY);

    // 2. Redemarrer le tapis
    String label = "Entrepot " + currentStore;
    displayStatus(label.c_str(), GREEN);
    conveyorStartSlow();
    M5.Speaker.tone(1500, 200);

    // 3. Laisser le colis passer devant l'aiguillage
    delay(10000);

    // 4. Remettre le servo en position neutre
    Serial.println("Servo -> position neutre");
    setServoAngle(SERVO_CH1, DEFAULT_ANGLE);
    delay(SERVO_MOVE_DELAY);

    currentUID       = "";
    currentStore     = "";
    targetWarehouse  = 2;
    targetServoAngle = DEFAULT_ANGLE;

    setState(STATE_READY);
}

void handleError() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.println("=== ERREUR ===");

    M5.Lcd.setCursor(10, 50);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println(lastError);

    M5.Lcd.setCursor(10, 100);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.println("Btn A: Reset");
    M5.Lcd.println("Btn C: Reinit");

    if (M5.BtnA.wasPressed()) {
        lastError = "";
        setState(STATE_READY);
    }

    if (M5.BtnC.wasPressed()) {
        lastError = "";
        setState(STATE_INIT);
    }
}
