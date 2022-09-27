#include <Arduino.h>
#include "io_pins.h"
#include "functions.h"
#include "lorawan.h"
#include "global.h"
#include "powerdown.h"
#include "version.h"
#include "version_build.h"

void setup()
{
  Serial.begin(115200);
  Serial.println(F("Starting ..."));
  Serial.println("Sketch: " VERSION_MAJOR "." VERSION_MINOR "." VERSION_PATCH "." BUILD_COMMIT "-" BUILD_BRANCH);
  Serial.println("Builddate: " BUILD_DATE " " BUILD_TIME);
  PrintResetReason();
  LoRaWANVersion();
  Setup_Pins();
  delay(3000);
  PowerDownSetupWatchdog();
  Blink_Info_LED();
  LoRaWANSetup();
  
}

void loop()
{
  LoRaWANDo();
}
