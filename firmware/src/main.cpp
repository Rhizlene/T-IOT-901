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

// API Dolibarr
const char* DOLIBARR_HOST = "172.22.27.165";
const char* DOLAPIKEY     = "1ZtIVnP5bQ9QMK983qXoii1kd3c72dRQ";

// Adresses I2C
#define RFID_I2C_ADDR   0x28
#define GOPLUS2_ADDR    0x38
#define GRBL_I2C_ADDR   0x70

// Servo (sur GoPlus2)
#define SERVO_CH1       0

// Mapping entrepots -> angles servo
#define WAREHOUSE_A_ANGLE   0    // Entrepot A = gauche
#define WAREHOUSE_B_ANGLE   90   // Entrepot B = centre
#define WAREHOUSE_C_ANGLE   180  // Entrepot C = droite
#define DEFAULT_ANGLE       90   // Position par defaut

// Timeouts (ms)
#define WIFI_TIMEOUT        20000
#define RFID_SCAN_TIMEOUT   5000
#define API_TIMEOUT         5000
#define MOTOR_MOVE_TIME     2000

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
String currentUID = "";
String currentProductRef = "";
int targetWarehouse = 1;

// Peripheriques
MFRC522 rfid(RFID_I2C_ADDR);
bool rfidOK = false;
bool grblOK = false;
bool servoOK = false;
bool wifiOK = false;

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
        case STATE_INIT: stateColor = BLUE; break;
        case STATE_READY: stateColor = GREEN; break;
        case STATE_DETECTING: stateColor = ORANGE; break;
        case STATE_READING: stateColor = MAGENTA; break;
        case STATE_QUERYING: stateColor = CYAN; break;
        case STATE_ROUTING: stateColor = YELLOW; break;
        case STATE_ERROR: stateColor = RED; break;
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
    for (size_t i = 0; i < strlen(cmd); i++) {
        Wire.write(cmd[i]);
    }
    Wire.write('\r');
    Wire.write('\n');
    Wire.endTransmission();

    delay(100);

    // Lire reponse
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

void conveyorForward(int distance_mm) {
    char cmd[32];
    sprintf(cmd, "G91");
    sendGcode(cmd);
    sprintf(cmd, "G1 Z%d F300", distance_mm);
    sendGcode(cmd);
    sendGcode("G90");
    delay(MOTOR_MOVE_TIME);
}

