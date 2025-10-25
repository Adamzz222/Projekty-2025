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
constexpr uint8_t MODE_COUNT = 5; // 0: live, 1: trasa, 2: last, 3: errors





// ===== Funkcje PID =====
float readPid(float   (ELM327::*func)(),  uint16_t timeout = 1000);
float readPid(uint8_t (ELM327::*func)(),  uint16_t timeout = 1000);
float readPid(int32_t (ELM327::*func)(),  uint16_t timeout = 1000);

// ===== Zapisywanie/wczytywanie last =====
void saveLastToNVS(float dist, float avgs, float avgc, unsigned long mins);
void loadLastFromNVS();

// ----Obliczenia
void computeInstantSimple(float dt);

// ===== DTC =====
extern String dtcList[6];
extern uint8_t dtcCount;
void readDTCs03();

