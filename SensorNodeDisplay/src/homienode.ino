#include <Homie.h>
#include "DHT.h"
#include <Adafruit_BMP085.h>
#include <Adafruit_SSD1306.h>
//#define FORCE_DEEPSLEEP

const int DEFAULT_TEMPERATURE_INTERVAL = 300;
DHT dht(D4, DHT22);
//DHT dht(D4, DHT11);
unsigned long lastTemperatureSent = 0, lastHumiditySent = 0, lastVoltSent = 0;
float temperature = 0, humidity = 0, volt = 0;
#define OLED_MOSI  13
#define OLED_CLK   12
#define OLED_DC    14
#define OLED_CS    16
#define OLED_RESET 15
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
String display_temp;
String display_humid;


HomieNode temperatureNode("temperature", "temperature");
HomieNode humidityNode("humidity", "humidity");
HomieNode voltNode("volt", "volt");
HomieSetting<long> temperatureIntervalSetting("temperatureInterval", "The temperature read interval in seconds");
HomieSetting<long> humidityIntervalSetting("humidityInterval", "The humidity read interval in seconds");
HomieSetting<long> voltIntervalSetting("voltInterval", "The voltage read interval in seconds");


void setupHandler() {
  temperatureNode.setProperty("unit").send("c");
  humidityNode.setProperty("unit").send("%");
  voltNode.setProperty("unit").send("V");

}

void voltage_metering() {
  int sensorValue = analogRead(A0);
  volt = sensorValue / 196.5;
}

void temperature_humidity_metering() {
  int retry_limit = 50;
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  display_temp = temperature;
  display_humid = humidity;
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

void displayData() {
    display.display();
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(6, 1);
    display.println("Temperature/Humidity");
    display.setTextSize(1);
    display.setCursor(6, 11);
    display.println(" ------------------ ");
    display.setCursor(15, 20);
    display.println(display_temp + " C   "+ display_humid + " %");
    }

void loopHandler() {
  displayData();
  if (millis() - lastTemperatureSent >= temperatureIntervalSetting.get() * 1000UL || lastTemperatureSent == 0) {
    delay(6000);
    voltage_metering();
    temperature_humidity_metering();
    Homie.getLogger() << "Temperature: " << temperature << " °C" << endl;
    Homie.getLogger() << "Humidity: " << humidity << " %" << endl;
    Homie.getLogger() << "Volt: " << volt << " V" << endl;
    temperatureNode.setProperty("degrees").send(String(temperature));
    humidityNode.setProperty("percent").send(String(humidity));
    voltNode.setProperty("volt").send(String(volt));
    lastTemperatureSent = millis();
    lastHumiditySent = millis();
    lastVoltSent = millis();
    #ifdef FORCE_DEEPSLEEP
      delay(6000);
      ESP.deepSleep(900 * 1000000, WAKE_RF_DEFAULT);
    #endif
  }
}

void setup() {
  Serial.begin(115200);
  Serial << endl << endl;
  Homie_setFirmware("SensorNodeDisplayD22", "1.0.3");
  Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);
  temperatureNode.advertise("unit");
  temperatureNode.advertise("degrees");
  humidityNode.advertise("unit");
  humidityNode.advertise("percent");
  voltNode.advertise("unit");
  voltNode.advertise("volt");
  temperatureIntervalSetting.setDefaultValue(DEFAULT_TEMPERATURE_INTERVAL).setValidator([] (long candidate) {
    return candidate > 0;
  });
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();
  Homie.setup();
}

void loop() {
  delay(6000);
  Homie.loop();
}
