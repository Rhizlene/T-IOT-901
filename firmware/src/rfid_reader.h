#pragma once

#include "MFRC522_I2C.h"
#include "config.h"

extern MFRC522 rfid;

bool initRFID();
String readRFIDTag();
