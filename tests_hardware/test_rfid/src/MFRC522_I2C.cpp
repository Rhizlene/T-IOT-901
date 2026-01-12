/*
 * MFRC522_I2C.cpp - Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS I2C
 * BY AROZCAN
 * Based on MFRC522.cpp - Based on ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI Library BY COOQROBOT.
 * Author: arozcan @ https://github.com/arozcan/MFRC522-I2C-Library
 * Released into the public domain.
 */

#include "MFRC522_I2C.h"
#include <Arduino.h>
#include <Wire.h>

MFRC522::MFRC522(byte chipAddress) {
    _chipAddress = chipAddress;
}

void MFRC522::PCD_WriteRegister(byte reg, byte value) {
    Wire.beginTransmission(_chipAddress);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void MFRC522::PCD_WriteRegister(byte reg, byte count, byte *values) {
    Wire.beginTransmission(_chipAddress);
    Wire.write(reg);
    for (byte index = 0; index < count; index++) {
        Wire.write(values[index]);
    }
    Wire.endTransmission();
}

byte MFRC522::PCD_ReadRegister(byte reg) {
    byte value;
    Wire.beginTransmission(_chipAddress);
    Wire.write(reg);
    Wire.endTransmission();
    const uint8_t sz = 1;
    Wire.requestFrom(_chipAddress, sz);
    value = Wire.read();
    return value;
}

void MFRC522::PCD_ReadRegister(byte reg, byte count, byte *values, byte rxAlign) {
    if (count == 0) {
        return;
    }
    byte address = reg;
    byte index   = 0;
    Wire.beginTransmission(_chipAddress);
    Wire.write(address);
    Wire.endTransmission();
    Wire.requestFrom(_chipAddress, count);
    while (Wire.available()) {
        if (index == 0 && rxAlign) {
            byte mask = 0;
            for (byte i = rxAlign; i <= 7; i++) {
                mask |= (1 << i);
            }
            byte value = Wire.read();
            values[0] = (values[index] & ~mask) | (value & mask);
        } else {
            values[index] = Wire.read();
        }
        index++;
    }
}

void MFRC522::PCD_SetRegisterBitMask(byte reg, byte mask) {
    byte tmp;
    tmp = PCD_ReadRegister(reg);
    PCD_WriteRegister(reg, tmp | mask);
}

void MFRC522::PCD_ClearRegisterBitMask(byte reg, byte mask) {
    byte tmp;
    tmp = PCD_ReadRegister(reg);
    PCD_WriteRegister(reg, tmp & (~mask));
}

byte MFRC522::PCD_CalculateCRC(byte *data, byte length, byte *result) {
    PCD_WriteRegister(CommandReg, PCD_Idle);
    PCD_WriteRegister(DivIrqReg, 0x04);
    PCD_SetRegisterBitMask(FIFOLevelReg, 0x80);
    PCD_WriteRegister(FIFODataReg, length, data);
    PCD_WriteRegister(CommandReg, PCD_CalcCRC);

    word i = 5000;
    byte n;
    while (1) {
        n = PCD_ReadRegister(DivIrqReg);
        if (n & 0x04) {
            break;
        }
        if (--i == 0) {
            return STATUS_TIMEOUT;
        }
    }

    PCD_WriteRegister(CommandReg, PCD_Idle);
    result[0] = PCD_ReadRegister(CRCResultRegL);
    result[1] = PCD_ReadRegister(CRCResultRegH);
    return STATUS_OK;
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
    while (PCD_ReadRegister(CommandReg) & (1 << 4)) {
    }
}

void MFRC522::PCD_AntennaOn() {
    byte value = PCD_ReadRegister(TxControlReg);
    if ((value & 0x03) != 0x03) {
        PCD_WriteRegister(TxControlReg, value | 0x03);
    }
}

void MFRC522::PCD_AntennaOff() {
    PCD_ClearRegisterBitMask(TxControlReg, 0x03);
}

byte MFRC522::PCD_GetAntennaGain() {
    return PCD_ReadRegister(RFCfgReg) & (0x07 << 4);
}

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

    word i;
    byte n;
    for (i = 0; i < 0xFF; i++) {
        n = PCD_ReadRegister(DivIrqReg);
        if (n & 0x04) {
            break;
        }
    }
    PCD_WriteRegister(CommandReg, PCD_Idle);

    byte result[64];
    PCD_ReadRegister(FIFODataReg, 64, result, 0);

    PCD_WriteRegister(AutoTestReg, 0x00);

    byte version = PCD_ReadRegister(VersionReg);

    const byte *reference;
    switch (version) {
        case 0x88:
            reference = FM17522_firmware_reference;
            break;
        case 0x90:
            reference = MFRC522_firmware_referenceV0_0;
            break;
        case 0x91:
            reference = MFRC522_firmware_referenceV1_0;
            break;
        case 0x92:
            reference = MFRC522_firmware_referenceV2_0;
            break;
        default:
            return false;
    }

    for (i = 0; i < 64; i++) {
        if (result[i] != pgm_read_byte(&(reference[i]))) {
            return false;
        }
    }

    return true;
}

