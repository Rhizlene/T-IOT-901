#include <M5Stack.h>
#include <WiFi.h>
#include <HTTPClient.h>


const char* WIFI_SSID     = "Finv";
const char* WIFI_PASSWORD = "17082020";

const char* DOLIBARR_HOST = "172.22.27.165";
const char* DOLAPIKEY     = "1ZtIVnP5bQ9QMK983qXoii1kd3c72dRQ";

void connectWiFi() {
    M5.Lcd.println("Connexion WiFi...");
    WiFi.mode(WIFI_STA);                
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 40) {
        delay(500);
        M5.Lcd.print(".");
        Serial.print(".");
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        M5.Lcd.println();
        M5.Lcd.println("WiFi OK");
        M5.Lcd.print("IP ESP32: ");
        M5.Lcd.println(WiFi.localIP());

        Serial.println();
        Serial.println("========== WIFI CONNECTE ==========");
        Serial.print("IP ESP32 = ");
        Serial.println(WiFi.localIP());
        Serial.println("===================================");
    } else {
        M5.Lcd.println();
        M5.Lcd.println("WiFi FAIL");
        Serial.println();
        Serial.println("Erreur WiFi (timeout connexion).");
    }
}

void testDolibarrApi() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Pas de WiFi, abort API");
        M5.Lcd.println("Pas de WiFi -> pas d'appel API");
        return;
    }

    HTTPClient http;

    String url = "http://";
    url += DOLIBARR_HOST;
    url += "/api/index.php/products?DOLAPIKEY=";
    url += DOLAPIKEY;
    url += "&accept=application/json"; 

    Serial.println("---------- APPEL API ----------");
    Serial.print("URL = ");
    Serial.println(url);
    M5.Lcd.println("Appel API...");

    http.begin(url);
    int httpCode = http.GET(); 

    if (httpCode > 0) {
        Serial.printf("HTTP code: %d\n", httpCode);

        String payload = http.getString();
        Serial.print("Taille reponse = ");
        Serial.println(payload.length());
        Serial.println("Debut payload:");
        Serial.println(payload.substring(0, 300)); 

        M5.Lcd.print("HTTP: ");
        M5.Lcd.println(httpCode);
        M5.Lcd.println("Voir Serial pour payload");
    } else {
        Serial.printf("Erreur HTTP: %s\n",
                      http.errorToString(httpCode).c_str());
        M5.Lcd.println("Erreur HTTP");
    }

    http.end();
    Serial.println("-------------------------------");
}

void setup() {
    M5.begin();
    Serial.begin(115200);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("Conveyor Firmware");
    M5.Lcd.println("Test Dolibarr API");
    M5.Lcd.println();

    connectWiFi();
    testDolibarrApi();
}

void loop() {
    M5.update();  
}
