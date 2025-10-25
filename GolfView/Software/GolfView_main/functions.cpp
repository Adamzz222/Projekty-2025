#include "functions.h"
#include <Preferences.h>

static Preferences prefs;
ELM327 obd; 
bool hasMoved = false;
bool savedThisSession = false;
bool tripStopSaved = false;

float last_avgSpeed = NAN;
float last_avgConsumption = NAN;
float last_totalDist = 0.0f;
unsigned long last_timeMin = 0;

// ===== Zmienne globalne PID =====
float rpm = NAN;
float speed = NAN;
float tempC = -50;
float engineLoad = NAN;
float iAT = NAN;
float MaP = NAN;
float tps_rel = NAN;

// ===== Wyniki obliczeń =====
float instConsumption = 0;
float avgConsumption  = 0;
float avgSpeed        = 0;

// ===== Statystyki trasy =====
float totalFuel     = 0.0f;
float totalDist     = 0.0f;
unsigned long currentMin = 0;

// ===== Tryby =====
int mode = 0;
unsigned long lastJoyTime = 0;

// ===== DTC =====
String dtcList[6];
uint8_t dtcCount = 0;



void computeInstantSimple(float dt)
{
  // Jeśli brakuje danych – nic nie rób
  if (isnan(rpm) || isnan(MaP) || isnan(iAT) || isnan(speed)) return;

  float fuelLps = 0.0f;

  // licz tylko powyżej 5 km/h (jak w Twoim kodzie)
  if (speed > 5.0f) {
    // DFCO / odjęty gaz: przy małym loadzie ustaw spalanie na 0
    if (!isnan(engineLoad) && engineLoad > 5.0f) {
      // Używamy tych samych stałych co wcześniej (tu wpisane jawnie, żeby nie było konfliktów linkera)
      const float R_AIR_   = 287.05f;
      const float ENG_COEF_= 0.002f * 0.5f * 0.85f;
      const float AFR_E_   = 14.7f;
      const float RHO_F_   = 745.0f;

      const float air_kgps =
        ENG_COEF_ * (rpm / 60.0f) * (MaP * 1000.0f) / (R_AIR_ * (iAT + 273.15f));

      fuelLps         = (air_kgps / AFR_E_) * 1000.0f / RHO_F_;              // L/s
      instConsumption = (fuelLps * 3600.0f / speed) * 100.0f;                 // L/100 km
    } else {
      instConsumption = 0.0f;   // DFCO
    }

    totalFuel += 4*fuelLps * dt;  // całka paliwa
    hasMoved  = true;
  }
}

// ---- u góry pliku:
#define INVALID  (-50.0f)

// ---- definicje:

float readPid(float (ELM327::*func)(), uint16_t timeout) {
  unsigned long start = millis();
  float val = INVALID;
  do {
    val = (obd.*func)();                                     // blokujące wywołanie
    if (!isnan(val) && obd.nb_rx_state == ELM_SUCCESS)       // tylko nb_rx_state
      return val;
    delay(10);
  } while (millis() - start < timeout);
  return INVALID;
}

float readPid(uint8_t (ELM327::*func)(), uint16_t timeout) {
  unsigned long start = millis();
  uint8_t raw = 0;                                           // NIE -50 (zawinie się)
  do {
    raw = (obd.*func)();
    if (obd.nb_rx_state == ELM_SUCCESS)
      return float(raw);
    delay(10);
  } while (millis() - start < timeout);
  return INVALID;
}

float readPid(int32_t (ELM327::*func)(), uint16_t timeout) {
  unsigned long start = millis();
  int32_t val = 0;
  do {
    val = (obd.*func)();
    if (obd.nb_rx_state == ELM_SUCCESS)                      // bez val >= 0
      return float(val);
    delay(10);
  } while (millis() - start < timeout);
  return INVALID;
}





void saveLastToNVS(float dist, float avgs, float avgc, unsigned long mins) {
  prefs.begin("lasttrip", false);
  prefs.putFloat("dist", dist);
  prefs.putFloat("avgs", avgs);
  prefs.putFloat("avgc", avgc);
  prefs.putULong("mins", mins);
  prefs.end();
}

void loadLastFromNVS() {
  prefs.begin("lasttrip", true);
  last_totalDist      = prefs.getFloat("dist", 0.0f);
  last_avgSpeed       = prefs.getFloat("avgs", NAN);
  last_avgConsumption = prefs.getFloat("avgc", NAN);
  last_timeMin        = prefs.getULong("mins", 0);
  prefs.end();
}

// ===== DTC =====
String _dtcFromWord(uint16_t w) {
    char buff[6];
    char firstChar;
    switch (w >> 14) {
        case 0: firstChar = 'P'; break;
        case 1: firstChar = 'C'; break;
        case 2: firstChar = 'B'; break;
        case 3: firstChar = 'U'; break;
    }
    sprintf(buff, "%c%04X", firstChar, w & 0x3FFF);
    return String(buff);
}

void readDTCs03() {
    dtcCount = 0;

    // 1) Wyślij Mode 03 (stored DTC)
    obd.sendCommand("03");

    // 2) Poczekaj na odpowiedź (ELM_SUCCESS) z prostym timeoutem
    const unsigned long timeoutMs = 1200;
    unsigned long t0 = millis();
    while (obd.nb_rx_state == ELM_GETTING_MSG && (millis() - t0) < timeoutMs) {
        delay(5);
    }
    if (obd.nb_rx_state != ELM_SUCCESS) {
        // brak poprawnej odpowiedzi – brak DTC do pokazania
        return;
    }

    // 3) Z parsujemy payload (ciąg znaków z ELM), wyciągamy bajty hex
    //    Przykładowe odpowiedzi: "43 01 33 00 00 00", czasem z nagłówkami/nowymi liniami
    //    -> zbieramy WSZYSTKIE bajty, potem znajdziemy segmenty po 0x43.
    uint8_t bytesBuf[128];
    size_t n = 0;

    auto hexVal = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        return -1;
    };

    const char* p = obd.payload;   // ELMduino 3.4.1: surowa odpowiedź w payload
    while (*p && n < sizeof(bytesBuf)) {
        // pomijamy wszystko poza hex
        while (*p && hexVal(*p) < 0) p++;
        if (!*p) break;
        int hi = hexVal(*p++); if (hi < 0) break;
        while (*p && hexVal(*p) < 0) p++;
        if (!*p) break;
        int lo = hexVal(*p++); if (lo < 0) break;
        bytesBuf[n++] = (uint8_t)((hi << 4) | lo);
    }

    if (n == 0) return;

    // 4) Znajdź wszystkie segmenty po 0x43 (odpowiedź dla Mode 03) i wyciągnij DTC
    //    Każde DTC to dwa bajty: ABCD -> 16 bitów, konwersja jak w _dtcFromWord
    for (size_t i = 0; i < n && dtcCount < 6; ++i) {
        if (bytesBuf[i] == 0x43) {
            // Bajty DTC zaczynają się po 0x43
            for (size_t j = i + 1; j + 1 < n && dtcCount < 6; j += 2) {
                uint16_t w = ((uint16_t)bytesBuf[j] << 8) | bytesBuf[j + 1];
                if (w == 0x0000 || w == 0xFFFF) break;   // koniec listy
                dtcList[dtcCount++] = _dtcFromWord(w);
            }
        }
    }
}