byte MFRC522::PCD_TransceiveData(byte *sendData, byte sendLen, byte *backData,
                                 byte *backLen, byte *validBits, byte rxAlign,
                                 bool checkCRC) {
    byte waitIRq = 0x30;
    return PCD_CommunicateWithPICC(PCD_Transceive, waitIRq, sendData, sendLen,
                                   backData, backLen, validBits, rxAlign, checkCRC);
}

byte MFRC522::PCD_CommunicateWithPICC(byte command, byte waitIRq, byte *sendData,
                                      byte sendLen, byte *backData, byte *backLen,
                                      byte *validBits, byte rxAlign, bool checkCRC) {
    byte n, _validBits;
    unsigned int i;

    byte txLastBits = validBits ? *validBits : 0;
    byte bitFraming = (rxAlign << 4) + txLastBits;

    PCD_WriteRegister(CommandReg, PCD_Idle);
    PCD_WriteRegister(ComIrqReg, 0x7F);
    PCD_SetRegisterBitMask(FIFOLevelReg, 0x80);
    PCD_WriteRegister(FIFODataReg, sendLen, sendData);
    PCD_WriteRegister(BitFramingReg, bitFraming);
    PCD_WriteRegister(CommandReg, command);
    if (command == PCD_Transceive) {
        PCD_SetRegisterBitMask(BitFramingReg, 0x80);
    }

    i = 2000;
    while (1) {
        n = PCD_ReadRegister(ComIrqReg);
        if (n & waitIRq) {
            break;
        }
        if (n & 0x01) {
            return STATUS_TIMEOUT;
        }
        if (--i == 0) {
            return STATUS_TIMEOUT;
        }
    }

    byte errorRegValue = PCD_ReadRegister(ErrorReg);
    if (errorRegValue & 0x13) {
        return STATUS_ERROR;
    }

    if (backData && backLen) {
        n = PCD_ReadRegister(FIFOLevelReg);
        if (n > *backLen) {
            return STATUS_NO_ROOM;
        }
        *backLen = n;
        PCD_ReadRegister(FIFODataReg, n, backData, rxAlign);
        _validBits = PCD_ReadRegister(ControlReg) & 0x07;
        if (validBits) {
            *validBits = _validBits;
        }
    }

    if (errorRegValue & 0x08) {
        return STATUS_COLLISION;
    }

    if (backData && backLen && checkCRC) {
        if (*backLen == 1 && _validBits == 4) {
            return STATUS_MIFARE_NACK;
        }
        if (*backLen < 2 || _validBits != 0) {
            return STATUS_CRC_WRONG;
        }
        byte controlBuffer[2];
        n = PCD_CalculateCRC(&backData[0], *backLen - 2, &controlBuffer[0]);
        if (n != STATUS_OK) {
            return n;
        }
        if ((backData[*backLen - 2] != controlBuffer[0]) ||
            (backData[*backLen - 1] != controlBuffer[1])) {
            return STATUS_CRC_WRONG;
        }
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
    byte validBits;
    byte status;

    if (bufferATQA == NULL || *bufferSize < 2) {
        return STATUS_NO_ROOM;
    }
    PCD_ClearRegisterBitMask(CollReg, 0x80);
    validBits = 7;
    status = PCD_TransceiveData(&command, 1, bufferATQA, bufferSize, &validBits);
    if (status != STATUS_OK) {
        return status;
    }
    if (*bufferSize != 2 || validBits != 0) {
        return STATUS_ERROR;
    }
    return STATUS_OK;
}

byte MFRC522::PICC_Select(Uid *uid, byte validBits) {
    bool uidComplete;
    bool selectDone;
    bool useCascadeTag;
    byte cascadeLevel = 1;
    byte result;
    byte count;
    byte index;
    byte uidIndex;
    int8_t currentLevelKnownBits;
    byte buffer[9];
    byte bufferUsed;
    byte rxAlign;
    byte txLastBits;
    byte *responseBuffer;
    byte responseLength;

    if (validBits > 80) {
        return STATUS_INVALID;
    }

    PCD_ClearRegisterBitMask(CollReg, 0x80);

    uidComplete = false;
    while (!uidComplete) {
        switch (cascadeLevel) {
            case 1:
                buffer[0]     = PICC_CMD_SEL_CL1;
                uidIndex      = 0;
                useCascadeTag = validBits && uid->size > 4;
                break;

            case 2:
                buffer[0]     = PICC_CMD_SEL_CL2;
                uidIndex      = 3;
                useCascadeTag = validBits && uid->size > 7;
                break;

            case 3:
                buffer[0]     = PICC_CMD_SEL_CL3;
                uidIndex      = 6;
                useCascadeTag = false;
                break;

            default:
                return STATUS_INTERNAL_ERROR;
                break;
        }

        currentLevelKnownBits = validBits - (8 * uidIndex);
        if (currentLevelKnownBits < 0) {
            currentLevelKnownBits = 0;
        }
        index = 2;
        if (useCascadeTag) {
            buffer[index++] = PICC_CMD_CT;
        }
        byte bytesToCopy = currentLevelKnownBits / 8 + (currentLevelKnownBits % 8 ? 1 : 0);
        if (bytesToCopy) {
            byte maxBytes = useCascadeTag ? 3 : 4;
            if (bytesToCopy > maxBytes) {
                bytesToCopy = maxBytes;
            }
            for (count = 0; count < bytesToCopy; count++) {
                buffer[index++] = uid->uidByte[uidIndex + count];
            }
        }
        if (useCascadeTag) {
            currentLevelKnownBits += 8;
        }

        selectDone = false;
        while (!selectDone) {
            if (currentLevelKnownBits >= 32) {
                buffer[1] = 0x70;
                buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
                result = PCD_CalculateCRC(buffer, 7, &buffer[7]);
                if (result != STATUS_OK) {
                    return result;
                }
                txLastBits = 0;
                bufferUsed = 9;
                responseBuffer = &buffer[6];
                responseLength = 3;
            } else {
                txLastBits = currentLevelKnownBits % 8;
                count      = currentLevelKnownBits / 8;
                index      = 2 + count;
                buffer[1]  = (index << 4) + txLastBits;
                bufferUsed = index + (txLastBits ? 1 : 0);
                responseBuffer = &buffer[index];
                responseLength = sizeof(buffer) - index;
            }

            rxAlign = txLastBits;
            PCD_WriteRegister(BitFramingReg, (rxAlign << 4) + txLastBits);

            result = PCD_TransceiveData(buffer, bufferUsed, responseBuffer,
                                        &responseLength, &txLastBits, rxAlign);
            if (result == STATUS_COLLISION) {
                result = PCD_ReadRegister(CollReg);
                if (result & 0x20) {
                    return STATUS_COLLISION;
                }
                byte collisionPos = result & 0x1F;
                if (collisionPos == 0) {
                    collisionPos = 32;
                }
                if (collisionPos <= currentLevelKnownBits) {
                    return STATUS_INTERNAL_ERROR;
                }
                currentLevelKnownBits = collisionPos;
                count = (currentLevelKnownBits - 1) % 8;
                index = 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0);
                buffer[index] |= (1 << count);
            } else if (result != STATUS_OK) {
                return result;
            } else {
                if (currentLevelKnownBits >= 32) {
                    selectDone = true;
                } else {
                    currentLevelKnownBits = 32;
                }
            }
        }

        index = (buffer[2] == PICC_CMD_CT) ? 3 : 2;
        bytesToCopy = (buffer[2] == PICC_CMD_CT) ? 3 : 4;
        for (count = 0; count < bytesToCopy; count++) {
            uid->uidByte[uidIndex + count] = buffer[index++];
        }

        if (responseLength != 3 || txLastBits != 0) {
            return STATUS_ERROR;
        }
        result = PCD_CalculateCRC(responseBuffer, 1, &buffer[2]);
        if (result != STATUS_OK) {
            return result;
        }
        if ((buffer[2] != responseBuffer[1]) || (buffer[3] != responseBuffer[2])) {
            return STATUS_CRC_WRONG;
        }
        if (responseBuffer[0] & 0x04) {
            cascadeLevel++;
        } else {
            uidComplete = true;
            uid->sak    = responseBuffer[0];
        }
    }

    uid->size = 3 * cascadeLevel + 1;

    return STATUS_OK;
}

