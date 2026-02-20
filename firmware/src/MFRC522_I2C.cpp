/*
 * MFRC522_I2C.cpp - Library for RFID MFRC522 via I2C
 * Author: arozcan @ https://github.com/arozcan/MFRC522-I2C-Library
 */

#include "MFRC522_I2C.h"

MFRC522::MFRC522(byte chipAddress) { _chipAddress = chipAddress; }

void MFRC522::PCD_WriteRegister(byte reg, byte value) {
    Wire.beginTransmission(_chipAddress);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void MFRC522::PCD_WriteRegister(byte reg, byte count, byte *values) {
    Wire.beginTransmission(_chipAddress);
    Wire.write(reg);
    for (byte i = 0; i < count; i++) Wire.write(values[i]);
    Wire.endTransmission();
}

byte MFRC522::PCD_ReadRegister(byte reg) {
    Wire.beginTransmission(_chipAddress);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(_chipAddress, (uint8_t)1);
    return Wire.read();
}

void MFRC522::PCD_ReadRegister(byte reg, byte count, byte *values, byte rxAlign) {
    if (count == 0) return;
    Wire.beginTransmission(_chipAddress);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(_chipAddress, count);
    byte index = 0;
    while (Wire.available()) {
        if (index == 0 && rxAlign) {
            byte mask = 0;
            for (byte i = rxAlign; i <= 7; i++) mask |= (1 << i);
            byte value = Wire.read();
            values[0] = (values[index] & ~mask) | (value & mask);
        } else {
            values[index] = Wire.read();
        }
        index++;
    }
}

void MFRC522::PCD_SetRegisterBitMask(byte reg, byte mask) {
    PCD_WriteRegister(reg, PCD_ReadRegister(reg) | mask);
}

void MFRC522::PCD_ClearRegisterBitMask(byte reg, byte mask) {
    PCD_WriteRegister(reg, PCD_ReadRegister(reg) & (~mask));
}

byte MFRC522::PCD_CalculateCRC(byte *data, byte length, byte *result) {
    PCD_WriteRegister(CommandReg, PCD_Idle);
    PCD_WriteRegister(DivIrqReg, 0x04);
    PCD_SetRegisterBitMask(FIFOLevelReg, 0x80);
    PCD_WriteRegister(FIFODataReg, length, data);
    PCD_WriteRegister(CommandReg, PCD_CalcCRC);
    for (word i = 5000; i > 0; i--) {
        byte n = PCD_ReadRegister(DivIrqReg);
        if (n & 0x04) {
            PCD_WriteRegister(CommandReg, PCD_Idle);
            result[0] = PCD_ReadRegister(CRCResultRegL);
            result[1] = PCD_ReadRegister(CRCResultRegH);
            return STATUS_OK;
        }
    }
    return STATUS_TIMEOUT;
}

void MFRC522::PCD_Init() {
    PCD_Reset();
    PCD_WriteRegister(TModeReg, 0x80);
    PCD_WriteRegister(TPrescalerReg, 0xA9);
    PCD_WriteRegister(TReloadRegH, 0x03);
    PCD_WriteRegister(TReloadRegL, 0xE8);
    PCD_WriteRegister(TxASKReg, 0x40);
    PCD_WriteRegister(ModeReg, 0x3D);
    PCD_AntennaOn();
}

void MFRC522::PCD_Reset() {
    PCD_WriteRegister(CommandReg, PCD_SoftReset);
    delay(50);
    while (PCD_ReadRegister(CommandReg) & (1 << 4));
}

void MFRC522::PCD_AntennaOn() {
    byte value = PCD_ReadRegister(TxControlReg);
    if ((value & 0x03) != 0x03) PCD_WriteRegister(TxControlReg, value | 0x03);
}

void MFRC522::PCD_AntennaOff() { PCD_ClearRegisterBitMask(TxControlReg, 0x03); }

byte MFRC522::PCD_GetAntennaGain() { return PCD_ReadRegister(RFCfgReg) & (0x07 << 4); }

void MFRC522::PCD_SetAntennaGain(byte mask) {
    if (PCD_GetAntennaGain() != mask) {
        PCD_ClearRegisterBitMask(RFCfgReg, (0x07 << 4));
        PCD_SetRegisterBitMask(RFCfgReg, mask & (0x07 << 4));
    }
}

bool MFRC522::PCD_PerformSelfTest() {
    PCD_Reset();
    byte ZEROES[25] = {0x00};
    PCD_SetRegisterBitMask(FIFOLevelReg, 0x80);
    PCD_WriteRegister(FIFODataReg, 25, ZEROES);
    PCD_WriteRegister(CommandReg, PCD_Mem);
    PCD_WriteRegister(AutoTestReg, 0x09);
    PCD_WriteRegister(FIFODataReg, 0x00);
    PCD_WriteRegister(CommandReg, PCD_CalcCRC);
    for (word i = 0; i < 0xFF; i++) {
        if (PCD_ReadRegister(DivIrqReg) & 0x04) break;
    }
    PCD_WriteRegister(CommandReg, PCD_Idle);
    byte result[64];
    PCD_ReadRegister(FIFODataReg, 64, result, 0);
    PCD_WriteRegister(AutoTestReg, 0x00);
    byte version = PCD_ReadRegister(VersionReg);
    const byte *reference;
    switch (version) {
        case 0x88: reference = FM17522_firmware_reference; break;
        case 0x90: reference = MFRC522_firmware_referenceV0_0; break;
        case 0x91: reference = MFRC522_firmware_referenceV1_0; break;
        case 0x92: reference = MFRC522_firmware_referenceV2_0; break;
        default: return false;
    }
    for (word i = 0; i < 64; i++) {
        if (result[i] != pgm_read_byte(&(reference[i]))) return false;
    }
    return true;
}

byte MFRC522::PCD_TransceiveData(byte *sendData, byte sendLen, byte *backData,
                                 byte *backLen, byte *validBits, byte rxAlign, bool checkCRC) {
    return PCD_CommunicateWithPICC(PCD_Transceive, 0x30, sendData, sendLen,
                                   backData, backLen, validBits, rxAlign, checkCRC);
}

byte MFRC522::PCD_CommunicateWithPICC(byte command, byte waitIRq, byte *sendData,
                                      byte sendLen, byte *backData, byte *backLen,
                                      byte *validBits, byte rxAlign, bool checkCRC) {
    byte txLastBits = validBits ? *validBits : 0;
    byte bitFraming = (rxAlign << 4) + txLastBits;
    PCD_WriteRegister(CommandReg, PCD_Idle);
    PCD_WriteRegister(ComIrqReg, 0x7F);
    PCD_SetRegisterBitMask(FIFOLevelReg, 0x80);
    PCD_WriteRegister(FIFODataReg, sendLen, sendData);
    PCD_WriteRegister(BitFramingReg, bitFraming);
    PCD_WriteRegister(CommandReg, command);
    if (command == PCD_Transceive) PCD_SetRegisterBitMask(BitFramingReg, 0x80);

    for (unsigned int i = 2000; i > 0; i--) {
        byte n = PCD_ReadRegister(ComIrqReg);
        if (n & waitIRq) break;
        if (n & 0x01) return STATUS_TIMEOUT;
        if (i == 1) return STATUS_TIMEOUT;
    }

    byte errorReg = PCD_ReadRegister(ErrorReg);
    if (errorReg & 0x13) return STATUS_ERROR;

    byte _validBits = 0;
    if (backData && backLen) {
        byte n = PCD_ReadRegister(FIFOLevelReg);
        if (n > *backLen) return STATUS_NO_ROOM;
        *backLen = n;
        PCD_ReadRegister(FIFODataReg, n, backData, rxAlign);
        _validBits = PCD_ReadRegister(ControlReg) & 0x07;
        if (validBits) *validBits = _validBits;
    }

    if (errorReg & 0x08) return STATUS_COLLISION;

    if (backData && backLen && checkCRC) {
        if (*backLen == 1 && _validBits == 4) return STATUS_MIFARE_NACK;
        if (*backLen < 2 || _validBits != 0) return STATUS_CRC_WRONG;
        byte controlBuffer[2];
        byte n = PCD_CalculateCRC(&backData[0], *backLen - 2, &controlBuffer[0]);
        if (n != STATUS_OK) return n;
        if ((backData[*backLen - 2] != controlBuffer[0]) ||
            (backData[*backLen - 1] != controlBuffer[1])) return STATUS_CRC_WRONG;
    }
    return STATUS_OK;
}

byte MFRC522::PICC_RequestA(byte *bufferATQA, byte *bufferSize) {
    return PICC_REQA_or_WUPA(PICC_CMD_REQA, bufferATQA, bufferSize);
}

byte MFRC522::PICC_WakeupA(byte *bufferATQA, byte *bufferSize) {
    return PICC_REQA_or_WUPA(PICC_CMD_WUPA, bufferATQA, bufferSize);
}

byte MFRC522::PICC_REQA_or_WUPA(byte command, byte *bufferATQA, byte *bufferSize) {
    if (bufferATQA == NULL || *bufferSize < 2) return STATUS_NO_ROOM;
    PCD_ClearRegisterBitMask(CollReg, 0x80);
    byte validBits = 7;
    byte status = PCD_TransceiveData(&command, 1, bufferATQA, bufferSize, &validBits);
    if (status != STATUS_OK) return status;
    if (*bufferSize != 2 || validBits != 0) return STATUS_ERROR;
    return STATUS_OK;
}

byte MFRC522::PICC_Select(Uid *uid, byte validBits) {
    bool uidComplete = false;
    byte cascadeLevel = 1;

    if (validBits > 80) return STATUS_INVALID;
    PCD_ClearRegisterBitMask(CollReg, 0x80);

    while (!uidComplete) {
        byte buffer[9];
        byte uidIndex;
        bool useCascadeTag;

        switch (cascadeLevel) {
            case 1: buffer[0] = PICC_CMD_SEL_CL1; uidIndex = 0; useCascadeTag = validBits && uid->size > 4; break;
            case 2: buffer[0] = PICC_CMD_SEL_CL2; uidIndex = 3; useCascadeTag = validBits && uid->size > 7; break;
            case 3: buffer[0] = PICC_CMD_SEL_CL3; uidIndex = 6; useCascadeTag = false; break;
            default: return STATUS_INTERNAL_ERROR;
        }

        int8_t currentLevelKnownBits = validBits - (8 * uidIndex);
        if (currentLevelKnownBits < 0) currentLevelKnownBits = 0;

        byte index = 2;
        if (useCascadeTag) buffer[index++] = PICC_CMD_CT;

        byte bytesToCopy = currentLevelKnownBits / 8 + (currentLevelKnownBits % 8 ? 1 : 0);
        if (bytesToCopy) {
            byte maxBytes = useCascadeTag ? 3 : 4;
            if (bytesToCopy > maxBytes) bytesToCopy = maxBytes;
            for (byte count = 0; count < bytesToCopy; count++) {
                buffer[index++] = uid->uidByte[uidIndex + count];
            }
        }
        if (useCascadeTag) currentLevelKnownBits += 8;

        bool selectDone = false;
        while (!selectDone) {
            byte txLastBits, bufferUsed, responseLength;
            byte *responseBuffer;
            byte result;

            if (currentLevelKnownBits >= 32) {
                buffer[1] = 0x70;
                buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
                result = PCD_CalculateCRC(buffer, 7, &buffer[7]);
                if (result != STATUS_OK) return result;
                txLastBits = 0;
                bufferUsed = 9;
                responseBuffer = &buffer[6];
                responseLength = 3;
            } else {
                txLastBits = currentLevelKnownBits % 8;
                byte count = currentLevelKnownBits / 8;
                index = 2 + count;
                buffer[1] = (index << 4) + txLastBits;
                bufferUsed = index + (txLastBits ? 1 : 0);
                responseBuffer = &buffer[index];
                responseLength = sizeof(buffer) - index;
            }

            byte rxAlign = txLastBits;
            PCD_WriteRegister(BitFramingReg, (rxAlign << 4) + txLastBits);
            result = PCD_TransceiveData(buffer, bufferUsed, responseBuffer, &responseLength, &txLastBits, rxAlign);

            if (result == STATUS_COLLISION) {
                result = PCD_ReadRegister(CollReg);
                if (result & 0x20) return STATUS_COLLISION;
                byte collisionPos = result & 0x1F;
                if (collisionPos == 0) collisionPos = 32;
                if (collisionPos <= currentLevelKnownBits) return STATUS_INTERNAL_ERROR;
                currentLevelKnownBits = collisionPos;
                byte count = (currentLevelKnownBits - 1) % 8;
                index = 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0);
                buffer[index] |= (1 << count);
            } else if (result != STATUS_OK) {
                return result;
            } else {
                selectDone = (currentLevelKnownBits >= 32);
                if (!selectDone) currentLevelKnownBits = 32;
            }
        }

        index = (buffer[2] == PICC_CMD_CT) ? 3 : 2;
        bytesToCopy = (buffer[2] == PICC_CMD_CT) ? 3 : 4;
        for (byte count = 0; count < bytesToCopy; count++) {
            uid->uidByte[uidIndex + count] = buffer[index++];
        }

        byte *responseBuffer = &buffer[6];
        if (responseBuffer[0] & 0x04) {
            cascadeLevel++;
        } else {
            uidComplete = true;
            uid->sak = responseBuffer[0];
        }
    }
    uid->size = 3 * cascadeLevel + 1;
    return STATUS_OK;
}

