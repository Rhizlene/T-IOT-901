#include "routing_api.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ============================================================================
// CACHE RAM (persist durant la session, max CACHE_MAX entrees)
// ============================================================================

#define CACHE_MAX 20

struct CacheEntry {
    char uid[32];
    int  warehouseId;
    char store[4];
    int  servoAngle;
};

static CacheEntry cache[CACHE_MAX];
static int cacheCount = 0;

static void cacheStore(const String& uid, int warehouseId, const String& store, int servoAngle) {
    // Mettre a jour l'entree si l'UID existe deja
    for (int i = 0; i < cacheCount; i++) {
        if (uid == cache[i].uid) {
            cache[i].warehouseId = warehouseId;
            store.toCharArray(cache[i].store, sizeof(cache[i].store));
            cache[i].servoAngle = servoAngle;
            return;
        }
    }
    // Nouvel UID : ajouter (FIFO circulaire si plein)
    int idx = (cacheCount < CACHE_MAX) ? cacheCount++ : (cacheCount % CACHE_MAX);
    uid.toCharArray(cache[idx].uid, sizeof(cache[idx].uid));
    cache[idx].warehouseId = warehouseId;
    store.toCharArray(cache[idx].store, sizeof(cache[idx].store));
    cache[idx].servoAngle = servoAngle;
    Serial.printf("Cache [%d/%d]: %s -> Entrepot %d (%s)\n", cacheCount, CACHE_MAX, uid.c_str(), warehouseId, store.c_str());
}

static bool cacheLookup(const String& uid, CacheEntry& out) {
    for (int i = 0; i < cacheCount; i++) {
        if (uid == cache[i].uid) {
            out = cache[i];
            return true;
        }
    }
    return false;
}

// ============================================================================
// NORMALISATION UID
// ============================================================================

static String normalizeUID(String uid) {
    uid.trim();
    uid.toUpperCase();
    uid.replace(":", "");
    uid.replace(" ", "");
    return uid;
}

// ============================================================================
// REQUETE DE ROUTAGE (avec fallback cache)
// ============================================================================

RouteResult queryRoute(const String& uidRaw) {
    RouteResult result;
    String uid = normalizeUID(uidRaw);
    CacheEntry cached;

    // WiFi KO : aller directement au cache
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Routing: WiFi KO, recherche cache...");
        if (cacheLookup(uid, cached)) {
            Serial.printf("Cache HIT: %s -> Entrepot %d (%s) angle=%d\n",
                          uid.c_str(), cached.warehouseId, cached.store, cached.servoAngle);
            result.status      = QUERY_CACHE_HIT;
            result.warehouseId = cached.warehouseId;
            result.store       = String(cached.store);
            result.servoAngle  = cached.servoAngle;
        } else {
            Serial.println("Cache MISS + WiFi KO -> fallback B");
            result.status      = QUERY_NO_API_NO_CACHE;
            result.warehouseId = 2;
            result.store       = "B";
            result.servoAngle  = DEFAULT_ANGLE;
        }
        return result;
    }

    // Appel API
    HTTPClient http;
    String url = "http://";
    url += ROUTING_API_HOST;
    url += ":";
    url += String(ROUTING_API_PORT);
    url += "/api/routing/by-rfid/";
    url += uid;

    Serial.print("ROUTING API GET: ");
    Serial.println(url);

    http.begin(url);
    http.setTimeout(API_TIMEOUT);
    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        Serial.println("Routing API Response: " + payload);

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            result.status      = QUERY_OK;
            result.warehouseId = doc["warehouseId"] | 2;
            result.store       = String(doc["store"] | "B");
            result.servoAngle  = doc["servoAngle"] | DEFAULT_ANGLE;
            // Stocker en cache pour les prochaines pannes reseau
            cacheStore(uid, result.warehouseId, result.store, result.servoAngle);
        } else {
            // JSON invalide : essayer le cache
            Serial.printf("JSON parse error: %s\n", error.c_str());
            if (cacheLookup(uid, cached)) {
                result.status      = QUERY_CACHE_HIT;
                result.warehouseId = cached.warehouseId;
                result.store       = String(cached.store);
                result.servoAngle  = cached.servoAngle;
            } else {
                result.status      = QUERY_NO_API_NO_CACHE;
                result.warehouseId = 2;
                result.store       = "B";
                result.servoAngle  = DEFAULT_ANGLE;
            }
        }

    } else if (httpCode == 404) {
        // UID absent de la base de l'API
        Serial.printf("Routing API: UID inconnu (404) -> fallback B\n");
        result.status      = QUERY_UNKNOWN_UID;
        result.warehouseId = 2;
        result.store       = "B";
        result.servoAngle  = DEFAULT_ANGLE;

    } else {
        // Erreur reseau / serveur -> essayer le cache
        Serial.printf("Routing API Error HTTP %d -> recherche cache\n", httpCode);
        if (cacheLookup(uid, cached)) {
            Serial.printf("Cache HIT: %s -> Entrepot %d (%s)\n",
                          uid.c_str(), cached.warehouseId, cached.store);
            result.status      = QUERY_CACHE_HIT;
            result.warehouseId = cached.warehouseId;
            result.store       = String(cached.store);
            result.servoAngle  = cached.servoAngle;
        } else {
            result.status      = QUERY_NO_API_NO_CACHE;
            result.warehouseId = 2;
            result.store       = "B";
            result.servoAngle  = DEFAULT_ANGLE;
        }
    }

    http.end();
    return result;
}