byte MFRC522::PICC_HaltA() {
    byte result;
    byte buffer[4];

    buffer[0] = PICC_CMD_HLTA;
    buffer[1] = 0;
    result = PCD_CalculateCRC(buffer, 2, &buffer[2]);
    if (result != STATUS_OK) {
        return result;
    }

    result = PCD_TransceiveData(buffer, sizeof(buffer), NULL, 0);
    if (result == STATUS_TIMEOUT) {
        return STATUS_OK;
    }
    if (result == STATUS_OK) {
        return STATUS_ERROR;
    }
    return result;
}

byte MFRC522::PCD_Authenticate(byte command, byte blockAddr, MIFARE_Key *key, Uid *uid) {
    byte waitIRq = 0x10;

    byte sendData[12];
    sendData[0] = command;
    sendData[1] = blockAddr;
    for (byte i = 0; i < MF_KEY_SIZE; i++) {
        sendData[2 + i] = key->keyByte[i];
    }
    for (byte i = 0; i < 4; i++) {
        sendData[8 + i] = uid->uidByte[i];
    }

    return PCD_CommunicateWithPICC(PCD_MFAuthent, waitIRq, &sendData[0], sizeof(sendData));
}

void MFRC522::PCD_StopCrypto1() {
    PCD_ClearRegisterBitMask(Status2Reg, 0x08);
}

