#include "functions.h"
#include "io_pins.h"
#include "global.h"
//#include "DHT.h"

void Setup_Pins()
{
    pinMode(PIN_INFO_LED, OUTPUT);
    pinMode(PIN_SHUTDOWN, OUTPUT);
    pinMode(PIN_SENSOR_ACTIVATION, OUTPUT);
    pinMode(VBAT_SENSOR_PIN, INPUT);
    pinMode(TEMP_SENSOR_PIN, INPUT);
    pinMode(SOIL_SENSOR_PIN, INPUT);
    pinMode(7, INPUT_PULLUP); //stop unused 
    pinMode(8, INPUT_PULLUP); //
    pinMode(A3, INPUT_PULLUP); //pins floating
    
    analogReference(DEFAULT);
}

//void unSetup_Pins()
//{
//    pinMode(PIN_INFO_LED, INPUT);
//    pinMode(PIN_SHUTDOWN, INPUT);
//    pinMode(PIN_SENSOR_ACTIVATION, INPUT);
//}

void Blink_Info_LED()
{
    digitalWrite(PIN_INFO_LED, HIGH);
    delay(30);
    digitalWrite(PIN_INFO_LED, LOW);
    delay(30);
}

void ReadVbat() {
 /*
  // https://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/ 
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bitRead(ADCSRA, ADSC));

  uint8_t adc_low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t adc_high = ADCH; // unlocks both
  long adc_result = (adc_high<<8) | adc_low;
  adc_result = 1125300L / adc_result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return adc_result; // Vcc in millivolts*/

  VBAT = analogRead(VBAT_SENSOR_PIN) * 0.01513944223; //// calibrate by changing the last digit(s) of 0.0166
                                                      //// calibrated 24/9/22.

  Serial.print("The battery is ");
  Serial.print(VBAT);
  Serial.println(" volts");
}

bool CheckShutdown()
{
 //Check if battery voltage is too low
 //Return true for succesful test

 if (isnan(VBAT))
 {
    return false;
 }
 else
 {
  if (VBAT <= SHUTDOWN_VOLTAGE)
  {
    Serial.println("Shutting down");
    delay(30);
    //Activate Voltage Regulator SD pin drain transistor.
    digitalWrite(PIN_SHUTDOWN, HIGH);
    delay(300);
  }
 }
 return true;
}

void TurnOnSensors()
{
  Serial.println("Activating Sensors");
  digitalWrite(PIN_SENSOR_ACTIVATION, HIGH);
  delay(2000); //wait 1s for readings to settle
}

void TurnOffSensors()
{
  Serial.println("Deactivating Sensors");
  digitalWrite(PIN_SENSOR_ACTIVATION, LOW);
  delay(30);
  //unSetup_Pins();
  //delay(30);
}

void ReadTempSensor()
{
 //Read and average the temp 
  int numReadings = 50;
  int averageReading = 0;
  for (int n = 0; n < numReadings; n++) 
    averageReading += analogRead(TEMP_SENSOR_PIN);

  averageReading /= numReadings;

  //Serial.print("Temp Pin average reading: ");
  //Serial.println(averageReading);

  float voltage = averageReading * (3300/1024.0);

  TEMPERATURE= (voltage - 585) /10; //was 500, but 585 seems to work with the soil sensor also attached.
  Serial.print("Temperature: ");
  Serial.print(TEMPERATURE);
  Serial.print(" \xC2\xB0"); // shows degree symbol
  Serial.println("C");
}

void ReadSoilSensor()
{
 //Read the value 
  int soil_reading = analogRead(SOIL_SENSOR_PIN); 

  //convert to percentage here for transmission

  SOIL_MOISTURE_PERCENT = map(soil_reading, AIR_VALUE,WATER_VALUE, 0, 100);

  Serial.print("Soil moisture ");
  Serial.print(SOIL_MOISTURE_PERCENT);
  Serial.println("%");
}

void PrintResetReason()
{
  uint8_t mcusr_copy = MCUSR;
  MCUSR = 0;
  Serial.print("MCUSR:");
  if(mcusr_copy & (1<<WDRF)) Serial.print(" WDRF");
  if(mcusr_copy & (1<<BORF)) Serial.print(" BORF");
  if(mcusr_copy & (1<<EXTRF)) Serial.print(" EXTRF");
  if(mcusr_copy & (1<<PORF)) Serial.print(" PORF");
  Serial.println();
}