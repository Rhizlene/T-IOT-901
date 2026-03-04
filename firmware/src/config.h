#pragma once

// ============================================================================
// CONFIGURATION - The Conveyor T-IOT-901
// ============================================================================

// WiFi
#define WIFI_SSID       "Finv"
#define WIFI_PASSWORD   "17082020"

// API Routing RFID (Node/Express)
#define ROUTING_API_HOST    "172.22.27.122"
#define ROUTING_API_PORT    3000

// Adresses I2C
#define RFID_I2C_ADDR   0x28
#define GOPLUS2_ADDR    0x38
#define GRBL_I2C_ADDR   0x70

// Servo (sur GoPlus2)
#define SERVO_CH1       0

// Mapping entrepots -> angles servo (magasins A/B/C)
#define WAREHOUSE_A_ANGLE   5
#define WAREHOUSE_B_ANGLE   15
#define WAREHOUSE_C_ANGLE   25
#define DEFAULT_ANGLE       15

// Timeouts (ms)
#define WIFI_TIMEOUT        20000
#define RFID_SCAN_TIMEOUT   5000
#define API_TIMEOUT         5000
#define MOTOR_MOVE_TIME     2000

// Vitesses moteur (mm/min)
#define CONVEYOR_SLOW_SPEED     20      // Vitesse lente continue (tapis en marche)
#define CONVEYOR_EJECT_SPEED    3000    // Vitesse d'ejection du colis

// Servo timing (ms)
#define SERVO_MOVE_DELAY    500     // Temps pour que le servo atteigne sa position
#define SERVO_HOLD_TIME     10000   // Temps de maintien du servo pendant le passage du colis

// ============================================================================
// ETATS DE LA MACHINE
// ============================================================================

enum ConveyorState {
    STATE_INIT,
    STATE_READY,
    STATE_DETECTING,
    STATE_READING,
    STATE_QUERYING,
    STATE_ROUTING,
    STATE_ERROR
};