byte MFRC522::MIFARE_Read(byte blockAddr, byte *buffer, byte *bufferSize) {
    byte result;

    if (buffer == NULL || *bufferSize < 18) {
        return STATUS_NO_ROOM;
    }

    buffer[0] = PICC_CMD_MF_READ;
    buffer[1] = blockAddr;
    result = PCD_CalculateCRC(buffer, 2, &buffer[2]);
    if (result != STATUS_OK) {
        return result;
    }

    return PCD_TransceiveData(buffer, 4, buffer, bufferSize, NULL, 0, true);
}

byte MFRC522::MIFARE_Write(byte blockAddr, byte *buffer, byte bufferSize) {
    byte result;

    if (buffer == NULL || bufferSize < 16) {
        return STATUS_INVALID;
    }

    byte cmdBuffer[2];
    cmdBuffer[0] = PICC_CMD_MF_WRITE;
    cmdBuffer[1] = blockAddr;
    result = PCD_MIFARE_Transceive(cmdBuffer, 2);
    if (result != STATUS_OK) {
        return result;
    }

    result = PCD_MIFARE_Transceive(buffer, bufferSize);
    if (result != STATUS_OK) {
        return result;
    }

    return STATUS_OK;
}

byte MFRC522::MIFARE_Decrement(byte blockAddr, long delta) {
    return MIFARE_TwoStepHelper(PICC_CMD_MF_DECREMENT, blockAddr, delta);
}

byte MFRC522::MIFARE_Increment(byte blockAddr, long delta) {
    return MIFARE_TwoStepHelper(PICC_CMD_MF_INCREMENT, blockAddr, delta);
}

byte MFRC522::MIFARE_Restore(byte blockAddr) {
    return MIFARE_TwoStepHelper(PICC_CMD_MF_RESTORE, blockAddr, 0L);
}

byte MFRC522::MIFARE_TwoStepHelper(byte command, byte blockAddr, long data) {
    byte result;
    byte cmdBuffer[2];

    cmdBuffer[0] = command;
    cmdBuffer[1] = blockAddr;
    result = PCD_MIFARE_Transceive(cmdBuffer, 2);
    if (result != STATUS_OK) {
        return result;
    }

    result = PCD_MIFARE_Transceive((byte *)&data, 4, true);
    if (result != STATUS_OK) {
        return result;
    }

    return STATUS_OK;
}

byte MFRC522::MIFARE_Transfer(byte blockAddr) {
    byte result;
    byte cmdBuffer[2];

    cmdBuffer[0] = PICC_CMD_MF_TRANSFER;
    cmdBuffer[1] = blockAddr;
    result = PCD_MIFARE_Transceive(cmdBuffer, 2);
    if (result != STATUS_OK) {
        return result;
    }
    return STATUS_OK;
}

byte MFRC522::MIFARE_GetValue(byte blockAddr, long *value) {
    byte status;
    byte buffer[18];
    byte size = sizeof(buffer);

    status = MIFARE_Read(blockAddr, buffer, &size);
    if (status == STATUS_OK) {
        *value = (long(buffer[3]) << 24) | (long(buffer[2]) << 16) |
                 (long(buffer[1]) << 8) | long(buffer[0]);
    }
    return status;
}

byte MFRC522::MIFARE_SetValue(byte blockAddr, long value) {
    byte buffer[18];

    buffer[0] = buffer[8] = (value & 0xFF);
    buffer[1] = buffer[9] = (value & 0xFF00) >> 8;
    buffer[2] = buffer[10] = (value & 0xFF0000) >> 16;
    buffer[3] = buffer[11] = (value & 0xFF000000) >> 24;
    buffer[4] = ~buffer[0];
    buffer[5] = ~buffer[1];
    buffer[6] = ~buffer[2];
    buffer[7] = ~buffer[3];
    buffer[12] = buffer[14] = blockAddr;
    buffer[13] = buffer[15] = ~blockAddr;

    return MIFARE_Write(blockAddr, buffer, 16);
}