byte MFRC522::PICC_HaltA() {
    byte buffer[4];
    buffer[0] = PICC_CMD_HLTA;
    buffer[1] = 0;
    byte result = PCD_CalculateCRC(buffer, 2, &buffer[2]);
    if (result != STATUS_OK) return result;
    result = PCD_TransceiveData(buffer, sizeof(buffer), NULL, 0);
    if (result == STATUS_TIMEOUT) return STATUS_OK;
    if (result == STATUS_OK) return STATUS_ERROR;
    return result;
}

byte MFRC522::PCD_Authenticate(byte command, byte blockAddr, MIFARE_Key *key, Uid *uid) {
    byte sendData[12];
    sendData[0] = command;
    sendData[1] = blockAddr;
    for (byte i = 0; i < MF_KEY_SIZE; i++) sendData[2 + i] = key->keyByte[i];
    for (byte i = 0; i < 4; i++) sendData[8 + i] = uid->uidByte[i];
    return PCD_CommunicateWithPICC(PCD_MFAuthent, 0x10, &sendData[0], sizeof(sendData));
}

void MFRC522::PCD_StopCrypto1() { PCD_ClearRegisterBitMask(Status2Reg, 0x08); }

byte MFRC522::MIFARE_Read(byte blockAddr, byte *buffer, byte *bufferSize) {
    if (buffer == NULL || *bufferSize < 18) return STATUS_NO_ROOM;
    buffer[0] = PICC_CMD_MF_READ;
    buffer[1] = blockAddr;
    byte result = PCD_CalculateCRC(buffer, 2, &buffer[2]);
    if (result != STATUS_OK) return result;
    return PCD_TransceiveData(buffer, 4, buffer, bufferSize, NULL, 0, true);
}

