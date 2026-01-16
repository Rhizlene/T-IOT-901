/**
 * MFRC522_I2C.h - Library for RFID MFRC522 via I2C
 * Author: arozcan @ https://github.com/arozcan/MFRC522-I2C-Library
 * Released into the public domain.
 */
#ifndef MFRC522_I2C_h
#define MFRC522_I2C_h

#include <Arduino.h>
#include <Wire.h>

// Firmware data for self-test
const byte MFRC522_firmware_referenceV0_0[] PROGMEM = {
    0x00, 0x87, 0x98, 0x0f, 0x49, 0xFF, 0x07, 0x19, 0xBF, 0x22, 0x30,
    0x49, 0x59, 0x63, 0xAD, 0xCA, 0x7F, 0xE3, 0x4E, 0x03, 0x5C, 0x4E,
    0x49, 0x50, 0x47, 0x9A, 0x37, 0x61, 0xE7, 0xE2, 0xC6, 0x2E, 0x75,
    0x5A, 0xED, 0x04, 0x3D, 0x02, 0x4B, 0x78, 0x32, 0xFF, 0x58, 0x3B,
    0x7C, 0xE9, 0x00, 0x94, 0xB4, 0x4A, 0x59, 0x5B, 0xFD, 0xC9, 0x29,
    0xDF, 0x35, 0x96, 0x98, 0x9E, 0x4F, 0x30, 0x32, 0x8D};
const byte MFRC522_firmware_referenceV1_0[] PROGMEM = {
    0x00, 0xC6, 0x37, 0xD5, 0x32, 0xB7, 0x57, 0x5C, 0xC2, 0xD8, 0x7C,
    0x4D, 0xD9, 0x70, 0xC7, 0x73, 0x10, 0xE6, 0xD2, 0xAA, 0x5E, 0xA1,
    0x3E, 0x5A, 0x14, 0xAF, 0x30, 0x61, 0xC9, 0x70, 0xDB, 0x2E, 0x64,
    0x22, 0x72, 0xB5, 0xBD, 0x65, 0xF4, 0xEC, 0x22, 0xBC, 0xD3, 0x72,
    0x35, 0xCD, 0xAA, 0x41, 0x1F, 0xA7, 0xF3, 0x53, 0x14, 0xDE, 0x7E,
    0x02, 0xD9, 0x0F, 0xB5, 0x5E, 0x25, 0x1D, 0x29, 0x79};
const byte MFRC522_firmware_referenceV2_0[] PROGMEM = {
    0x00, 0xEB, 0x66, 0xBA, 0x57, 0xBF, 0x23, 0x95, 0xD0, 0xE3, 0x0D,
    0x3D, 0x27, 0x89, 0x5C, 0xDE, 0x9D, 0x3B, 0xA7, 0x00, 0x21, 0x5B,
    0x89, 0x82, 0x51, 0x3A, 0xEB, 0x02, 0x0C, 0xA5, 0x00, 0x49, 0x7C,
    0x84, 0x4D, 0xB3, 0xCC, 0xD2, 0x1B, 0x81, 0x5D, 0x48, 0x76, 0xD5,
    0x71, 0x61, 0x21, 0xA9, 0x86, 0x96, 0x83, 0x38, 0xCF, 0x9D, 0x5B,
    0x6D, 0xDC, 0x15, 0xBA, 0x3E, 0x7D, 0x95, 0x3B, 0x2F};
const byte FM17522_firmware_reference[] PROGMEM = {
    0x00, 0xD6, 0x78, 0x8C, 0xE2, 0xAA, 0x0C, 0x18, 0x2A, 0xB8, 0x7A,
    0x7F, 0xD3, 0x6A, 0xCF, 0x0B, 0xB1, 0x37, 0x63, 0x4B, 0x69, 0xAE,
    0x91, 0xC7, 0xC3, 0x97, 0xAE, 0x77, 0xF4, 0x37, 0xD7, 0x9B, 0x7C,
    0xF5, 0x3C, 0x11, 0x8F, 0x15, 0xC3, 0xD7, 0xC1, 0x5B, 0x00, 0x2A,
    0xD0, 0x75, 0xDE, 0x9E, 0x51, 0x64, 0xAB, 0x3E, 0xE9, 0x15, 0xB5,
    0xAB, 0x56, 0x9A, 0x98, 0x82, 0x26, 0xEA, 0x2A, 0x62};

