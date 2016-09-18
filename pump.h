#ifndef pump_h
#define pump_h

#include <Arduino.h>
#include "time.h"

// Pump
class Pump {

  bool state;       // Pump is ON / OFF

  int pump_pin;       // DO pin#
  int moisture_pin;   // AI pin#
  int waterlevel_pin; // DI pin#
  int button_pin;     // DI pin#
  
  float moisture;    // Soil Moisture level
  float waterlevel;  // Water Tank Level

public:
  Pump(int pump, int moist, int water, int button);
  void Init();
  void ForceSetOn(void);
  void ForceSetOff(void);
  bool SetOn(void);
  bool SetOff(void);
  
  float ReadMoisture();
  float ReadWaterLevel();
  bool ReadPumpState();
  bool ReadButtonState();
};


#endif