byte MFRC522::MIFARE_Write(byte blockAddr, byte *buffer, byte bufferSize) {
    if (buffer == NULL || bufferSize < 16) return STATUS_INVALID;
    byte cmdBuffer[2] = {PICC_CMD_MF_WRITE, blockAddr};
    byte result = PCD_MIFARE_Transceive(cmdBuffer, 2);
    if (result != STATUS_OK) return result;
    return PCD_MIFARE_Transceive(buffer, bufferSize);
}

byte MFRC522::MIFARE_TwoStepHelper(byte command, byte blockAddr, long data) {
    byte cmdBuffer[2] = {command, blockAddr};
    byte result = PCD_MIFARE_Transceive(cmdBuffer, 2);
    if (result != STATUS_OK) return result;
    return PCD_MIFARE_Transceive((byte *)&data, 4, true);
}

byte MFRC522::PCD_MIFARE_Transceive(byte *sendData, byte sendLen, bool acceptTimeout) {
    if (sendData == NULL || sendLen > 16) return STATUS_INVALID;
    byte cmdBuffer[18];
    memcpy(cmdBuffer, sendData, sendLen);
    byte result = PCD_CalculateCRC(cmdBuffer, sendLen, &cmdBuffer[sendLen]);
    if (result != STATUS_OK) return result;
    sendLen += 2;
    byte cmdBufferSize = sizeof(cmdBuffer);
    byte validBits = 0;
    result = PCD_CommunicateWithPICC(PCD_Transceive, 0x30, cmdBuffer, sendLen,
                                     cmdBuffer, &cmdBufferSize, &validBits);
    if (acceptTimeout && result == STATUS_TIMEOUT) return STATUS_OK;
    if (result != STATUS_OK) return result;
    if (cmdBufferSize != 1 || validBits != 4) return STATUS_ERROR;
    if (cmdBuffer[0] != MF_ACK) return STATUS_MIFARE_NACK;
    return STATUS_OK;
}