byte MFRC522::MIFARE_Ultralight_Write(byte page, byte *buffer, byte bufferSize) {
    byte result;

    if (buffer == NULL || bufferSize < 4) {
        return STATUS_INVALID;
    }

    byte cmdBuffer[6];
    cmdBuffer[0] = PICC_CMD_UL_WRITE;
    cmdBuffer[1] = page;
    memcpy(&cmdBuffer[2], buffer, 4);

    result = PCD_MIFARE_Transceive(cmdBuffer, 6);
    if (result != STATUS_OK) {
        return result;
    }
    return STATUS_OK;
}

byte MFRC522::PCD_MIFARE_Transceive(byte *sendData, byte sendLen, bool acceptTimeout) {
    byte result;
    byte cmdBuffer[18];

    if (sendData == NULL || sendLen > 16) {
        return STATUS_INVALID;
    }

    memcpy(cmdBuffer, sendData, sendLen);
    result = PCD_CalculateCRC(cmdBuffer, sendLen, &cmdBuffer[sendLen]);
    if (result != STATUS_OK) {
        return result;
    }
    sendLen += 2;

    byte waitIRq = 0x30;
    byte cmdBufferSize = sizeof(cmdBuffer);
    byte validBits = 0;
    result = PCD_CommunicateWithPICC(PCD_Transceive, waitIRq, cmdBuffer, sendLen,
                                     cmdBuffer, &cmdBufferSize, &validBits);
    if (acceptTimeout && result == STATUS_TIMEOUT) {
        return STATUS_OK;
    }
    if (result != STATUS_OK) {
        return result;
    }
    if (cmdBufferSize != 1 || validBits != 4) {
        return STATUS_ERROR;
    }
    if (cmdBuffer[0] != MF_ACK) {
        return STATUS_MIFARE_NACK;
    }
    return STATUS_OK;
}

const __FlashStringHelper *MFRC522::GetStatusCodeName(byte code) {
    switch (code) {
        case STATUS_OK:
            return F("Success.");
        case STATUS_ERROR:
            return F("Error in communication.");
        case STATUS_COLLISION:
            return F("Collision detected.");
        case STATUS_TIMEOUT:
            return F("Timeout in communication.");
        case STATUS_NO_ROOM:
            return F("A buffer is not big enough.");
        case STATUS_INTERNAL_ERROR:
            return F("Internal error in the code.");
        case STATUS_INVALID:
            return F("Invalid argument.");
        case STATUS_CRC_WRONG:
            return F("The CRC_A does not match.");
        case STATUS_MIFARE_NACK:
            return F("A MIFARE PICC responded with NAK.");
        default:
            return F("Unknown error");
    }
}

byte MFRC522::PICC_GetType(byte sak) {
    if (sak & 0x04) {
        return PICC_TYPE_NOT_COMPLETE;
    }

    switch (sak) {
        case 0x09:
            return PICC_TYPE_MIFARE_MINI;
        case 0x08:
            return PICC_TYPE_MIFARE_1K;
        case 0x18:
            return PICC_TYPE_MIFARE_4K;
        case 0x00:
            return PICC_TYPE_MIFARE_UL;
        case 0x10:
        case 0x11:
            return PICC_TYPE_MIFARE_PLUS;
        case 0x01:
            return PICC_TYPE_TNP3XXX;
        default:
            break;
    }

    if (sak & 0x20) {
        return PICC_TYPE_ISO_14443_4;
    }

    if (sak & 0x40) {
        return PICC_TYPE_ISO_18092;
    }

    return PICC_TYPE_UNKNOWN;
}

const __FlashStringHelper *MFRC522::PICC_GetTypeName(byte piccType) {
    switch (piccType) {
        case PICC_TYPE_ISO_14443_4:
            return F("PICC compliant with ISO/IEC 14443-4");
        case PICC_TYPE_ISO_18092:
            return F("PICC compliant with ISO/IEC 18092 (NFC)");
        case PICC_TYPE_MIFARE_MINI:
            return F("MIFARE Mini, 320 bytes");
        case PICC_TYPE_MIFARE_1K:
            return F("MIFARE 1KB");
        case PICC_TYPE_MIFARE_4K:
            return F("MIFARE 4KB");
        case PICC_TYPE_MIFARE_UL:
            return F("MIFARE Ultralight or Ultralight C");
        case PICC_TYPE_MIFARE_PLUS:
            return F("MIFARE Plus");
        case PICC_TYPE_TNP3XXX:
            return F("MIFARE TNP3XXX");
        case PICC_TYPE_NOT_COMPLETE:
            return F("SAK indicates UID is not complete.");
        case PICC_TYPE_UNKNOWN:
        default:
            return F("Unknown type");
    }
}

