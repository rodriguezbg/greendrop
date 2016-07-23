/*
  GreenDrop2 Project

  A simple home drip irrigation system, sending data (soil moisture, temperature) to IoT Server (io.adafruit).

  Circuit:
  MKR1000 or WiFi shield attached

  created 31 May 2016
  by Kristiyan Traykov
*/
#include "DHT.h"
#include <SPI.h>
#include <WiFi101.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include "Clock.h"
#include "Pump.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Adafruit_SleepyDog.h>


// Temperature/Humidity Sensor (DHT22)
#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Watchog timeout
int countdownMS = 0;

// Wifi Create WiFiClient class to connect to the MQTT server.
WiFiClient client;
char ssid[] = "rodriguezbg";      //  your network SSID (name)
char pass[] = "mraziada";         // your network password
int keyIndex = 0;                 // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;

// IoT Server (Adafruit.io)
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "rodriguezbg"          // ...your AIO username (see https://accounts.adafruit.com)..."
#define AIO_KEY         "02686e446d584307af2fb8d5d9d82768"             //"...your AIO key..."

// Store the MQTT server, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);

// Setup a FEEDS called for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char TEMP_FEED[] PROGMEM = AIO_USERNAME "/feeds/temp";
Adafruit_MQTT_Publish temp = Adafruit_MQTT_Publish(&mqtt, TEMP_FEED);

const char WATERL_FEED[] PROGMEM = AIO_USERNAME "/feeds/pumpstatus";
Adafruit_MQTT_Publish pumpstatus = Adafruit_MQTT_Publish(&mqtt, WATERL_FEED);

//const char HUMID_FEED[] PROGMEM = AIO_USERNAME "/feeds/humidity";
//Adafruit_MQTT_Publish humi = Adafruit_MQTT_Publish(&mqtt, HUMID_FEED);

const char MOIST1_FEED[] PROGMEM = AIO_USERNAME "/feeds/moisture1";
const char MOIST2_FEED[] PROGMEM = AIO_USERNAME "/feeds/moisture2";
const char MOIST3_FEED[] PROGMEM = AIO_USERNAME "/feeds/moisture3";
const char MOIST4_FEED[] PROGMEM = AIO_USERNAME "/feeds/moisture4";
Adafruit_MQTT_Publish moist[] = {
  Adafruit_MQTT_Publish(&mqtt, MOIST1_FEED),
  Adafruit_MQTT_Publish(&mqtt, MOIST2_FEED),
  Adafruit_MQTT_Publish(&mqtt, MOIST3_FEED),
  Adafruit_MQTT_Publish(&mqtt, MOIST4_FEED)
};

const char BUTTON1_FEED[] PROGMEM = AIO_USERNAME "/feeds/button1";
const char BUTTON2_FEED[] PROGMEM = AIO_USERNAME "/feeds/button2";
const char BUTTON3_FEED[] PROGMEM = AIO_USERNAME "/feeds/button3";
const char BUTTON4_FEED[] PROGMEM = AIO_USERNAME "/feeds/button4";
Adafruit_MQTT_Subscribe button[] = {
  Adafruit_MQTT_Subscribe(&mqtt, BUTTON1_FEED),
  Adafruit_MQTT_Subscribe(&mqtt, BUTTON2_FEED),
  Adafruit_MQTT_Subscribe(&mqtt, BUTTON3_FEED),
  Adafruit_MQTT_Subscribe(&mqtt, BUTTON4_FEED)
};

//
// Create pump objects
//
int num_pumps = 4;
Pump pump[] = {
  Pump(1, PIN_A1, 0, 14), // DO Pump, AI Moisture Sensor, DI Water Level, DI Button pin
  Pump(4, PIN_A2, 0, 13),
  Pump(7, PIN_A3, 0, 12),
  Pump(8, PIN_A4, 0, 11)
};


void setup() {
  Serial.begin(9600);      // initialize serial communication

  // Set IN/OUT pins
  pinMode(LED_BUILTIN, OUTPUT);      // set the LED pin mode

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true);       // don't continue
  }

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    Alarm.delay(10000);
  }

  printWifiStatus();                        // you're connected now, so print out the status

  // Set the time & date
  setTimeUsingTimeServer();

  // Morning/Evening watering timer
  for (int i = 0; i < num_pumps; i++) {
    pump[i].Init();
    Alarm.alarmRepeat(8, 30, 0, PumpOn, &pump[i]);    // 8:30 AM every day
    Alarm.alarmRepeat(20, 30, 0, PumpOn, &pump[i]);    // 8:30+ PM every day
    //  Alarm.alarmRepeat(dowSaturday,8,30,30,WeeklyAlarm);  // 8:30:30 every Saturday
    //  Alarm.timerRepeat(10, Publish, NULL);            // timer for every 30 seconds

    // Setup MQTT subscription for onoff feed.
    mqtt.subscribe(&button[i]);
  }

  // Enable HW watchdog
  // Finally demonstrate the watchdog resetting by enabling it for a shorter
  // period of time and waiting a long time without a reset.  Notice you can pass
  // a _maximum_ countdown time (in milliseconds) to the enable call.  The library
  // will try to use that value as the countdown, but it might pick a smaller
  // value if the hardware doesn't support it.  The actual countdown value will
  // be returned so you can see what it is.
  countdownMS = Watchdog.enable(10000);
  Serial.print("Watchdog enabled! countdownMS =");
  Serial.println(countdownMS);
}

bool hw_button[] = {1, 1, 1, 1};
bool hw_button_old[] = {1, 1, 1, 1};

