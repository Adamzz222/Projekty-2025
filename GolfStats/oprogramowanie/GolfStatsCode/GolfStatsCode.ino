#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <BluetoothSerial.h>
#include <ELMduino.h>

#include "Ethnocentric_Rg14pt7b.h"
#include "Ethnocentric_Rg4pt7b.h"
#include "logo.h"
#include "functions.h"  // deklaracje naszych funkcji z functions.cpp

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define JOY_PIN 34
#define OLED_ADDR 0x3C

//OLED 128x64 2.42" I2C - domyślne piny, res -> -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

BluetoothSerial SerialBT; 
uint8_t REMOTE_MAC[] = {0x10, 0x21, 0x3E, 0x4E, 0x93, 0xFF};

// --- STAŁE ---
constexpr float R_AIR    = 287.05f;
constexpr float ENG_COEF = 0.002f * 0.5f * 0.85f;
constexpr float AFR_E    = 14.7f;
constexpr float RHO_F    = 745.0f;

uint16_t loopCounter = 0;  // licznik pętli (lokalny)
float fuelLps;             // chwilowy przepływ paliwa [L/s]
int x;                     // odczyt z joysticka
unsigned long lastUpdateMs;
unsigned long now;
float dt;

void setup() {
  // OLED intro
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  for (int y = -64; y <= 0; y += 2) {
    display.clearDisplay();
    display.drawBitmap(0, y, logo_vw_bmp, 128, 64, WHITE);
    display.display();
  }
  delay(500);
  for (int y = 0; y <= 64; y += 2) {
    display.clearDisplay();
    display.drawBitmap(0, y, logo_vw_bmp, 128, 64, WHITE);
    display.drawBitmap(0, y - 64, napis_vw_bmp, 128, 64, WHITE);
    display.display();
  }

  // BT + ELM
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
  now = millis();
  dt = (now - lastUpdateMs) / 1000.0f; // sekundy
  lastUpdateMs = now;
  // zabezpieczenie
  if (dt < 0.0f) dt = 0.0f;
  if (dt > 3.0f) dt = 3.0f; 
    loopCounter = (loopCounter + 1) % 10;

    // PID-y co iterację
  rpm      = readFloatPidBlocking(&ELM327::rpm);
  speed    = readIntPidBlocking(&ELM327::kph);
  MaP      = readFloatPidBlocking(&ELM327::manifoldPressure);
  tps_rel = readFloatPidBlocking(&ELM327::relativeThrottle);

  // PID-y co 10 iteracji
  if (loopCounter == 0) {
    tempC      = readFloatPidBlocking(&ELM327::engineCoolantTemp);
    engineLoad = readFloatPidBlocking(&ELM327::engineLoad);
    iAT        = readFloatPidBlocking(&ELM327::intakeAirTemp);
    // odśwież błędy, jeśli w trybie Errors
    if (mode == 3) readDTCs03();
  }

  if (!isnan(rpm) && !isnan(MaP) && !isnan(speed) && speed > 5.0f &&
    !isnan(tps_rel) && tps_rel <= 1.0f &&  // zamknięta przepustnica
    rpm > 1300.0f &&
    MaP < 30.0f) 
{
  instConsumption = 0.0f;
} else {
    // normalna jazda / luz → licz tak jak miałeś
    instConsumption = (
      ENG_COEF * (rpm / 60.0f)
      * (MaP * 1000.0f)
      / (R_AIR * (iAT + 273.15f))
      / AFR_E
      * 1000.0f
      / RHO_F
      * 3600.0f
      / speed
      * 100.0f
    );

    fuelLps = (
      ENG_COEF * (rpm / 60.0f)
      * (MaP * 1000.0f)
      / (R_AIR * (iAT + 273.15f))
      / AFR_E
      * 1000.0f
      / RHO_F
    );

    totalFuel += fuelLps * dt;
    hasMoved = true;
  }

  if (!isnan(speed) && speed >= 0 && dt > 0.0f) {
    totalDist += (speed * dt) / 3600.0f;  // km/h * s / 3600 = km
  } 
  currentMin = millis() / 60000UL;
  avgConsumption = (totalDist > 0.01f) ? (totalFuel / totalDist) * 100.0f : NAN;
  avgSpeed       = (currentMin > 0) ? (totalDist * 60.0f) / float(currentMin) : NAN;

  if (!isnan(speed) && speed == 0.0f) {
    if (!tripStopSaved) {
      saveLastToNVS(totalDist, avgSpeed, avgConsumption, currentMin);
      tripStopSaved = true;   // jeden zapis na jeden postój
    }
  } 
  else if (!isnan(speed) && speed > 1.0f) {
    tripStopSaved = false;     // odblokuj zapis dla następnego postoju
  }

  // Joystick z debounce
  x = analogRead(JOY_PIN);
  if (millis() - lastJoyTime > JOY_DEBOUNCE_MS) {
    if (x > 3700) {
      mode = (mode + 1) % MODE_COUNT;
      lastJoyTime = millis();
      if (mode == 3) readDTCs03();
    } else if (x < 400) {
      mode = (mode == 0) ? (MODE_COUNT - 1) : (mode - 1);
      lastJoyTime = millis();
      if (mode == 3) readDTCs03();
    }
  }

  // Wyświetlanie
  display.clearDisplay();
  switch (mode) {
    case 0: { // Live
      display.setFont(&Ethnocentric_Rg14pt7b);
      display.setTextColor(WHITE);
      display.setCursor(35, 19);
      display.print(int(speed));

      display.setFont(&Ethnocentric_Rg4pt7b);
      display.setCursor(52, 27);
      display.print("kph");

      display.setFont();
      display.setCursor(30, 56);
      display.print(isnan(instConsumption) ? String("--.-") : String(instConsumption, 1));
      display.print(" L/100km");

      int barX = 29, barY = 37, barWidth = 72, barHeight = 5;
      display.drawRect(barX, barY, barWidth + 1, barHeight, WHITE);
      int fillWidth = (!isnan(engineLoad)) ? map((int)engineLoad, 0, 100, 0, barWidth - 2) : 0;
      display.fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, WHITE);
      for (int i = 0; i <= 4; i++) display.drawFastVLine(barX + 18 * i, barY - 2, 2, WHITE);

      display.drawBitmap(107, 25, load_bmp, 20, 20, WHITE);
      display.drawBitmap(0, 20, engine_temp_bmp, 20, 20, WHITE);
      display.setFont(&Ethnocentric_Rg4pt7b);
      display.setCursor(0, 47);
      display.print(int(tempC));
      break;
    }

    case 1: { // Trasa bieżąca
      display.setFont(&Ethnocentric_Rg4pt7b);
      display.setCursor(45, 20);
      display.print("Trasa");
      display.setFont();
      display.setCursor(0, 28);
      display.print(
        String("prd:   ") + (isnan(avgSpeed) ? String("--") : String(avgSpeed, 0)) + " km/h\n" +
        "spl:   " + (isnan(avgConsumption) ? String("--.-") : String(avgConsumption, 1)) + " L/100km\n" +
        "dys:   " + String(totalDist, 1) + " km\n" +
        "czas:   " + currentMin + " min");
      break;
    }

    case 2: { // Last (z NVS)
      display.setFont(&Ethnocentric_Rg4pt7b);
      display.setCursor(45, 20);
      display.print("Last");
      display.setFont();
      display.setCursor(0, 28);
      display.print(
        String("prd:   ") + (isnan(last_avgSpeed) ? String("--") : String(last_avgSpeed, 0)) + " km/h\n" +
        "spl:   " + (isnan(last_avgConsumption) ? String("--.-") : String(last_avgConsumption, 1)) + " L/100km\n" +
        "dys:   " + String(last_totalDist, 1) + " km\n" +
        "czas:   " + last_timeMin + " min");
      break;
    }

    case 3: { // Errors
      display.setFont(&Ethnocentric_Rg4pt7b);
      display.setCursor(45, 20);
      display.print("Errors");
      display.setFont();
      display.setCursor(0, 28);

      if (dtcCount == 0) {
        display.print("Brak aktywnych DTC");
      } else {
        for (uint8_t i = 0; i < dtcCount && i < 6; ++i) {
          display.print(dtcList[i]);
          if (i < dtcCount - 1) display.print("\n");
        }
      }
      break;
    }

  }
  display.display();
}