void MFRC522::PICC_DumpToSerial(Uid *uid) {
    MIFARE_Key key;

    Serial.print(F("Card UID:"));
    for (byte i = 0; i < uid->size; i++) {
        if (uid->uidByte[i] < 0x10)
            Serial.print(F(" 0"));
        else
            Serial.print(F(" "));
        Serial.print(uid->uidByte[i], HEX);
    }
    Serial.println();

    byte piccType = PICC_GetType(uid->sak);
    Serial.print(F("PICC type: "));
    Serial.println(PICC_GetTypeName(piccType));

    switch (piccType) {
        case PICC_TYPE_MIFARE_MINI:
        case PICC_TYPE_MIFARE_1K:
        case PICC_TYPE_MIFARE_4K:
            for (byte i = 0; i < 6; i++) {
                key.keyByte[i] = 0xFF;
            }
            PICC_DumpMifareClassicToSerial(uid, piccType, &key);
            break;

        case PICC_TYPE_MIFARE_UL:
            PICC_DumpMifareUltralightToSerial();
            break;

        case PICC_TYPE_ISO_14443_4:
        case PICC_TYPE_ISO_18092:
        case PICC_TYPE_MIFARE_PLUS:
        case PICC_TYPE_TNP3XXX:
            Serial.println(F("Dumping memory contents not implemented for that PICC type."));
            break;

        case PICC_TYPE_UNKNOWN:
        case PICC_TYPE_NOT_COMPLETE:
        default:
            break;
    }

    Serial.println();
    PICC_HaltA();
}

void MFRC522::PICC_DumpMifareClassicToSerial(Uid *uid, byte piccType, MIFARE_Key *key) {
    byte no_of_sectors = 0;
    switch (piccType) {
        case PICC_TYPE_MIFARE_MINI:
            no_of_sectors = 5;
            break;
        case PICC_TYPE_MIFARE_1K:
            no_of_sectors = 16;
            break;
        case PICC_TYPE_MIFARE_4K:
            no_of_sectors = 40;
            break;
        default:
            break;
    }

    if (no_of_sectors) {
        Serial.println(F("Sector Block   0  1  2  3   4  5  6  7   8  9 10 11  12 13 14 15  AccessBits"));
        for (int8_t i = no_of_sectors - 1; i >= 0; i--) {
            PICC_DumpMifareClassicSectorToSerial(uid, key, i);
        }
    }
    PICC_HaltA();
    PCD_StopCrypto1();
}

