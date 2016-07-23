#include "pump.h"

Pump::Pump(int pump, int moist, int water, int button)
{
  state = false;
  moisture = 0;    // Soil Moisture level
  waterlevel = 0;  // Water Tank Level

  pump_pin = pump;
  moisture_pin = moist;
  waterlevel_pin = water;
  button_pin = button;
}

void Pump::Init()
{
  pinMode(pump_pin, OUTPUT);
  pinMode(moisture_pin, INPUT); // set the Soil Moisture Sensort pin mode
  pinMode(waterlevel_pin, INPUT); // water sensor
  pinMode(button_pin, INPUT_PULLUP);// button
}

bool Pump::ReadPumpState(void)
{
  return state;
}

bool Pump::ReadButtonState(void)
{
  return digitalRead( button_pin );
}

void Pump::ForceSetOff(void)
{
  // Turn pump OFF
  state = false;
  digitalWrite(pump_pin, LOW);
}

void Pump::ForceSetOn(void)
{
  // Turn pump ON
  state = true;
  digitalWrite(pump_pin, HIGH);
}

bool Pump::SetOn(void)
{
  Serial.println("Pump::SetOn()");

  // Check if the pump is not already set ON
  if (state == true) {
    Serial.println("Pump::SetOn: Currently is ON.");
    return false;
  }

  // Read moisture level
  moisture = analogRead( moisture_pin );

  // Read water tank level
  waterlevel = digitalRead( waterlevel_pin );

  // Turn pump ON
  state = true;
  digitalWrite(pump_pin, HIGH);

  return true;

  /*
    // Turn the pump only if the soil is dry
    if (moisture >= 650) {

      Serial.println("Pump::SetOn: Soil is DRY.");

      // Don't turn on the pump if water tank is empty
      if (waterlevel == HIGH) {
        Serial.println("Pump::SetOn: Water tank is empty!");
        //     return false;
      }

      // Turn pump ON
      state = true;
      digitalWrite(pump_pin, HIGH);

      return true;
    }

    if ( (moisture < 650) && (moisture >= 500) ) {
      // in case of moist soil:
      Serial.println("Pump::SetOn: Soil is moist.");
    }

    if (moisture < 500) {
      // in case of soggy soil:
      Serial.println("Pump::SetOn: Soil is soggy.");
    }

    return false;
  */
}

bool Pump::SetOff(void)
{
  Serial.println("Pump::SetOff()");

  // Check if the pump is not already set ON
  if (state == false) {
    Serial.println("Pump::SetOff: Currently is OFF.");
    return false;
  }

  // Turn pump OFF
  state = false;
  digitalWrite(pump_pin, LOW);

  return true;
}

float Pump::ReadMoisture()
{
  // Read moisture level
  moisture = analogRead( moisture_pin );
  return moisture;
}

float Pump::ReadWaterLevel()
{
  // Read water tank level
  waterlevel = digitalRead( waterlevel_pin );
  return waterlevel;
}
