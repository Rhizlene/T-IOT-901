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
 *
 * Logique:
 *   - Lecture UID RFID
 *   - Appel API Routing: GET /api/routing/by-rfid/{UID}
 *   - Reponse: { warehouseId: 1|2|3, store: A|B|C, servoAngle: 5|15|25 }
 *   - Aiguillage + ejection
 */

#include <M5Stack.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "MFRC522_I2C.h"

// ============================================================================
// CONFIGURATION
// ============================================================================

// WiFi
const char* WIFI_SSID     = "Finv";
const char* WIFI_PASSWORD = "17082020";

// API Routing RFID (Node/Express)
const char* ROUTING_API_HOST = "172.22.27.122";
const int   ROUTING_API_PORT = 3000;

// Adresses I2C
#define RFID_I2C_ADDR   0x28
#define GOPLUS2_ADDR    0x38
#define GRBL_I2C_ADDR   0x70

// Servo (sur GoPlus2)
#define SERVO_CH1       0

// Mapping entrepots -> angles servo (magasins A/B/C)
#define WAREHOUSE_A_ANGLE   5
#define WAREHOUSE_B_ANGLE   15
#define WAREHOUSE_C_ANGLE   25
#define DEFAULT_ANGLE       15

// Timeouts (ms)
#define WIFI_TIMEOUT        20000
#define RFID_SCAN_TIMEOUT   5000
#define API_TIMEOUT         5000
#define MOTOR_MOVE_TIME     2000

// Vitesses moteur (mm/min)
#define CONVEYOR_SLOW_SPEED   80    // Vitesse lente continue (tapis en marche)
#define CONVEYOR_EJECT_SPEED  3000  // Vitesse d'ejection du colis

// ============================================================================
// ETATS DE LA MACHINE
// ============================================================================

enum ConveyorState {
    STATE_INIT,
    STATE_READY,
    STATE_DETECTING,
    STATE_READING,
    STATE_QUERYING,
    STATE_ROUTING,
    STATE_ERROR
};

const char* stateNames[] = {
    "INIT", "READY", "DETECTING", "READING", "QUERYING", "ROUTING", "ERROR"
};

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

ConveyorState currentState = STATE_INIT;
String lastError = "";

String currentUID = "";      // UID lu (format "04:82:..." selon MFRC522)
String currentStore = "";    // "A" / "B" / "C"
int targetWarehouse = 2;     // 1=A, 2=B, 3=C (B par defaut)

MFRC522 rfid(RFID_I2C_ADDR);
bool rfidOK = false;
bool grblOK = false;
bool servoOK = false;
bool wifiOK = false;
bool conveyorRunning = false;

// ============================================================================
// FONCTIONS AFFICHAGE
// ============================================================================

void displayStatus(const char* status, uint16_t color = WHITE) {
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

// ============================================================================
// FONCTIONS SERVO (via GoPlus2)
// ============================================================================

void setServoAngle(uint8_t channel, uint8_t angle) {
    Wire.beginTransmission(GOPLUS2_ADDR);
    Wire.write(channel);
    Wire.write(angle);
    Wire.endTransmission();
    Serial.printf("Servo CH%d -> %d deg\n", channel, angle);
}

bool initServo() {
    Wire.beginTransmission(GOPLUS2_ADDR);
    if (Wire.endTransmission() == 0) {
        setServoAngle(SERVO_CH1, DEFAULT_ANGLE);
        Serial.println("GoPlus2 (Servo) OK @ 0x38");
        return true;
    }
    Serial.println("GoPlus2 (Servo) NOT FOUND!");
    return false;
}

// ============================================================================
// FONCTIONS GRBL (Moteur convoyeur)
// ============================================================================

void sendGcode(const char* cmd) {
    Serial.print("GRBL TX: ");
    Serial.println(cmd);

    Wire.beginTransmission(GRBL_I2C_ADDR);
    for (size_t i = 0; i < strlen(cmd); i++) Wire.write(cmd[i]);
    Wire.write('\r');
    Wire.write('\n');
    Wire.endTransmission();

    delay(100);

    Wire.requestFrom(GRBL_I2C_ADDR, (uint8_t)32);
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
        sendGcode("$X");           // Unlock
        delay(200);
        sendGcode("$102=80");      // Z steps/mm
        delay(100);
        sendGcode("$112=500");     // Z max rate
        delay(100);
        sendGcode("G21");          // Millimeters
        delay(100);
        sendGcode("G90");          // Absolute
        delay(100);
        sendGcode("G92 Z0");       // Set zero
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
// Appeler uniquement quand GRBL est en etat IDLE (apres conveyorStop ou init)
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
    sendGcode("G21");       // Remettre en mode mm
    sendGcode("G90");       // Remettre en mode absolu
    conveyorRunning = false;
    Serial.println("Tapis: STOP (Soft Reset)");
}

void conveyorForward(int distance_mm) {
    sendGcode("G91");
    char cmd[32];
    sendGcode("G91");
    sprintf(cmd, "G1 Z%d F%d", distance_mm, CONVEYOR_EJECT_SPEED);
    sendGcode(cmd);
    sendGcode("G90");
    // Delai calcule : (distance / vitesse) * 60s * 1000ms + 20% de marge
    int moveTime = (int)((float)distance_mm / CONVEYOR_EJECT_SPEED * 60000.0f * 1.2f);
    if (moveTime < 500) moveTime = 500;
    Serial.printf("conveyorForward: %dmm, attente %dms\n", distance_mm, moveTime);
    delay(moveTime);
}

void conveyorBackward(int distance_mm) {
    sendGcode("G91");
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

// ============================================================================
// FONCTIONS RFID
// ============================================================================

bool initRFID() {
    rfid.PCD_Init();
    delay(100);

    byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
    Serial.print("MFRC522 Version: 0x");
    Serial.println(version, HEX);

    if (version == 0x91 || version == 0x92 || version == 0x88 || version == 0x15) {
        rfid.PCD_SetAntennaGain(rfid.RxGain_max);
        Serial.println("RFID Unit 2 OK @ 0x28");
        return true;
    }
    Serial.println("RFID Unit 2 NOT FOUND!");
    return false;
}

// UID au format "04:82:..."
String readRFIDTag() {
    if (!rfid.PICC_IsNewCardPresent()) return "";
    if (!rfid.PICC_ReadCardSerial()) return "";

    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] < 0x10) uid += "0";
        uid += String(rfid.uid.uidByte[i], HEX);
        if (i < rfid.uid.size - 1) uid += ":";
    }
    uid.toUpperCase();

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    return uid;
}

