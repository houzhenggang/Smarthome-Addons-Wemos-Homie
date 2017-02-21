
/*

  The MIT License (MIT)

  Copyright (c) 2017 HOLGER IMBERY, CONTACT@CONNECTEDOBJECTS.CLOUD

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <Homie.h>
#include "DHT.h"
#include <Adafruit_BMP085.h>
#include <Adafruit_SSD1306.h>
// Remove "//" in the following line to use with battery shield and deepSleep
//#define WITH_BATTERY
// Remove "//" once to clean up spiffs
//#define CLEAN_UP

const int DEFAULT_TEMPERATURE_INTERVAL = 300;

// choose correct sensor and pin
DHT dht(D4, DHT22);
//DHT dht(D4, DHT11);
unsigned long lastTemperatureSent = 0, lastHumiditySent = 0;
float temperature = 0, humidity = 0;
#ifdef WITH_BATTERY
  unsigned long lastVoltSent = 0;
  float volt = 0;
#endif


HomieNode temperatureNode("temperature", "temperature");
HomieNode humidityNode("humidity", "humidity");
#ifdef WITH_BATTERY
  HomieNode voltNode("volt", "volt");
#endif
HomieSetting<long> temperatureIntervalSetting("temperatureInterval", "The temperature read interval in seconds");
#ifdef WITH_BATTERY
  HomieSetting<long> voltIntervalSetting("voltInterval", "The voltage read interval in seconds");
#endif

void setupHandler() {
  temperatureNode.setProperty("unit").send("c");
  humidityNode.setProperty("unit").send("%");
  #ifdef WITH_BATTERY
    voltNode.setProperty("unit").send("V");
  #endif
}

void voltage_metering() {
  #ifdef WITH_BATTERY
    int sensorValue = analogRead(A0);
    volt = sensorValue / 196.5;
  #endif
}

void temperature_humidity_metering() {
  int retry_limit = 50;
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  while ((isnan(temperature) || isnan(humidity))
         || (temperature == 0 && humidity == 0)) {
      if (--retry_limit < 1) {
          temperature = -1;
          humidity = -1;
          return;
      }
      delay(500);
      temperature = dht.readTemperature();
      humidity = dht.readHumidity();
   }
}


void loopHandler() {
  if (millis() - lastTemperatureSent >= temperatureIntervalSetting.get() * 1000UL || lastTemperatureSent == 0) {
    delay(6000);
    #ifdef WITH_BATTERY
      voltage_metering();
    #endif
    temperature_humidity_metering();
    Homie.getLogger() << "Temperature: " << temperature << " °C" << endl;
    Homie.getLogger() << "Humidity: " << humidity << " %" << endl;
    #ifdef WITH_BATTERY
      Homie.getLogger() << "Volt: " << volt << " V" << endl;
    #endif
    temperatureNode.setProperty("degrees").send(String(temperature));
    humidityNode.setProperty("percent").send(String(humidity));
    #ifdef WITH_BATTERY
      voltNode.setProperty("volt").send(String(volt));
    #endif
    lastTemperatureSent = millis();
    lastHumiditySent = millis();
    #ifdef WITH_BATTERY
      lastVoltSent = millis();
      delay(6000);
      ESP.deepSleep(900 * 1000000, WAKE_RF_DEFAULT);
    #endif
  }
}

void setup() {
  Serial.begin(115200);
  Serial << endl << endl;
  Homie_setFirmware("SensorNode", "1.1.1");
  Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);
  temperatureNode.advertise("unit");
  temperatureNode.advertise("degrees");
  humidityNode.advertise("unit");
  humidityNode.advertise("percent");
  #ifdef WITH_BATTERY
    voltNode.advertise("unit");
    voltNode.advertise("volt");
  #endif
  temperatureIntervalSetting.setDefaultValue(DEFAULT_TEMPERATURE_INTERVAL).setValidator([] (long candidate) {
    return candidate > 0;
  });
  #ifdef CLEAN_UP
    Homie.reset();
  #endif
  Homie.setup();
}

void loop() {
  delay(6000);
  Homie.loop();
}
