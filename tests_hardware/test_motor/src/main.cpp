/*
 * M5Stack GRBL Module 13.2 - Conveyor Belt Test
 * Motor: NEMA 17 (17HS4401)
 *
 * Branchement sur connecteur Z (convoyeur):
 *   B-     B+     A-     A+
 *   Noir   Vert   Bleu   Rouge
 *
 * IMPORTANT: Alimentation externe 9-24V sur le GRBL Module (pas GoPlus2!)
 */

#include <M5Stack.h>
#include <Wire.h>

// GRBL Module I2C address
#define GRBL_I2C_ADDR 0x70

// Mode d'envoi des commandes GRBL
#define GRBL_REG_GCODE 0x00
#define GRBL_REG_STATUS 0x01

// Function to send G-code command to GRBL module (methode officielle)
void sendGcode(const char* cmd) {
    Serial.print("TX: ");
    Serial.println(cmd);

    // Methode 1: Envoi direct caractere par caractere
    Wire.beginTransmission(GRBL_I2C_ADDR);
    for (size_t i = 0; i < strlen(cmd); i++) {
        Wire.write(cmd[i]);
    }
    Wire.write('\r');  // Carriage return
    Wire.write('\n');  // Line feed
    uint8_t error = Wire.endTransmission();

    if (error != 0) {
        Serial.print("I2C Error: ");
        Serial.println(error);
    }

    delay(100);  // Attendre que GRBL traite la commande

    // Lire la reponse
    Wire.requestFrom(GRBL_I2C_ADDR, 32);
    String response = "";
    while (Wire.available()) {
        char c = Wire.read();
        if (c >= 32 && c < 127) {
            response += c;
        }
    }
    if (response.length() > 0) {
        Serial.print("RX: ");
        Serial.println(response);
    }
}

// Envoyer commande avec registre specifique
void sendGcodeReg(uint8_t reg, const char* cmd) {
    Serial.print("TX[");
    Serial.print(reg);
    Serial.print("]: ");
    Serial.println(cmd);

    Wire.beginTransmission(GRBL_I2C_ADDR);
    Wire.write(reg);
    for (size_t i = 0; i < strlen(cmd); i++) {
        Wire.write(cmd[i]);
    }
    Wire.write('\n');
    Wire.endTransmission();

    delay(100);
}

// Lire le statut du GRBL
void readStatus() {
    Wire.beginTransmission(GRBL_I2C_ADDR);
    Wire.write('?');  // Status query
    Wire.endTransmission();

    delay(50);

    Wire.requestFrom(GRBL_I2C_ADDR, 64);
    Serial.print("Status: ");
    while (Wire.available()) {
        char c = Wire.read();
        if (c >= 32 && c < 127) {
            Serial.print(c);
        }
    }
    Serial.println();
}

// Scanner I2C pour debug
void scanI2C() {
    Serial.println("Scanning I2C bus...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("  Found device at 0x");
            Serial.println(addr, HEX);
        }
    }
}

void setup() {
    M5.begin(true, false, true, false);  // LCD, SD, Serial, I2C
    M5.Power.begin();
    Serial.begin(115200);

    delay(100);

    // Initialize I2C
    Wire.begin(21, 22);
    Wire.setClock(100000);

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("=== CONVEYOR TEST ===");
    M5.Lcd.println("");
    M5.Lcd.println("GRBL Module 13.2");
    M5.Lcd.println("Tapis roulant");
    M5.Lcd.println("");

    Serial.println("================================");
    Serial.println("  Conveyor Belt Test");
    Serial.println("  GRBL Module 13.2 + 17HS4401");
    Serial.println("================================");

    // Scan I2C
    scanI2C();

    // Check GRBL module
    Wire.beginTransmission(GRBL_I2C_ADDR);
    if (Wire.endTransmission() == 0) {
        M5.Lcd.setTextColor(GREEN);
        M5.Lcd.println("GRBL: OK (0x70)");
        Serial.println("GRBL Module found at 0x70");
    } else {
        M5.Lcd.setTextColor(RED);
        M5.Lcd.println("GRBL: NOT FOUND!");
        Serial.println("ERROR: GRBL Module not found!");
    }

    M5.Lcd.println("");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("PORT Z");
    M5.Lcd.println("A: Avancer (10mm)");
    M5.Lcd.println("B: Reculer (10mm)");
    M5.Lcd.println("C: Status");

    // Initialize GRBL with delays
    delay(1000);  // Wait for GRBL to boot

    Serial.println("\nInitializing GRBL...");

    // Reset and unlock
    sendGcode("$X");        // Unlock (clear alarm)
    delay(200);

    // Configure for conveyor - Z axis
    sendGcode("$102=80");   // Z steps/mm (adjust as needed)
    delay(100);
    sendGcode("$112=500");  // Z max rate mm/min
    delay(100);
    sendGcode("$122=50");   // Z acceleration mm/sec^2
    delay(100);

    // Set modes
    sendGcode("G21");       // Millimeters
    delay(100);
    sendGcode("G90");       // Absolute positioning
    delay(100);
    sendGcode("G92 Z0");    // Set current position as zero
    delay(100);

    Serial.println("");
    Serial.println("Ready! Press buttons to test.");
    Serial.println("================================");

    // Read initial status
    readStatus();
}

void loop() {
    M5.update();

    // Button A: Avancer le convoyeur
    if (M5.BtnA.wasPressed()) {
        M5.Lcd.fillRect(0, 200, 320, 40, BLACK);
        M5.Lcd.setCursor(10, 200);
        M5.Lcd.setTextColor(YELLOW);
        M5.Lcd.println(">>> Avancer");

        Serial.println("\n--- Button A: Avancer ---");
        sendGcode("G91");           // Relative mode
        sendGcode("G1 Z10 F300");   // Move Z axis (conveyor forward)
        sendGcode("G90");           // Back to absolute

        readStatus();
    }

    // Button B: Reculer le convoyeur
    if (M5.BtnB.wasPressed()) {
        M5.Lcd.fillRect(0, 200, 320, 40, BLACK);
        M5.Lcd.setCursor(10, 200);
        M5.Lcd.setTextColor(CYAN);
        M5.Lcd.println(">>> Reculer");

        Serial.println("\n--- Button B: Reculer ---");
        sendGcode("G91");           // Relative mode
        sendGcode("G1 Z-10 F300");  // Move Z axis (conveyor backward)
        sendGcode("G90");           // Back to absolute

        readStatus();
    }

    // Button C: Status / Test
    if (M5.BtnC.wasPressed()) {
        M5.Lcd.fillRect(0, 200, 320, 40, BLACK);
        M5.Lcd.setCursor(10, 200);
        M5.Lcd.setTextColor(GREEN);
        M5.Lcd.println(">>> Status");

        Serial.println("\n--- Button C: Status ---");
        readStatus();
        sendGcode("$G");  // View parser state
        sendGcode("$$");  // View settings
    }

    delay(50);
}
