#include "rfid_reader.h"

MFRC522 rfid(RFID_I2C_ADDR);

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

// Retourne l'UID au format "04:82:..." ou "" si aucun tag
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