// ============================================================================
// WIFI
// ============================================================================

bool connectWiFi() {
    Serial.println("Connexion WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("WiFi OK - IP: ");
        Serial.println(WiFi.localIP());
        return true;
    }

    Serial.println("\nWiFi FAILED");
    return false;
}

// ============================================================================
// ROUTING API (RFID -> warehouse)
// ============================================================================

String normalizeUID(String uid) {
    uid.trim();
    uid.toUpperCase();
    uid.replace(":", "");
    uid.replace(" ", "");
    return uid;
}

int queryWarehouseByUID(const String& uidRaw) {
    if (WiFi.status() != WL_CONNECTED) return -1;

    String uid = normalizeUID(uidRaw);

    HTTPClient http;
    String url = "http://";
    url += ROUTING_API_HOST;
    url += ":";
    url += String(ROUTING_API_PORT);
    url += "/api/routing/by-rfid/";
    url += uid;

    Serial.print("ROUTING API GET: ");
    Serial.println(url);

    http.begin(url);
    http.setTimeout(API_TIMEOUT);
    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        Serial.println("Routing API Response: " + payload);

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            int warehouseId = doc["warehouseId"] | -1;
            const char* store = doc["store"] | "";

            currentStore = String(store);

            http.end();
            return warehouseId;
        } else {
            Serial.print("JSON parse error: ");
            Serial.println(error.c_str());
        }
    } else {
        Serial.printf("Routing API Error: %d\n", httpCode);
    }

    http.end();
    return -1;
}

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
        conveyorStop();
        M5.Speaker.tone(800, 100);
        setState(STATE_READING);
        return;
    }

    // Bouton A = arret + avancer manuellement
    if (M5.BtnA.wasPressed()) {
        if (conveyorRunning) conveyorStop();
        displayStatus("Manuel: Avancer", YELLOW);
        conveyorForward(50);
        conveyorRunning = false;  // Sera redemarré au prochain cycle
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
        testAngle = (testAngle + 5) % 31; // petit test fin
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

    targetWarehouse = queryWarehouseByUID(currentUID);

    if (targetWarehouse > 0) {
        Serial.printf("UID -> Entrepot %d\n", targetWarehouse);
        setState(STATE_ROUTING);
    } else {
        Serial.println("Routing API KO - Mode degrade (B)");
        targetWarehouse = 2; // B (centre)
        currentStore = "B";
        setState(STATE_ROUTING);
    }
}

void handleRouting() {
    displayStatus("Aiguillage...", YELLOW);

    int angle = DEFAULT_ANGLE;
    switch (targetWarehouse) {
        case 1: angle = WAREHOUSE_A_ANGLE; break;
        case 2: angle = WAREHOUSE_B_ANGLE; break;
        case 3: angle = WAREHOUSE_C_ANGLE; break;
        default: angle = DEFAULT_ANGLE; break;
    }

    Serial.printf("Entrepot %d -> Servo %d deg\n", targetWarehouse, angle);
    setServoAngle(SERVO_CH1, angle);
    delay(500);  // Laisser le servo atteindre la position

    // Ejecter le colis a vitesse normale
    displayStatus("Ejection...", GREEN);
    conveyorForward(300);

    M5.Speaker.tone(1500, 200);

    // Retour servo en position neutre apres ejection
    setServoAngle(SERVO_CH1, DEFAULT_ANGLE);
    delay(300);

    currentUID = "";
    currentStore = "";
    targetWarehouse = 2;

    // Le tapis sera redemarré lentement dans handleReady()
    conveyorRunning = false;

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

// ============================================================================
// SETUP & LOOP
// ============================================================================

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
    Serial.println("  THE CONVEYOR - Firmware CLEAN (RFID Routing API)");
    Serial.println("  T-IOT-901 EPITECH Marseille");
    Serial.println("========================================\n");

    delay(1000);
    setState(STATE_INIT);
}

void loop() {
    M5.update();

    switch (currentState) {
        case STATE_INIT:      handleInit(); break;
        case STATE_READY:     handleReady(); break;
        case STATE_DETECTING: handleDetecting(); break;
        case STATE_READING:   handleReading(); break;
        case STATE_QUERYING:  handleQuerying(); break;
        case STATE_ROUTING:   handleRouting(); break;
        case STATE_ERROR:     handleError(); break;
    }

    delay(50);
}