void conveyorBackward(int distance_mm) {
    char cmd[32];
    sprintf(cmd, "G91");
    sendGcode(cmd);
    sprintf(cmd, "G1 Z-%d F300", distance_mm);
    sendGcode(cmd);
    sendGcode("G90");
    delay(MOTOR_MOVE_TIME);
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

// Lecture des donnees NDEF du tag (pour NTAG/MIFARE Ultralight)
String readNDEFPayload() {
    if (!rfid.PICC_IsNewCardPresent()) return "";
    if (!rfid.PICC_ReadCardSerial()) return "";

    // Buffer pour lire les pages (NTAG213/215/216 = 4 bytes par page)
    // MIFARE_Read lit 16 bytes (4 pages) a la fois
    byte buffer[18];
    byte bufferSize = sizeof(buffer);
    String rawData = "";

    // Lire les pages 4 a 20 par blocs de 4 pages (16 bytes)
    // On lit page 4, 8, 12, 16 pour eviter les chevauchements
    for (byte page = 4; page <= 16; page += 4) {
        bufferSize = sizeof(buffer);
        byte status = rfid.MIFARE_Read(page, buffer, &bufferSize);
        if (status == rfid.STATUS_OK) {
            // Lire les 16 bytes retournes
            for (int i = 0; i < 16; i++) {
                rawData += (char)buffer[i];
            }
        } else {
            Serial.printf("MIFARE_Read page %d failed: %d\n", page, status);
            break;
        }
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    Serial.print("Raw NDEF data (hex): ");
    for (unsigned int i = 0; i < rawData.length() && i < 64; i++) {
        Serial.printf("%02X ", (uint8_t)rawData[i]);
    }
    Serial.println();

    // Chercher le JSON dans les donnees brutes
    // Le JSON peut etre prefixe par des headers NDEF (ex: "en" pour Text record)
    String payload = "";
    for (unsigned int i = 0; i < rawData.length(); i++) {
        char c = rawData[i];
        // Garder uniquement les caracteres imprimables ASCII
        if (c >= 32 && c < 127) {
            payload += c;
        }
    }

    Serial.println("Filtered payload: " + payload);

    // Extraire le JSON (tout ce qui est entre { et })
    int jsonStart = payload.indexOf('{');
    int jsonEnd = payload.lastIndexOf('}');
    if (jsonStart >= 0 && jsonEnd > jsonStart) {
        String json = payload.substring(jsonStart, jsonEnd + 1);
        Serial.println("Extracted JSON: " + json);
        return json;
    }

    Serial.println("No JSON found in NDEF payload");
    return "";
}

// Extraire la reference produit du JSON NDEF
String extractProductRef(const String& jsonPayload) {
    if (jsonPayload.length() == 0) return "";

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonPayload);

    if (!error) {
        // Chercher le champ "ref"
        if (doc.containsKey("ref")) {
            String ref = doc["ref"].as<String>();
            Serial.print("NDEF Ref: ");
            Serial.println(ref);
            return ref;
        }
    }

    Serial.println("JSON parse error ou champ 'ref' manquant");
    return "";
}

// ============================================================================
// FONCTIONS WIFI
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
// FONCTIONS API DOLIBARR
// ============================================================================

int queryProductWarehouse(const String& productRef) {
    if (WiFi.status() != WL_CONNECTED) return -1;

    HTTPClient http;
    String url = "http://";
    url += DOLIBARR_HOST;
    url += "/api/index.php/products?DOLAPIKEY=";
    url += DOLAPIKEY;
    url += "&sqlfilters=(ref:=:'";
    url += productRef;
    url += "')";

    Serial.print("API GET: ");
    Serial.println(url);

    http.begin(url);
    http.setTimeout(API_TIMEOUT);
    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        Serial.println("API Response: " + payload.substring(0, 200));

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error && doc.is<JsonArray>() && doc.size() > 0) {
            int warehouseId = doc[0]["fk_default_warehouse"] | 1;
            Serial.printf("Warehouse ID: %d\n", warehouseId);
            http.end();
            return warehouseId;
        }
    }

    Serial.printf("API Error: %d\n", httpCode);
    http.end();
    return -1;
}

bool postStockMovement(const String& productRef, int warehouseId) {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    String url = "http://";
    url += DOLIBARR_HOST;
    url += "/api/index.php/stockmovements?DOLAPIKEY=";
    url += DOLAPIKEY;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["product_id"] = productRef;
    doc["warehouse_id"] = warehouseId;
    doc["qty"] = 1;
    doc["type"] = 0;  // 0 = entree

    String json;
    serializeJson(doc, json);

    Serial.print("API POST: ");
    Serial.println(json);

    int httpCode = http.POST(json);
    http.end();

    return (httpCode == 200 || httpCode == 201);
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

    // Init I2C
    Wire.begin(21, 22);
    Wire.setClock(100000);
    delay(100);

    // Scan I2C
    Serial.println("\n=== I2C Scan ===");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  Device @ 0x%02X\n", addr);
        }
    }

    // Init peripheriques
    rfidOK = initRFID();
    grblOK = initGRBL();
    servoOK = initServo();

    // WiFi optionnel - ne bloque pas le demarrage
    displayStatus("Connexion WiFi...", BLUE);
    wifiOK = connectWiFi();
    if (!wifiOK) {
        Serial.println("WiFi KO - Mode degrade actif");
    }

    displayState();

    // WiFi n'est pas requis pour fonctionner (mode degrade)
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
    displayStatus("PRET - Attente colis", GREEN);

    // Verifier si un tag RFID est present
    if (rfid.PICC_IsNewCardPresent()) {
        setState(STATE_DETECTING);
    }

    // Bouton A = mode manuel avancer
    if (M5.BtnA.wasPressed()) {
        displayStatus("Manuel: Avancer", YELLOW);
        conveyorForward(50);
    }

    // Bouton B = mode manuel reculer
    if (M5.BtnB.wasPressed()) {
        displayStatus("Manuel: Reculer", YELLOW);
        conveyorBackward(50);
    }

    // Bouton C = test servo
    if (M5.BtnC.wasPressed()) {
        static int testAngle = 0;
        testAngle = (testAngle + 90) % 270;
        displayStatus("Test Servo", CYAN);
        setServoAngle(SERVO_CH1, testAngle);
        M5.Speaker.tone(800 + testAngle * 2, 50);
    }
}

