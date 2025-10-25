#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <BluetoothSerial.h>
#include <ELMduino.h>

//pliki podrzędne 
#include "Ethnocentric_Rg14pt7b.h"
#include "Ethnocentric_Rg4pt7b.h"
#include "logo.h" 
#include "functions.h" 

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define JOY_PIN_X 34
#define MICRO_SWITCH_PIN 35

//OLED 128x64 2.42" I2C - domyślne piny, res -> -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//ELM-327
BluetoothSerial SerialBT; 
uint8_t REMOTE_MAC[] = {0x10, 0x21, 0x3E, 0x4E, 0x93, 0xFF};

// odczyty z joysticka
int x;                     
bool sw;

// chwilowe
float    dt = 0.0f;    // dt w sekundach (z ułamkiem)
uint32_t now = 0;      // czas w ms
uint32_t lastUpdateMs = 0;
int loopCounter = 0;;
unsigned long dtcStart = 0;
bool dtcDone = false;

//race
float raceStartMs = 0;  
float raceTimeSec=0.0f; // aktualny czas biegu w sekundach
float raceDistance = 0.0f; // km
float raceAveSpeed = NAN;  // km/h
float race100KphTime = NAN;// s do 100 km/h
bool raceStatus        = false; // pomiar w toku
bool raceResultSaved   = false; // do przyszłego zapisu
bool race100kphReached = false; // czy padł już 100 km/h




void setup() {
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);
  pinMode(MICRO_SWITCH_PIN, INPUT-PULLUP);
  
  for (int y = 0; y <= 64; y += 2) {
    display.clearDisplay();
    display.drawBitmap(0, y - 64, napis_vw_bmp, 128, 64, WHITE);
    display.display();
  }
  
  SerialBT.begin("ESP32_OBD", true);
  SerialBT.setPin("1234", 4);
  while (!SerialBT.connect(REMOTE_MAC)) {
    delay(2000);
  }
  SerialBT.print("ATZ\r");  delay(500);
  SerialBT.print("ATE0\r"); delay(100);
  SerialBT.print("ATSP0\r");delay(200);

  if (!obd.begin(SerialBT, Serial)) {
    while (1) delay(1000);
  }

  // Wczytaj Last z poprzedniej sesji (zamrożony)
  loadLastFromNVS();
  totalFuel     = 0.0f;
  totalDist     = 0.0f;
  currentMin    = 0;
  avgSpeed      = NAN;
  avgConsumption= NAN;
  lastUpdateMs = millis();
}


