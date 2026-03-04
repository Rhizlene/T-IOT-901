#pragma once

#include <Wire.h>
#include "config.h"

bool initServo();
void setServoAngle(uint8_t channel, uint8_t angle);
