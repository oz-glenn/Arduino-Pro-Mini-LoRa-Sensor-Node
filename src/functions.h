#pragma once
#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H

#include <Arduino.h>

void Setup_Pins(void);
void Blink_Info_LED(void);
void ReadVbat(void);
bool CheckShutdown(void);
void ReadTempSensor(void);
void PrintResetReason(void);
void ReadSoilSensor(void);
void TurnOnSensors(void);
//void unSetup_Pins (void);
void TurnOffSensors (void);
#endif