void handleDetecting() {
    displayStatus("Colis detecte...", ORANGE);
    M5.Speaker.tone(800, 100);

    // Avancer le tapis pour positionner le colis sous le lecteur
    conveyorForward(100);

    setState(STATE_READING);
}

void handleReading() {
    displayStatus("Lecture RFID...", MAGENTA);

    unsigned long start = millis();
    currentUID = "";
    currentProductRef = "";
    int attempts = 0;

    while (millis() - start < RFID_SCAN_TIMEOUT) {
        attempts++;
        Serial.printf("Tentative lecture RFID #%d\n", attempts);

        // D'abord essayer de lire les donnees NDEF (JSON)
        String ndefPayload = readNDEFPayload();
        if (ndefPayload.length() > 0) {
            Serial.println("NDEF JSON trouve: " + ndefPayload);

            // Extraire la reference du JSON
            String ref = extractProductRef(ndefPayload);
            if (ref.length() > 0) {
                currentProductRef = ref;
                currentUID = "NDEF:" + ref;
                Serial.println("Produit extrait: " + currentProductRef);
                M5.Speaker.tone(1200, 100);
                displayState();
                setState(STATE_QUERYING);
                return;
            }
        }

        // Petite pause avant de reessayer
        delay(200);

        // Fallback: lire juste l'UID si pas de NDEF
        currentUID = readRFIDTag();
        if (currentUID.length() > 0) {
            Serial.println("UID lu: " + currentUID);
            M5.Speaker.tone(1200, 100);

            // Utiliser l'UID comme reference par defaut
            currentProductRef = "PROD-" + currentUID.substring(0, 8);

            displayState();
            setState(STATE_QUERYING);
            return;
        }

        delay(300);
    }

    // Timeout - tag non lu
    Serial.printf("RFID: Echec apres %d tentatives\n", attempts);
    lastError = "E030: Tag illisible";
    setState(STATE_ERROR);
}

void handleQuerying() {
    displayStatus("Requete API...", CYAN);

    // Requete API pour obtenir l'entrepot
    targetWarehouse = queryProductWarehouse(currentProductRef);

    if (targetWarehouse > 0) {
        Serial.printf("Produit -> Entrepot %d\n", targetWarehouse);

        // Enregistrer mouvement de stock
        postStockMovement(currentProductRef, targetWarehouse);

        setState(STATE_ROUTING);
    } else {
        // Mode degrade: utiliser entrepot par defaut
        Serial.println("API KO - Mode degrade");
        targetWarehouse = 1;
        setState(STATE_ROUTING);
    }
}

void handleRouting() {
    displayStatus("Aiguillage...", YELLOW);

    // Positionner le servo selon l'entrepot
    int angle = DEFAULT_ANGLE;
    switch (targetWarehouse) {
        case 1: angle = WAREHOUSE_A_ANGLE; break;
        case 2: angle = WAREHOUSE_B_ANGLE; break;
        case 3: angle = WAREHOUSE_C_ANGLE; break;
    }

    Serial.printf("Entrepot %d -> Servo %d deg\n", targetWarehouse, angle);
    setServoAngle(SERVO_CH1, angle);
    delay(500);

    // Ejecter le colis
    displayStatus("Ejection...", GREEN);
    conveyorForward(200);

    M5.Speaker.tone(1500, 200);

    // Retour position initiale
    setServoAngle(SERVO_CH1, DEFAULT_ANGLE);
    delay(300);

    // Reset pour prochain colis
    currentUID = "";
    currentProductRef = "";

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
    Serial.println("  THE CONVEYOR - Firmware v1.0");
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
