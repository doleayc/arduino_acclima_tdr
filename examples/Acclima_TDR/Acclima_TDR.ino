#include "AcclimaTDR.h"
#include "EnableInterrupt.h"

#define ENABLE_ACCLIMA_TDR_DEBUG  0
#define debugSerial Serial

#define PIN_SDI12 5


SDI12 sdi (PIN_SDI12);
AcclimaTDR acclimaTDR( sdi );

void setup() {
  // put your setup code here, to run once:
  debugSerial.begin(19200);

  // If SDI pin hasn't dedicated interrupt as D2 on Arduino UNO, we should use any other digital pin.
  // Then, we need to set an event on PIN_CHANGE_INTERRUPT on this pin.
  // If SDI pin has own interrupt, we must use it.
  enableInterrupt(PIN_SDI12, SDI12::handleInterrupt, CHANGE);

  #if ENABLE_ACCLIMA_TDR_DEBUG == 1
    debugSerial.println("Debug active");
    acclimaTDR.setDebugSerial(debugSerial);
  #endif
  
  debugSerial.println("START");
  
  char address;
  
  do {    
    address = acclimaTDR.findAddress();
    
    debugSerial.print(F("Address: "));
    
    if(address == '?') {
      debugSerial.println(F("Unable to get"));
      delay(10000);
    }
    else {
      debugSerial.println(address);
    }
  }while(address == '?');

  #if ENABLE_ACCLIMA_TDR_DEBUG == 1
  char infoBuffer[50] = "";
  acclimaTDR.getInfo(infoBuffer, sizeof(infoBuffer));
  debugSerial.print("Info: ");
  debugSerial.println(infoBuffer);
  #endif
  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:
  float vol_water, temperature, permitivity;
  uint16_t electrical_cond, pore_water;

  acclimaTDR.readValues(&vol_water, &temperature, &permitivity, &electrical_cond, &pore_water);

  debugSerial.print(F("Volumetric water: "));
  debugSerial.println(vol_water);
  debugSerial.print(F("Temperature: "));
  debugSerial.println(temperature);
  debugSerial.print(F("Permitivity: "));
  debugSerial.println(permitivity);
  debugSerial.print(F("Electrical conductivity: "));
  debugSerial.println(electrical_cond);
  debugSerial.print(F("Soil pore water: "));
  debugSerial.println(pore_water);
  
  delay(10000);  
}
