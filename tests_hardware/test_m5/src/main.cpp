#include <M5Stack.h>

void setup() {
  M5.begin();
  Serial.begin(115200);

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.println("TEST M5 CORE");
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.println("Ecran OK");
  M5.Lcd.setCursor(10, 70);
  M5.Lcd.println("Boutons A/B/C");

  Serial.println("=== TEST M5 CORE ===");
  Serial.println("Ecran OK");
  Serial.println("Boutons A/B/C");
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("Bouton A OK");
    Serial.println("Bouton A OK");
  }

  if (M5.BtnB.wasPressed()) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("Bouton B OK");
    Serial.println("Bouton B OK");
  }

  if (M5.BtnC.wasPressed()) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("Bouton C OK");
    Serial.println("Bouton C OK");
  }

  delay(20);
}