const __FlashStringHelper *MFRC522::GetStatusCodeName(byte code) {
    switch (code) {
        case STATUS_OK: return F("Success");
        case STATUS_ERROR: return F("Error");
        case STATUS_COLLISION: return F("Collision");
        case STATUS_TIMEOUT: return F("Timeout");
        case STATUS_NO_ROOM: return F("No room");
        case STATUS_INTERNAL_ERROR: return F("Internal error");
        case STATUS_INVALID: return F("Invalid");
        case STATUS_CRC_WRONG: return F("CRC wrong");
        case STATUS_MIFARE_NACK: return F("MIFARE NACK");
        default: return F("Unknown");
    }
}

byte MFRC522::PICC_GetType(byte sak) {
    if (sak & 0x04) return PICC_TYPE_NOT_COMPLETE;
    switch (sak) {
        case 0x09: return PICC_TYPE_MIFARE_MINI;
        case 0x08: return PICC_TYPE_MIFARE_1K;
        case 0x18: return PICC_TYPE_MIFARE_4K;
        case 0x00: return PICC_TYPE_MIFARE_UL;
        case 0x10: case 0x11: return PICC_TYPE_MIFARE_PLUS;
        case 0x01: return PICC_TYPE_TNP3XXX;
    }
    if (sak & 0x20) return PICC_TYPE_ISO_14443_4;
    if (sak & 0x40) return PICC_TYPE_ISO_18092;
    return PICC_TYPE_UNKNOWN;
}

