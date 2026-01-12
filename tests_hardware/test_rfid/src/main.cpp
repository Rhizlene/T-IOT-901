#include <M5Stack.h>
#include <Wire.h>

// M5Stack RFID Unit - try multiple protocols
#define RFID_I2C_ADDR 0x28

// UART mode
HardwareSerial RFIDSerial(2);  // Use Serial2

bool useUART = false;

// Function to read UID via I2C
String readRFIDCard_I2C() {
    Wire.requestFrom(RFID_I2C_ADDR, 8);

    if (Wire.available() < 8) {
        return "";
    }

    uint8_t data[8];
    for (int i = 0; i < 8; i++) {
        data[i] = Wire.read();
    }

    // Check for valid data
    if (data[0] == 0x01 && data[1] > 0 && data[1] <= 7) {
        String uid = "";
        for (int i = 0; i < data[1] && i < 5; i++) {
            if (data[2 + i] < 0x10) uid += "0";
            uid += String(data[2 + i], HEX);
            if (i < data[1] - 1 && i < 4) uid += ":";
        }
        uid.toUpperCase();
        return uid;
    }

    return "";
}

// Function to read UID via UART
String readRFIDCard_UART() {
    if (RFIDSerial.available() >= 8) {
        uint8_t data[14];
        int count = 0;

        while (RFIDSerial.available() && count < 14) {
            data[count++] = RFIDSerial.read();
        }

        // Look for hex UID in ASCII format
        String response = "";
        for (int i = 0; i < count; i++) {
            if (data[i] >= '0' && data[i] <= '9' || data[i] >= 'A' && data[i] <= 'F' || data[i] >= 'a' && data[i] <= 'f') {
                response += (char)data[i];
            }
        }

        if (response.length() >= 8) {
            String uid = "";
            for (int i = 0; i < response.length() && i < 14; i++) {
                uid += response.charAt(i);
                if (i % 2 == 1 && i < response.length() - 1) {
                    uid += ":";
                }
            }
            uid.toUpperCase();
            return uid;
        }
    }

    return "";
}

void setup() {
    M5.begin();
    M5.Power.begin();
    Serial.begin(115200);

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("=== RFID TEST ===");
    M5.Lcd.println("Multi-Protocol");

    Serial.println("\n============================");
    Serial.println("  RFID Unit Test (I2C+UART)");
    Serial.println("============================");

    // Initialize I2C
    Wire.begin(21, 22);
    Wire.setClock(100000);

    Serial.println("Scanning I2C bus...");
    M5.Lcd.println("Scanning I2C...");

    int deviceCount = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("Device at 0x");
            if (addr < 0x10) Serial.print("0");
            Serial.println(addr, HEX);
            deviceCount++;
        }
    }

    Serial.print("Found ");
    Serial.print(deviceCount);
    Serial.println(" I2C device(s)\n");

    // Check for RFID at 0x28
    Wire.beginTransmission(RFID_I2C_ADDR);
    if (Wire.endTransmission() == 0) {
        Serial.println("Device found at 0x28 - trying I2C mode");
        M5.Lcd.println("I2C Mode");
        useUART = false;

        // Send init commands
        Serial.println("Sending init commands...");
        uint8_t initCmds[][4] = {
            {0x00, 0x00, 0x00, 0x00},
            {0x01, 0x00, 0x00, 0x00},
            {0xFF, 0x00, 0x00, 0x00},
        };

        for (int i = 0; i < 3; i++) {
            Wire.beginTransmission(RFID_I2C_ADDR);
            Wire.write(initCmds[i], 4);
            Wire.endTransmission();
            delay(100);
        }
    } else {
        Serial.println("No I2C device at 0x28 - trying UART mode");
        M5.Lcd.println("UART Mode");
        useUART = true;

        // Initialize UART on pins 21 (RX) and 22 (TX)
        RFIDSerial.begin(9600, SERIAL_8N1, 21, 22);
        delay(500);
    }

    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("Ready!");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("");
    M5.Lcd.println("Place card...");

    Serial.println("\n>>> Ready to scan <<<");
    Serial.println(">>> Place card near reader <<<\n");
}

String lastUID = "";
unsigned long lastReadTime = 0;
unsigned long lastDebugTime = 0;

void loop() {
    M5.update();

    unsigned long currentTime = millis();

    // Debug output every 2 seconds
    if (currentTime - lastDebugTime > 2000) {
        lastDebugTime = currentTime;

        if (!useUART) {
            // I2C debug
            Wire.requestFrom(RFID_I2C_ADDR, 8);
            if (Wire.available() >= 8) {
                uint8_t rawData[8];
                for (int i = 0; i < 8; i++) {
                    rawData[i] = Wire.read();
                }

                Serial.print("[I2C] Raw: ");
                for (int i = 0; i < 8; i++) {
                    if (rawData[i] < 0x10) Serial.print("0");
                    Serial.print(rawData[i], HEX);
                    Serial.print(" ");
                }

                Serial.print("| Card: ");
                Serial.print(rawData[0] == 0x01 ? "YES" : "NO");
                Serial.print(" | UID Len: ");
                Serial.print(rawData[1]);

                if (rawData[0] == 0x01 && rawData[1] > 0 && rawData[1] <= 7) {
                    Serial.print(" | UID: ");
                    for (int i = 0; i < rawData[1] && i < 5; i++) {
                        if (rawData[2 + i] < 0x10) Serial.print("0");
                        Serial.print(rawData[2 + i], HEX);
                        if (i < rawData[1] - 1 && i < 4) Serial.print(":");
                    }
                }
                Serial.println();
            }
        } else {
            // UART debug
            if (RFIDSerial.available() > 0) {
                Serial.print("[UART] Available: ");
                Serial.print(RFIDSerial.available());
                Serial.print(" bytes | Data: ");

                String data = "";
                while (RFIDSerial.available()) {
                    uint8_t b = RFIDSerial.read();
                    if (b < 0x10) Serial.print("0");
                    Serial.print(b, HEX);
                    Serial.print(" ");
                    data += (char)b;
                }
                Serial.print("| ASCII: '");
                Serial.print(data);
                Serial.println("'");
            } else {
                Serial.println("[UART] No data");
            }
        }
    }

    // Try to read card
    if (currentTime - lastReadTime > 200) {
        lastReadTime = currentTime;

        String uid = useUART ? readRFIDCard_UART() : readRFIDCard_I2C();

        if (uid.length() > 0 && uid != lastUID) {
            lastUID = uid;

            M5.Lcd.fillRect(0, 120, 320, 120, BLACK);
            M5.Lcd.setCursor(10, 120);
            M5.Lcd.setTextColor(YELLOW);
            M5.Lcd.println("CARD DETECTED!");

            Serial.println("\n========== CARD DETECTED ==========");
            Serial.print("UID: ");
            Serial.println(uid);
            Serial.println("===================================\n");

            M5.Lcd.setTextColor(WHITE);
            M5.Lcd.print("UID: ");
            M5.Lcd.println(uid);

            M5.Lcd.setTextColor(GREEN);
            M5.Lcd.println("Success!");

            delay(1500);

            M5.Lcd.fillRect(0, 120, 320, 120, BLACK);
            M5.Lcd.setCursor(10, 120);
            M5.Lcd.setTextColor(WHITE);
            M5.Lcd.println("Place next card...");

        } else if (uid.length() == 0 && lastUID.length() > 0) {
            if (currentTime - lastReadTime > 1000) {
                lastUID = "";
            }
        }
    }

    delay(50);
}