class MFRC522 {
   public:
    enum PCD_Register {
        CommandReg = 0x01, ComIEnReg = 0x02, DivIEnReg = 0x03,
        ComIrqReg = 0x04, DivIrqReg = 0x05, ErrorReg = 0x06,
        Status1Reg = 0x07, Status2Reg = 0x08, FIFODataReg = 0x09,
        FIFOLevelReg = 0x0A, WaterLevelReg = 0x0B, ControlReg = 0x0C,
        BitFramingReg = 0x0D, CollReg = 0x0E, ModeReg = 0x11,
        TxModeReg = 0x12, RxModeReg = 0x13, TxControlReg = 0x14,
        TxASKReg = 0x15, TxSelReg = 0x16, RxSelReg = 0x17,
        RxThresholdReg = 0x18, DemodReg = 0x19, MfTxReg = 0x1C,
        MfRxReg = 0x1D, SerialSpeedReg = 0x1F, CRCResultRegH = 0x21,
        CRCResultRegL = 0x22, ModWidthReg = 0x24, RFCfgReg = 0x26,
        GsNReg = 0x27, CWGsPReg = 0x28, ModGsPReg = 0x29,
        TModeReg = 0x2A, TPrescalerReg = 0x2B, TReloadRegH = 0x2C,
        TReloadRegL = 0x2D, TCounterValueRegH = 0x2E, TCounterValueRegL = 0x2F,
        TestSel1Reg = 0x31, TestSel2Reg = 0x32, TestPinEnReg = 0x33,
        TestPinValueReg = 0x34, TestBusReg = 0x35, AutoTestReg = 0x36,
        VersionReg = 0x37, AnalogTestReg = 0x38, TestDAC1Reg = 0x39,
        TestDAC2Reg = 0x3A, TestADCReg = 0x3B
    };

    enum PCD_Command {
        PCD_Idle = 0x00, PCD_Mem = 0x01, PCD_GenerateRandomID = 0x02,
        PCD_CalcCRC = 0x03, PCD_Transmit = 0x04, PCD_NoCmdChange = 0x07,
        PCD_Receive = 0x08, PCD_Transceive = 0x0C, PCD_MFAuthent = 0x0E,
        PCD_SoftReset = 0x0F
    };

    enum PCD_RxGain {
        RxGain_18dB = 0x00 << 4, RxGain_23dB = 0x01 << 4,
        RxGain_33dB = 0x04 << 4, RxGain_38dB = 0x05 << 4,
        RxGain_43dB = 0x06 << 4, RxGain_48dB = 0x07 << 4,
        RxGain_min = 0x00 << 4, RxGain_avg = 0x04 << 4, RxGain_max = 0x07 << 4
    };

    enum PICC_Command {
        PICC_CMD_REQA = 0x26, PICC_CMD_WUPA = 0x52, PICC_CMD_CT = 0x88,
        PICC_CMD_SEL_CL1 = 0x93, PICC_CMD_SEL_CL2 = 0x95, PICC_CMD_SEL_CL3 = 0x97,
        PICC_CMD_HLTA = 0x50, PICC_CMD_MF_AUTH_KEY_A = 0x60,
        PICC_CMD_MF_AUTH_KEY_B = 0x61, PICC_CMD_MF_READ = 0x30,
        PICC_CMD_MF_WRITE = 0xA0, PICC_CMD_MF_DECREMENT = 0xC0,
        PICC_CMD_MF_INCREMENT = 0xC1, PICC_CMD_MF_RESTORE = 0xC2,
        PICC_CMD_MF_TRANSFER = 0xB0, PICC_CMD_UL_WRITE = 0xA2
    };

