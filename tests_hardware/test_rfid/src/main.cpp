/*
 * M5Stack RFID 2 Unit Test
 * Using official M5Stack MFRC522 I2C library
 */

#include <M5Stack.h>
#include <Wire.h>
#include "MFRC522_I2C.h"

// M5Stack RFID 2 Unit I2C address
#define RFID_I2C_ADDR 0x28

MFRC522 mfrc522(RFID_I2C_ADDR);

void setup() {
    M5.begin();
    M5.Power.begin();
    Serial.begin(115200);

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("=== RFID 2 TEST ===");
    M5.Lcd.println("Lib: M5Stack MFRC522");

    Serial.println("\n=============================");
    Serial.println("  M5Stack RFID 2 Unit Test");
    Serial.println("  Using M5Stack MFRC522 I2C");
    Serial.println("=============================\n");

    // Initialize I2C on Port A (GPIO 21 SDA, GPIO 22 SCL)
    Wire.begin(21, 22);
    Wire.setClock(100000);  // 100kHz I2C

    Serial.println("Scanning I2C bus...");

    // Scan I2C bus
    int deviceCount = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("  Device at 0x");
            if (addr < 0x10) Serial.print("0");
            Serial.print(addr, HEX);
            
            if (addr == RFID_I2C_ADDR) {
                Serial.print(" <- RFID 2 Unit");
            }
            Serial.println();
            deviceCount++;
        }
    }
    Serial.print("Found ");
    Serial.print(deviceCount);
    Serial.println(" I2C device(s)\n");

    // Initialize MFRC522
    Serial.println("Initializing MFRC522...");
    mfrc522.PCD_Init();
    delay(100);

    // Check firmware version
    byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    Serial.print("MFRC522 Firmware Version: 0x");
    Serial.print(version, HEX);
    if (version == 0x91 || version == 0x92) {
        Serial.println(" (OK)");
    } else if (version == 0x88) {
        Serial.println(" (FM17522 clone)");
    } else if (version == 0x00 || version == 0xFF) {
        Serial.println(" (Communication error!)");
    } else {
        Serial.println(" (Unknown version)");
    }

    // Set antenna gain to maximum
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
    Serial.print("Antenna Gain: 0x");
    Serial.println(mfrc522.PCD_GetAntennaGain(), HEX);

    Serial.println("\n>>> Ready to scan! <<<");
    Serial.println(">>> Place NFC tag on reader <<<\n");

    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("\nRFID Ready!");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("\nPlace card on");
    M5.Lcd.println("RFID 2 Unit...");
}

String lastUID = "";
unsigned long lastDisplayTime = 0;

void loop() {
    M5.update();

    // Look for new cards
    if (!mfrc522.PICC_IsNewCardPresent()) {
        delay(50);
        return;
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) {
        delay(50);
        return;
    }

    // Build UID string
    String uidStr = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) {
            uidStr += "0";
        }
        uidStr += String(mfrc522.uid.uidByte[i], HEX);
        if (i < mfrc522.uid.size - 1) {
            uidStr += ":";
        }
    }
    uidStr.toUpperCase();

    // Avoid duplicate reads
    if (uidStr != lastUID || millis() - lastDisplayTime > 2000) {
        lastUID = uidStr;
        lastDisplayTime = millis();

        // Print to Serial
        Serial.println("================================");
        Serial.print("Card UID: ");
        Serial.println(uidStr);
        Serial.print("UID Size: ");
        Serial.print(mfrc522.uid.size);
        Serial.println(" bytes");
        
        // Get card type
        byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
        Serial.print("Card Type: ");
        Serial.println(mfrc522.PICC_GetTypeName(piccType));
        Serial.println("================================\n");

        // Beep
        M5.Speaker.tone(1000, 100);

        // Display on LCD
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(10, 10);
        M5.Lcd.setTextColor(GREEN);
        M5.Lcd.setTextSize(2);
        M5.Lcd.println("Card Detected!");
        
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.println("\nUID:");
        M5.Lcd.setTextColor(YELLOW);
        M5.Lcd.setTextSize(2);
        M5.Lcd.println(uidStr);

        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.setTextSize(1);
        M5.Lcd.println("\nType:");
        M5.Lcd.println(mfrc522.PICC_GetTypeName(piccType));
    }

    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
    
    delay(100);
}