void loop() {


  now = millis();                              // now i lastUpdateMs są uint32_t
  uint32_t diff = now - lastUpdateMs;          // poprawnie działa także po overflow
  dt = diff * (1.0f / 1000.0f);                // sekundy jako float
  lastUpdateMs = now;

  if (dt < 0.0f) dt = 0.0f;                    // clamp
  if (dt > 3.0f) dt = 3.0f;
  // czas pomiędzy pętlami
  
  loopCounter = (loopCounter + 1) % 12;
  
  // obliczenia z użyciem dt
  if (speed != -50.0f && !isnan(speed) && speed >= 0 && dt > 0.0f) {
    totalDist += (speed * dt) / 3600.0f;  // km/h * s / 3600 = km
  } 
  currentMin = millis() / 60000UL;
  avgConsumption = (totalDist > 0.01f) ? (totalFuel / totalDist) * 100.0f : NAN;
  avgSpeed       = (currentMin > 0) ? (totalDist * 60.0f) / float(currentMin) : NAN;

  
  // PID-y co iterację
  engineLoad = readPid(&ELM327::engineLoad);
  speed    = readPid(&ELM327::kph);
  // PID-y co 6 iteracji
  if (loopCounter % 4 == 0) {
    computeInstantSimple(dt);
    rpm      = readPid(&ELM327::rpm);
    MaP      = readPid(&ELM327::manifoldPressure);
  }
  // Co 12 iteracji
  if (loopCounter == 7) {
    tempC      = readPid(&ELM327::engineCoolantTemp);
    iAT        = readPid(&ELM327::intakeAirTemp);
  }

  if (!isnan(speed) && speed == 0.0f) {
    if (!tripStopSaved) {
      saveLastToNVS(totalDist, avgSpeed, avgConsumption, currentMin);
      tripStopSaved = true;   // jeden zapis na jeden postój
    }
  } 
  else if (!isnan(speed) && speed > 1.0f) {
    tripStopSaved = false;     // odblokuj zapis dla następnego postoju
  }

  // mode
  x = analogRead(JOY_PIN_X);
  sw = digitalRead(MICRO_SWITCH_PIN);

  
  if (!sw) {
    raceStatus         = true;
    raceStartMs        = now;
    raceTimeSec        = 0.0f;
    raceDistance       = 0.0f;
    raceAveSpeed       = NAN;
    race100KphTime     = NAN;
    race100kphReached  = false;
  }
  if (x > 3700) {
    mode = (mode + 1) % MODE_COUNT;
  }
  else if (mode != 4 && x < 400) {
    mode = (mode == 0) ? (MODE_COUNT - 1) : (mode - 1);
  }

  if (mode == 3) {
    if (dtcStart == 0) dtcStart = now;                    // now = millis()
    if (!dtcDone && (now - dtcStart >= 6000UL)) {         // 3 s dwell
      readDTCs03();                                       // JEDEN strzał
      dtcDone = true;
    }
  } else {                                                // opuściłeś Errors
    dtcStart = 0;
    dtcDone  = false;
  }

  // Wyświetlanie
  display.clearDisplay();
  switch (mode) {
    case 0: { // Live
      display.setFont(&Ethnocentric_Rg14pt7b);
      display.setTextColor(WHITE);
      display.setCursor(35, 19);
      if (speed == -50.0f || isnan(speed)) {
        display.print("--");
      } else {
        display.print(int(speed));
      }

      display.setFont(&Ethnocentric_Rg4pt7b);
      display.setCursor(52, 27);
      display.print("kph");

      display.setFont();
      display.setCursor(30, 56);
      display.print(isnan(instConsumption) ? String("--.-") : String(instConsumption, 1));
      display.print(" L/100km");

      int barX = 29, barY = 37, barWidth = 72, barHeight = 5;
      display.drawRect(barX, barY, barWidth + 1, barHeight, WHITE);
      int fillWidth = (engineLoad != -50.0f) ? map((int)engineLoad, 0, 100, 0, barWidth - 2) : 0;
      display.fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, WHITE);
      for (int i = 0; i <= 4; i++) display.drawFastVLine(barX + 18 * i, barY - 2, 2, WHITE);

      display.drawBitmap(107, 25, load_bmp, 20, 20, WHITE);
      display.drawBitmap(0, 20, engine_temp_bmp, 20, 20, WHITE);
      display.setFont(&Ethnocentric_Rg4pt7b);
      display.setCursor(0, 47);
      if (tempC == -50.0f) {
        display.print("--");
      } else {
        display.print(int(tempC));
      }
      break;
    }

    case 1: { // Trasa bieżąca
      display.setFont(&Ethnocentric_Rg4pt7b);
      display.setCursor(45, 20);
      display.print("Trasa");
      display.setFont();
      display.setCursor(0, 28);
      display.print(
        String("spd:   ") + (isnan(avgSpeed) ? String("--") : String(avgSpeed, 0)) + " km/h\n" +
        "con:   " + (isnan(avgConsumption) ? String("--.-") : String(avgConsumption, 1)) + " L/100km\n" +
        "dis:   " + String(totalDist, 1) + " km\n" +
        "tim:   " + currentMin + " min");
      break;
    }

    case 2: { // Last (z NVS)
      display.setFont(&Ethnocentric_Rg4pt7b);
      display.setCursor(45, 20);
      display.print("Last");
      display.setFont();
      display.setCursor(0, 28);
      display.print(
        String("spd:   ") + (isnan(last_avgSpeed) ? String("--") : String(last_avgSpeed, 0)) + " km/h\n" +
        "con:   " + (isnan(last_avgConsumption) ? String("--.-") : String(last_avgConsumption, 1)) + " L/100km\n" +
        "dis:   " + String(last_totalDist, 1) + " km\n" +
        "tim:   " + last_timeMin + " min");
      break;
    }

    case 3: { // Errors
      display.setFont(&Ethnocentric_Rg4pt7b);
      display.setCursor(45, 20);
      display.print("Errors");
      display.setFont();
      display.setCursor(0, 28);

      if (dtcCount == 0) {
        display.print("No errors");
      } else {
        for (uint8_t i = 0; i < dtcCount && i < 6; ++i) {
          display.print(dtcList[i]);
          if (i < dtcCount - 1) display.print("\n");
        }
      }
      break;
    }
   case 4: {
      if (!raceResultSaved) {

        // start pomiaru (zapisz tylko moment startu)
        if (speed > 0.0f && !raceStatus) {
          raceStatus  = true;
          raceStartMs = now;            // używamy Twojego 'now'
        }

        // stop pomiaru gdy stoisz lub brak danych
        if (speed == 0.0f || speed == -50.0f || isnan(speed)) {
          raceStatus = false;
        }

        if (raceStatus) {
          // czas biegu w sekundach
          raceTimeSec = (now - raceStartMs) * 0.001f;

          // 0–100 km/h (jednorazowo)
          if (!race100kphReached && speed >= 100.0f) {
            race100KphTime   = raceTimeSec;
            race100kphReached = true;
          }

          // dystans [km] (km/h * s / 3600)
          if (speed != -50.0f && !isnan(speed) && speed >= 0.0f && dt > 0.0f) {
            raceDistance += (speed * dt) / 3600.0f;
          }

          // średnia prędkość [km/h]
          raceAveSpeed = (raceTimeSec > 0.0f)
                          ? (raceDistance / (raceTimeSec / 3600.0f))
                          : NAN; 
        }                  
        
        display.setFont(&Ethnocentric_Rg14pt7b);
        display.setTextColor(WHITE);
        display.setCursor(35, 19);
        if (speed == -50.0f || isnan(speed)) {
          display.print("--");
        } else {
          display.print(int(speed));
        }
        display.setFont();
        display.setCursor(0, 38);
        display.print(
          String("tim:   ") 
            + ((raceStatus || raceTimeSec > 0.0f) ? String(raceTimeSec, 2) : String("--.--")) + " s\n"
          + "dis:   " + String(raceDistance, 1) + " km\n"
          + "0-100 time:   " 
            + (!isnan(race100KphTime) ? String(race100KphTime, 2) : String("--.--")) + " s"
        );
        break;
      }
   }
  }
  display.display();
}

