#include <Homie.h>
#include "DHT.h"
#include <Adafruit_BMP085.h>


const int DEFAULT_TEMPERATURE_INTERVAL = 300;
//DHT dht(D4, DHT22);
DHT dht(D4, DHT11);
unsigned long lastTemperatureSent = 0, lastHumiditySent = 0, lastVoltSent = 0;
float temperature = 0, humidity = 0;


HomieNode temperatureNode("temperature", "temperature");
HomieNode humidityNode("humidity", "humidity");
HomieSetting<long> temperatureIntervalSetting("temperatureInterval", "The temperature read interval in seconds");
HomieSetting<long> humidityIntervalSetting("humidityInterval", "The humidity read interval in seconds");


void setupHandler() {
  temperatureNode.setProperty("unit").send("c");
  humidityNode.setProperty("unit").send("%");
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
    temperature_humidity_metering();
    Homie.getLogger() << "Temperature: " << temperature << " °C" << endl;
    Homie.getLogger() << "Humidity: " << humidity << " %" << endl;
    temperatureNode.setProperty("degrees").send(String(temperature));
    humidityNode.setProperty("percent").send(String(humidity));
    lastTemperatureSent = millis();
    lastHumiditySent = millis();
  }
}

void setup() {
  Homie.disableLedFeedback();
  Serial.begin(115200);
  Serial << endl << endl;
  Homie_setFirmware("SensorNode", "1.0.3");
  Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);
  temperatureNode.advertise("unit");
  temperatureNode.advertise("degrees");
  humidityNode.advertise("unit");
  humidityNode.advertise("percent");
  temperatureIntervalSetting.setDefaultValue(DEFAULT_TEMPERATURE_INTERVAL).setValidator([] (long candidate) {
    return candidate > 0;
  });

  Homie.setup();
}

void loop() {
  Homie.loop();
}
