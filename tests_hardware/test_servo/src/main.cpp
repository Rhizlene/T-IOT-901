#include <M5Stack.h>
#include <Wire.h>
 
// GoPlus2 I2C address
#define GOPLUS2_ADDR 0x38
 
// Servo channels (0-3 pour SERVO 1-4)
#define SERVO_CH1 0
#define SERVO_CH2 1
#define SERVO_CH3 2
#define SERVO_CH4 3
 
int currentPosition = 5;
 
// Fonction pour contrôler un servo via GoPlus2
void setServoAngle(uint8_t channel, uint8_t angle) {
    Wire.beginTransmission(GOPLUS2_ADDR);
    Wire.write(channel);      // Canal servo (0-3)
    Wire.write(angle);        
    Wire.endTransmission();
}
 
void setup() {
    M5.begin();
    M5.Power.begin();
    Serial.begin(115200);
 
    // Initialisation I2C (SDA=21, SCL=22 par défaut sur M5Stack)
    Wire.begin(21, 22);
    Wire.setClock(100000); // 100kHz
 
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("=== SERVO TEST ===");
    M5.Lcd.println("");
    M5.Lcd.println("GoPlus2 + Servo");
    M5.Lcd.println("The Conveyor");
    M5.Lcd.println("");
 
    Serial.println("============================");
    Serial.println("  Servo Control via GoPlus2");
    Serial.println("============================");
 
    // Scan I2C pour vérifier la présence du GoPlus2
    Wire.beginTransmission(GOPLUS2_ADDR);
    if (Wire.endTransmission() == 0) {
        M5.Lcd.setTextColor(GREEN);
        M5.Lcd.println("GoPlus2: OK (0x38)");
        Serial.println("GoPlus2 found at 0x38");
    } else {
        M5.Lcd.setTextColor(RED);
        M5.Lcd.println("GoPlus2: NOT FOUND!");
        Serial.println("ERROR: GoPlus2 not found!");
    }
 
    M5.Lcd.println("");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("A: Gauche (5)");
    M5.Lcd.println("B: Centre (15)");
    M5.Lcd.println("C: Droite (25)");
 
    // Position initiale au centre
    setServoAngle(SERVO_CH1, 15);
    currentPosition = 15;
 
    Serial.println("Initial position: 15 deg");
    Serial.println("============================");
}
 
void loop() {
    M5.update();
 
    // Bouton A : Position 5° (Gauche)
    if (M5.BtnA.wasPressed()) {
        currentPosition = 5;
        setServoAngle(SERVO_CH1, currentPosition);
 
        M5.Lcd.fillRect(0, 200, 320, 40, BLACK);
        M5.Lcd.setCursor(10, 200);
        M5.Lcd.setTextColor(YELLOW);
        M5.Lcd.println(">>> GAUCHE (5 deg)");
 
        Serial.println("Position: 5 deg (LEFT)");
        M5.Speaker.tone(800, 50);
    }
 
    // Bouton B : Position 15° (Centre)
    if (M5.BtnB.wasPressed()) {
        currentPosition = 15;
        setServoAngle(SERVO_CH1, currentPosition);
 
        M5.Lcd.fillRect(0, 200, 320, 40, BLACK);
        M5.Lcd.setCursor(10, 200);
        M5.Lcd.setTextColor(CYAN);
        M5.Lcd.println(">>> CENTRE (15 deg)");
 
        Serial.println("Position: 15 deg (CENTER)");
        M5.Speaker.tone(1000, 50);
    }
 
    // Bouton C : Position 25° (Droite)
    if (M5.BtnC.wasPressed()) {
        currentPosition = 25;
        setServoAngle(SERVO_CH1, currentPosition);
 
        M5.Lcd.fillRect(0, 200, 320, 40, BLACK);
        M5.Lcd.setCursor(10, 200);
        M5.Lcd.setTextColor(GREEN);
        M5.Lcd.println(">>> DROITE (25 deg)");
 
        Serial.println("Position: 25 deg (RIGHT)");
        M5.Speaker.tone(1200, 50);
    }
 
    delay(50);
}          