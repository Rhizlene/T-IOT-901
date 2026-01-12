#include <M5Stack.h>
#include <ESP32Servo.h>

Servo servo1;
int servoPin = 25; // GPIO 25 pour SERVO 1 du GoPlus2
int currentPosition = 90; // Position actuelle

void setup() {
    M5.begin();
    M5.Power.begin();
    Serial.begin(115200);
    
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("=== SERVO TEST ===");
    M5.Lcd.println("");
    M5.Lcd.println("The Conveyor");
    M5.Lcd.println("Tri des colis");
    M5.Lcd.println("");
    
    Serial.println("============================");
    Serial.println("  Servo Control Test");
    Serial.println("============================");
    
    // Configuration PWM
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    
    servo1.setPeriodHertz(50); // Fréquence standard 50Hz
    servo1.attach(servoPin, 1000, 2000);
    
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("Servo: GPIO 25");
    M5.Lcd.println("");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("A: Gauche (0)");
    M5.Lcd.println("B: Centre (90)");
    M5.Lcd.println("C: Droite (180)");
    
    Serial.println("Servo attached on GPIO 25");
    
    // Position initiale au centre
    servo1.write(90);
    currentPosition = 90;
    
    Serial.println("Initial position: 90 deg");
    Serial.println("============================");
}

void loop() {
    M5.update();
    
    // Bouton A : Position 0° (Gauche)
    if (M5.BtnA.wasPressed()) {
        currentPosition = 0;
        servo1.write(currentPosition);
        
        M5.Lcd.fillRect(0, 200, 320, 40, BLACK);
        M5.Lcd.setCursor(10, 200);
        M5.Lcd.setTextColor(YELLOW);
        M5.Lcd.println(">>> GAUCHE (0 deg)");
        
        Serial.println("Position: 0 deg (LEFT)");
        M5.Speaker.tone(800, 50);
    }
    
    // Bouton B : Position 90° (Centre)
    if (M5.BtnB.wasPressed()) {
        currentPosition = 90;
        servo1.write(currentPosition);
        
        M5.Lcd.fillRect(0, 200, 320, 40, BLACK);
        M5.Lcd.setCursor(10, 200);
        M5.Lcd.setTextColor(CYAN);
        M5.Lcd.println(">>> CENTRE (90 deg)");
        
        Serial.println("Position: 90 deg (CENTER)");
        M5.Speaker.tone(1000, 50);
    }
    
    // Bouton C : Position 180° (Droite)
    if (M5.BtnC.wasPressed()) {
        currentPosition = 180;
        servo1.write(currentPosition);
        
        M5.Lcd.fillRect(0, 200, 320, 40, BLACK);
        M5.Lcd.setCursor(10, 200);
        M5.Lcd.setTextColor(GREEN);
        M5.Lcd.println(">>> DROITE (180 deg)");
        
        Serial.println("Position: 180 deg (RIGHT)");
        M5.Speaker.tone(1200, 50);
    }
    
    delay(50);
}