void loop()
.0+{
  // Reset the watchdog with every loop to make sure the sketch keeps running.
  // If you comment out this call watch what happens after about 8 iterations!
  Watchdog.reset();

  if (MQTT_Publish()) {
    digitalWrite(LED_BUILTIN, HIGH); // Set LED ON
  }

  // 1 sec delay
  for (int j = 0; j < 50; j++) {

    // Delay
    Alarm.delay(20);

    // Check pump buttons
    for (int i = 0; i < num_pumps; i++) {

      hw_button[i] = pump[i].ReadButtonState();

      if ((hw_button[i] == 0) && (hw_button_old[i] == 1)) {
        Serial.println("Button: PRESSED!");
        if (pump[i].ReadPumpState()) {
          pump[i].ForceSetOff();
        } else {
          pump[i].ForceSetOn();
        }
      }

      hw_button_old[i] = hw_button[i];
    } // for i
  } // for j

  digitalWrite(LED_BUILTIN, LOW); // Set LED OFF
}


void PumpOn(void *pContext)
{
  Pump *pPump = (Pump *)pContext;

  Serial.println("PumpOn: Enter!");

  if (pPump) {
    if (pPump->SetOn()) {
      Serial.print("PumpOn: Set Alarm OFF: ");
      // Set it off after 2 min
      Alarm.timerOnce(2*60, PumpOff, pContext); // called once
    } else {
      Serial.print("PumpOn: Set On failed! ");
    }
  }

  Serial.println("PumpOn: Leave!");

}

void PumpOff(void *pContext)
{
  Pump *pPump = (Pump *)pContext;

  Serial.print("PumpOff: Enter!");

  if (pPump) {
    pPump->SetOff();
  }

  Serial.print("PumpOff: Leave!");
}

bool MQTT_Publish()
{
  float temperature = 0;
  float humidity = 0;
  float moisture = 0;
  float waterlevel = 0;
  float pumpstate = 0;
  Adafruit_MQTT_Subscribe *subscription = NULL;

  //
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  //
  if (!MQTT_connect()) {
    Serial.println("MQTT_Publish: MQTT_connect failed!");
    return false;
  }

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humidity = dht.readHumidity();

  // Read temperature as Celsius (the default)
  temperature = dht.readTemperature();

  // Publish Temperature Sensor Data
  if (!temp.publish(temperature)) {
    Serial.println("Temp publish FAILED!");
  }

  //  // Publish Humidity Sensor Data
  //  if (!humi.publish(humidity)) {
  //    Serial.println("Humidity publish FAILED!");
  //  }

  printTime(); Serial.println("");

  // Morning/Evening watering timer
  for (int i = 0; i < num_pumps; i++) {

    // Read pump sensors data
    moisture =  pump[i].ReadMoisture();
    waterlevel = pump[i].ReadWaterLevel();
    pumpstate = pump[i].ReadPumpState();

    Serial.print("\t Pump#");
    Serial.print(i);
    Serial.print(" \t");

    Serial.print("\t Soil Moisture: ");
    Serial.print(moisture);
    Serial.print("");

    Serial.print("\t Humidity: ");
    Serial.print(humidity);
    Serial.print(" %");

    Serial.print("\t Temperature: ");
    Serial.print(temperature);
    Serial.print(" *C");

    Serial.print("\t Pump State: ");
    Serial.print(pumpstate);
    Serial.println(" ");

    // Publish Moisture Sensor Data
    if (!moist[i].publish(moisture)) {
      Serial.println("Moisture publish FAILED!");
    }

    // Publish current Pump State
//    if (!pumpstatus.publish(pumpstate)) {
//      Serial.println("Pump status publish FAILED!");
//    }
  }

  // Publish current Pump State
  pumpstate = pump[0].ReadPumpState();
  if (!pumpstatus.publish(pumpstate)) {
    Serial.println("Pump status publish FAILED!");
  }

  // Read Button ON/OFF
  subscription = NULL;
  while ((subscription = mqtt.readSubscription(100))) {
    for (int i = 0; i < num_pumps; i++) {
      if (subscription == &button[i]) {
        Serial.print("Button"); Serial.print(i); Serial.print(": ");
        Serial.println((char *)button[i].lastread);

        if (strcmp((char *)button[i].lastread, "ON") == 0) {
          Serial.println("Set pump manually ON!");
          pump[i].ForceSetOn();
        }

        if (strcmp((char *)button[i].lastread, "OFF") == 0) {
          Serial.println("Set pump manually OFF!");
          pump[i].ForceSetOff();
        }
      }
    }
  } // while


  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  if (!mqtt.ping()) {
    Serial.println("MQTT ping failed!");
    mqtt.disconnect();
  }

  return true;
}


// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
bool MQTT_connect() {
  int8_t ret;

  //  Serial.println("MQTT_connect: Enter!");

  // Stop if already connected.
  if (mqtt.connected()) {
    //    Serial.println("MQTT_connect: Already connected!");
    return true;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 10;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    Alarm.delay(1000);  // wait 1 second
    Watchdog.reset();
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      Serial.println("MQTT_connect: Failed to connect!");
      while (1);
      //return false;
    }
    ret = 0;
  }

  Serial.println("MQTT_connect: OK!");
  return true;
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void printTime() {
  // Print date...
  Serial.print(day());
  Serial.print("/");
  Serial.print(month());
  Serial.print("/");
  Serial.print(year());
  Serial.print(" ");

  // ...and time
  digitalClockDisplay();
}

void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
}

void printDigits(int digits)
{
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
