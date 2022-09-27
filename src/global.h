#pragma once
#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <Arduino.h>

#define SERIAL_DEBUG

extern volatile int POWER_DOWN_SLEEP_COUNTER;
extern float VBAT;
extern float SHUTDOWN_VOLTAGE;
extern float TEMPERATURE;
extern int AIR_VALUE;
extern int WATER_VALUE;
extern float SOIL_MOISTURE_PERCENT;
#endif