void MFRC522::PICC_DumpMifareClassicSectorToSerial(Uid *uid, MIFARE_Key *key, byte sector) {
    byte status;
    byte firstBlock;
    byte no_of_blocks;
    bool isSectorTrailer;

    byte c1, c2, c3;
    byte c1_, c2_, c3_;
    bool invertedError;
    byte g[4];
    byte group;
    bool firstInGroup;

    if (sector < 32) {
        no_of_blocks = 4;
        firstBlock = sector * no_of_blocks;
    } else if (sector < 40) {
        no_of_blocks = 16;
        firstBlock = 128 + (sector - 32) * no_of_blocks;
    } else {
        return;
    }

    byte byteCount;
    byte buffer[18];
    byte blockAddr;
    isSectorTrailer = true;
    for (int8_t blockOffset = no_of_blocks - 1; blockOffset >= 0; blockOffset--) {
        blockAddr = firstBlock + blockOffset;
        if (isSectorTrailer) {
            if (sector < 10)
                Serial.print(F("   "));
            else
                Serial.print(F("  "));
            Serial.print(sector);
            Serial.print(F("   "));
        } else {
            Serial.print(F("       "));
        }
        if (blockAddr < 10)
            Serial.print(F("   "));
        else {
            if (blockAddr < 100)
                Serial.print(F("  "));
            else
                Serial.print(F(" "));
        }
        Serial.print(blockAddr);
        Serial.print(F("  "));
        if (isSectorTrailer) {
            status = PCD_Authenticate(PICC_CMD_MF_AUTH_KEY_A, firstBlock, key, uid);
            if (status != STATUS_OK) {
                Serial.print(F("PCD_Authenticate() failed: "));
                Serial.println(GetStatusCodeName(status));
                return;
            }
        }
        byteCount = sizeof(buffer);
        status = MIFARE_Read(blockAddr, buffer, &byteCount);
        if (status != STATUS_OK) {
            Serial.print(F("MIFARE_Read() failed: "));
            Serial.println(GetStatusCodeName(status));
            continue;
        }
        for (byte index = 0; index < 16; index++) {
            if (buffer[index] < 0x10)
                Serial.print(F(" 0"));
            else
                Serial.print(F(" "));
            Serial.print(buffer[index], HEX);
            if ((index % 4) == 3) {
                Serial.print(F(" "));
            }
        }
        if (isSectorTrailer) {
            c1 = buffer[7] >> 4;
            c2 = buffer[8] & 0xF;
            c3 = buffer[8] >> 4;
            c1_ = buffer[6] & 0xF;
            c2_ = buffer[6] >> 4;
            c3_ = buffer[7] & 0xF;
            invertedError = (c1 != (~c1_ & 0xF)) || (c2 != (~c2_ & 0xF)) || (c3 != (~c3_ & 0xF));
            g[0] = ((c1 & 1) << 2) | ((c2 & 1) << 1) | ((c3 & 1) << 0);
            g[1] = ((c1 & 2) << 1) | ((c2 & 2) << 0) | ((c3 & 2) >> 1);
            g[2] = ((c1 & 4) << 0) | ((c2 & 4) >> 1) | ((c3 & 4) >> 2);
            g[3] = ((c1 & 8) >> 1) | ((c2 & 8) >> 2) | ((c3 & 8) >> 3);
            isSectorTrailer = false;
        }

        if (no_of_blocks == 4) {
            group = blockOffset;
            firstInGroup = true;
        } else {
            group = blockOffset / 5;
            firstInGroup = (group == 3) || (group != (blockOffset + 1) / 5);
        }

        if (firstInGroup) {
            Serial.print(F(" [ "));
            Serial.print((g[group] >> 2) & 1, DEC);
            Serial.print(F(" "));
            Serial.print((g[group] >> 1) & 1, DEC);
            Serial.print(F(" "));
            Serial.print((g[group] >> 0) & 1, DEC);
            Serial.print(F(" ] "));
            if (invertedError) {
                Serial.print(F(" Inverted access bits did not match! "));
            }
        }

        if (group != 3 && (g[group] == 1 || g[group] == 6)) {
            long value = (long(buffer[3]) << 24) | (long(buffer[2]) << 16) |
                         (long(buffer[1]) << 8) | long(buffer[0]);
            Serial.print(F(" Value=0x"));
            Serial.print(value, HEX);
            Serial.print(F(" Adr=0x"));
            Serial.print(buffer[12], HEX);
        }
        Serial.println();
    }

    return;
}

void MFRC522::PICC_DumpMifareUltralightToSerial() {
    byte status;
    byte byteCount;
    byte buffer[18];
    byte i;

    Serial.println(F("Page  0  1  2  3"));
    for (byte page = 0; page < 16; page += 4) {
        byteCount = sizeof(buffer);
        status = MIFARE_Read(page, buffer, &byteCount);
        if (status != STATUS_OK) {
            Serial.print(F("MIFARE_Read() failed: "));
            Serial.println(GetStatusCodeName(status));
            break;
        }
        for (byte offset = 0; offset < 4; offset++) {
            i = page + offset;
            if (i < 10)
                Serial.print(F("  "));
            else
                Serial.print(F(" "));
            Serial.print(i);
            Serial.print(F("  "));
            for (byte index = 0; index < 4; index++) {
                i = 4 * offset + index;
                if (buffer[i] < 0x10)
                    Serial.print(F(" 0"));
                else
                    Serial.print(F(" "));
                Serial.print(buffer[i], HEX);
            }
            Serial.println();
        }
    }
}

void MFRC522::MIFARE_SetAccessBits(byte *accessBitBuffer, byte g0, byte g1, byte g2, byte g3) {
    byte c1 = ((g3 & 4) << 1) | ((g2 & 4) << 0) | ((g1 & 4) >> 1) | ((g0 & 4) >> 2);
    byte c2 = ((g3 & 2) << 2) | ((g2 & 2) << 1) | ((g1 & 2) << 0) | ((g0 & 2) >> 1);
    byte c3 = ((g3 & 1) << 3) | ((g2 & 1) << 2) | ((g1 & 1) << 1) | ((g0 & 1) << 0);

    accessBitBuffer[0] = (~c2 & 0xF) << 4 | (~c1 & 0xF);
    accessBitBuffer[1] = c1 << 4 | (~c3 & 0xF);
    accessBitBuffer[2] = c3 << 4 | c2;
}

