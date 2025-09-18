#pragma once
#include <Arduino.h>
#include <ELMduino.h>

// Udostępnij instancję OBD z .ino
extern ELM327 obd;
// ===== Zmienne globalne PID =====
extern float rpm;
extern float speed;
extern float tempC;
extern float engineLoad;
extern float iAT;
extern float MaP;
extern float tps_rel;     

// Flagi sesji
extern bool hasMoved;
extern bool savedThisSession;
extern bool tripStopSaved; 

// Dane "Last" wczytywane z NVS
extern float last_avgSpeed;
extern float last_avgConsumption;
extern float last_totalDist;
extern unsigned long last_timeMin;

// ===== Wyniki obliczeń =====
extern float instConsumption;
extern float avgConsumption;
extern float avgSpeed;

// ===== Statystyki trasy =====
extern float totalFuel;
extern float totalDist;
extern unsigned long currentMin;

// ===== Tryby =====
extern int mode;
constexpr uint8_t MODE_COUNT = 4; // 0: live, 1: trasa, 2: last, 3: errors

// ===== Debounce joysticka =====
extern unsigned long lastJoyTime;
constexpr unsigned long JOY_DEBOUNCE_MS = 50;

// ===== Funkcje PID =====
float readFloatPidBlocking(float (ELM327::*func)(), uint16_t timeout = 1000);
float readFloatPidBlocking(uint8_t (ELM327::*func)(), uint16_t timeout = 1000);
int   readIntPidBlocking(int32_t (ELM327::*func)(), uint16_t timeout = 1000);

// ===== Zapisywanie/wczytywanie last =====
void saveLastToNVS(float dist, float avgs, float avgc, unsigned long mins);
void loadLastFromNVS();

// ===== DTC =====
extern String dtcList[6];
extern uint8_t dtcCount;
void readDTCs03();