    enum MIFARE_Misc { MF_ACK = 0xA, MF_KEY_SIZE = 6 };

    enum PICC_Type {
        PICC_TYPE_UNKNOWN = 0, PICC_TYPE_ISO_14443_4 = 1,
        PICC_TYPE_ISO_18092 = 2, PICC_TYPE_MIFARE_MINI = 3,
        PICC_TYPE_MIFARE_1K = 4, PICC_TYPE_MIFARE_4K = 5,
        PICC_TYPE_MIFARE_UL = 6, PICC_TYPE_MIFARE_PLUS = 7,
        PICC_TYPE_TNP3XXX = 8, PICC_TYPE_NOT_COMPLETE = 255
    };

    enum StatusCode {
        STATUS_OK = 1, STATUS_ERROR = 2, STATUS_COLLISION = 3,
        STATUS_TIMEOUT = 4, STATUS_NO_ROOM = 5, STATUS_INTERNAL_ERROR = 6,
        STATUS_INVALID = 7, STATUS_CRC_WRONG = 8, STATUS_MIFARE_NACK = 9
    };

    typedef struct { byte size; byte uidByte[10]; byte sak; } Uid;
    typedef struct { byte keyByte[MF_KEY_SIZE]; } MIFARE_Key;

    Uid uid;
    static const byte FIFO_SIZE = 64;

    MFRC522(byte chipAddress);
    void PCD_WriteRegister(byte reg, byte value);
    void PCD_WriteRegister(byte reg, byte count, byte *values);
    byte PCD_ReadRegister(byte reg);
    void PCD_ReadRegister(byte reg, byte count, byte *values, byte rxAlign = 0);
    void PCD_SetRegisterBitMask(byte reg, byte mask);
    void PCD_ClearRegisterBitMask(byte reg, byte mask);
    byte PCD_CalculateCRC(byte *data, byte length, byte *result);
    void PCD_Init();
    void PCD_Reset();
    void PCD_AntennaOn();
    void PCD_AntennaOff();
    byte PCD_GetAntennaGain();
    void PCD_SetAntennaGain(byte mask);
    bool PCD_PerformSelfTest();
    byte PCD_TransceiveData(byte *sendData, byte sendLen, byte *backData,
                            byte *backLen, byte *validBits = NULL,
                            byte rxAlign = 0, bool checkCRC = false);
    byte PCD_CommunicateWithPICC(byte command, byte waitIRq, byte *sendData,
                                 byte sendLen, byte *backData = NULL,
                                 byte *backLen = NULL, byte *validBits = NULL,
                                 byte rxAlign = 0, bool checkCRC = false);
    byte PICC_RequestA(byte *bufferATQA, byte *bufferSize);
    byte PICC_WakeupA(byte *bufferATQA, byte *bufferSize);
    byte PICC_REQA_or_WUPA(byte command, byte *bufferATQA, byte *bufferSize);
    byte PICC_Select(Uid *uid, byte validBits = 0);
    byte PICC_HaltA();
    byte PCD_Authenticate(byte command, byte blockAddr, MIFARE_Key *key, Uid *uid);
    void PCD_StopCrypto1();
    byte MIFARE_Read(byte blockAddr, byte *buffer, byte *bufferSize);
    byte MIFARE_Write(byte blockAddr, byte *buffer, byte bufferSize);
    byte PCD_MIFARE_Transceive(byte *sendData, byte sendLen, bool acceptTimeout = false);
    const __FlashStringHelper *GetStatusCodeName(byte code);
    byte PICC_GetType(byte sak);
    const __FlashStringHelper *PICC_GetTypeName(byte type);
    bool PICC_IsNewCardPresent();
    bool PICC_ReadCardSerial();

   private:
    byte _chipAddress;
    byte MIFARE_TwoStepHelper(byte command, byte blockAddr, long data);
};

#endif
