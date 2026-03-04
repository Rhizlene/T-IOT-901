#include "servo.h"
#include <Arduino.h>

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
