#ifndef clock_h
#define clock_h

#include <Arduino.h>
#include <WiFi101.h>
#include <WiFiUdp.h>
#include "time.h"

void setTimeUsingTimeServer();
unsigned long readLinuxEpochUsingNTP();
unsigned long sendNTPpacket(IPAddress & address);

#endif