const __FlashStringHelper *MFRC522::PICC_GetTypeName(byte piccType) {
    switch (piccType) {
        case PICC_TYPE_ISO_14443_4: return F("ISO/IEC 14443-4");
        case PICC_TYPE_ISO_18092: return F("ISO/IEC 18092 (NFC)");
        case PICC_TYPE_MIFARE_MINI: return F("MIFARE Mini");
        case PICC_TYPE_MIFARE_1K: return F("MIFARE 1KB");
        case PICC_TYPE_MIFARE_4K: return F("MIFARE 4KB");
        case PICC_TYPE_MIFARE_UL: return F("MIFARE Ultralight");
        case PICC_TYPE_MIFARE_PLUS: return F("MIFARE Plus");
        case PICC_TYPE_TNP3XXX: return F("MIFARE TNP3XXX");
        case PICC_TYPE_NOT_COMPLETE: return F("UID not complete");
        default: return F("Unknown");
    }
}

bool MFRC522::PICC_IsNewCardPresent() {
    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);
    byte result = PICC_RequestA(bufferATQA, &bufferSize);
    return (result == STATUS_OK || result == STATUS_COLLISION);
}

bool MFRC522::PICC_ReadCardSerial() {
    return (PICC_Select(&uid) == STATUS_OK);
}
