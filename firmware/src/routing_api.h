#pragma once

#include <Arduino.h>
#include "config.h"

// Resultat d'une requete de routage
enum QueryStatus {
    QUERY_OK,             // API OK, UID connu -> route normale
    QUERY_CACHE_HIT,      // API inaccessible, resultat depuis le cache RAM
    QUERY_UNKNOWN_UID,    // API OK mais UID inconnu (404) -> fallback B
    QUERY_NO_API_NO_CACHE // API inaccessible + pas de cache -> fallback B
};

struct RouteResult {
    QueryStatus status;
    int warehouseId;  // 1=A, 2=B, 3=C
    String store;     // "A", "B" ou "C"
    int servoAngle;   // angle servo (depuis API ou cache)
};

RouteResult queryRoute(const String& uidRaw);