bool MFRC522::MIFARE_OpenUidBackdoor(bool logErrors) {
    PICC_HaltA();

    byte cmd = 0x40;
    byte validBits = 7;
    byte response[32];
    byte received;
    byte status = PCD_TransceiveData(&cmd, (byte)1, response, &received, &validBits, (byte)0, false);
    if (status != STATUS_OK) {
        if (logErrors) {
            Serial.println(F("Card did not respond to 0x40 after HALT command."));
            Serial.print(F("Error name: "));
            Serial.println(GetStatusCodeName(status));
        }
        return false;
    }
    if (received != 1 || response[0] != 0x0A) {
        if (logErrors) {
            Serial.print(F("Got bad response on backdoor 0x40 command: "));
            Serial.print(response[0], HEX);
            Serial.print(F(" ("));
            Serial.print(validBits);
            Serial.print(F(" valid bits)\r\n"));
        }
        return false;
    }

    cmd = 0x43;
    validBits = 8;
    status = PCD_TransceiveData(&cmd, (byte)1, response, &received, &validBits, (byte)0, false);
    if (status != STATUS_OK) {
        if (logErrors) {
            Serial.println(F("Error in communication at command 0x43"));
            Serial.print(F("Error name: "));
            Serial.println(GetStatusCodeName(status));
        }
        return false;
    }
    if (received != 1 || response[0] != 0x0A) {
        if (logErrors) {
            Serial.print(F("Got bad response on backdoor 0x43 command: "));
            Serial.print(response[0], HEX);
            Serial.print(F(" ("));
            Serial.print(validBits);
            Serial.print(F(" valid bits)\r\n"));
        }
        return false;
    }

    return true;
}

bool MFRC522::MIFARE_SetUid(byte *newUid, byte uidSize, bool logErrors) {
    if (!newUid || !uidSize || uidSize > 15) {
        if (logErrors) {
            Serial.println(F("New UID buffer empty, size 0, or size > 15 given"));
        }
        return false;
    }

    MIFARE_Key key = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    byte status = PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (byte)1, &key, &uid);
    if (status != STATUS_OK) {
        if (status == STATUS_TIMEOUT) {
            if (!PICC_IsNewCardPresent() || !PICC_ReadCardSerial()) {
                Serial.println(F("No card was previously selected, and none are available."));
                return false;
            }

            status = PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (byte)1, &key, &uid);
            if (status != STATUS_OK) {
                if (logErrors) {
                    Serial.println(F("Failed to authenticate to card for reading."));
                    Serial.println(GetStatusCodeName(status));
                }
                return false;
            }
        } else {
            if (logErrors) {
                Serial.print(F("PCD_Authenticate() failed: "));
                Serial.println(GetStatusCodeName(status));
            }
            return false;
        }
    }

    byte block0_buffer[18];
    byte byteCount = sizeof(block0_buffer);
    status = MIFARE_Read((byte)0, block0_buffer, &byteCount);
    if (status != STATUS_OK) {
        if (logErrors) {
            Serial.print(F("MIFARE_Read() failed: "));
            Serial.println(GetStatusCodeName(status));
            Serial.println(F("Are you sure your KEY A for sector 0 is 0xFFFFFFFFFFFF?"));
        }
        return false;
    }

    byte bcc = 0;
    for (int i = 0; i < uidSize; i++) {
        block0_buffer[i] = newUid[i];
        bcc ^= newUid[i];
    }
    block0_buffer[uidSize] = bcc;

    PCD_StopCrypto1();

    if (!MIFARE_OpenUidBackdoor(logErrors)) {
        if (logErrors) {
            Serial.println(F("Activating the UID backdoor failed."));
        }
        return false;
    }

    status = MIFARE_Write((byte)0, block0_buffer, (byte)16);
    if (status != STATUS_OK) {
        if (logErrors) {
            Serial.print(F("MIFARE_Write() failed: "));
            Serial.println(GetStatusCodeName(status));
        }
        return false;
    }

    byte atqa_answer[2];
    byte atqa_size = 2;
    PICC_WakeupA(atqa_answer, &atqa_size);

    return true;
}

bool MFRC522::MIFARE_UnbrickUidSector(bool logErrors) {
    MIFARE_OpenUidBackdoor(logErrors);

    byte block0_buffer[] = {0x01, 0x02, 0x03, 0x04, 0x04, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    byte status = MIFARE_Write((byte)0, block0_buffer, (byte)16);
    if (status != STATUS_OK) {
        if (logErrors) {
            Serial.print(F("MIFARE_Write() failed: "));
            Serial.println(GetStatusCodeName(status));
        }
        return false;
    }
    return true;
}

bool MFRC522::PICC_IsNewCardPresent() {
    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);
    byte result = PICC_RequestA(bufferATQA, &bufferSize);
    return (result == STATUS_OK || result == STATUS_COLLISION);
}

bool MFRC522::PICC_ReadCardSerial() {
    byte result = PICC_Select(&uid);
    return (result == STATUS_OK);
}
