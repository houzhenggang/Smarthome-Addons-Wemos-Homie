#include <Homie.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "DHT.h"
#include <Adafruit_BMP085.h>
#include <Adafruit_SSD1306.h>
// remove once to clean up spiffs
//#define CLEAN_UP
//choose correct display
//#define I2C_DISPLAY
#define SPI_DISPLAY

const int DEFAULT_TEMPERATURE_INTERVAL = 300;

#define PIN_LED D0
DHT dht(D4, DHT22);
//DHT dht(D1, DHT11);
unsigned long lastTemperatureSent = 0, lastHumiditySent = 0, lastVoltSent = 0;
float temperature = 0, humidity = 0;
#ifdef I2C_DISPLAY
  U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, /* clock=*/ D3, /* data=*/ D2, /* reset=*/ U8X8_PIN_NONE);
#endif
#ifdef SPI_DISPLAY
  U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI display(U8G2_R0, /* clock=*/ D6, /* data=*/ D7, /* cs=*/ D2, /* dc=*/ D5, /* reset=*/ D8);
#endif
String display_temp;
String display_humid;

HomieNode displayNode("display", "display");
HomieNode temperatureNode("temperature", "temperature");
HomieNode humidityNode("humidity", "humidity");
HomieSetting<long> temperatureIntervalSetting("temperatureInterval", "The temperature read interval in seconds");
HomieSetting<const char*> monitorTopicSetting("monitorTopic", "Topic to subscribe to");
AsyncMqttClient& mqttClient = Homie.getMqttClient();


void setupHandler() {
  temperatureNode.setProperty("unit").send("c");
  humidityNode.setProperty("unit").send("%");

}

void temperature_humidity_metering() {
  int retry_limit = 50;
  humidity = dht.readHumidity();
  Serial.println(humidity);
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
      display_temp = String(temperature);
      display_humid = humidity;
   }
}

void displayHandler() {
  display.setFont(u8g2_font_helvB08_tf);
  String name = String(Homie.getConfiguration().name);
  int name_len = name.length() + 1;
  char name_array[name_len];
  name.toCharArray(name_array, name_len);
  display.drawStr(0,12,name_array);
  display.setFont(u8g2_font_helvR10_tf);
  int temperature_len = display_temp.length() + 1;
  char temperature_array[temperature_len];
  display_temp.toCharArray(temperature_array, temperature_len);
  int humidity_len = display_humid.length() + 1;
  char humidity_array[humidity_len];
  display_humid.toCharArray(humidity_array, humidity_len);
  display.drawStr(0,35,temperature_array);
  display.drawStr(38,35,"C");
  display.drawStr(64,35,humidity_array);
  display.drawStr(105,35,"%");
  display.sendBuffer();
}

void loopHandler() {
  displayHandler();
  if (millis() - lastTemperatureSent >= temperatureIntervalSetting.get() * 1000UL || lastTemperatureSent == 0) {
    delay(6000);
    temperature_humidity_metering();
    Homie.getLogger() << "Temperature: " << temperature << " °C" << endl;
    Homie.getLogger() << "Humidity: " << humidity << " %" << endl;
    temperatureNode.setProperty("degrees").send(String(temperature));
    humidityNode.setProperty("percent").send(String(humidity));
    lastTemperatureSent = millis();
    lastHumiditySent = millis();
  }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("** Publish received **");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("payload: ");
  Serial.println(payload);
  display.clearBuffer();
  display.drawLine(0,45,128,45);
  display.drawStr(0,64, payload);
  display.drawStr(28,64, "  C");
  display.sendBuffer();
}

void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
   case HomieEventType::MQTT_CONNECTED:
    uint16_t packetIdSub = mqttClient.subscribe(monitorTopicSetting.get(), 0);
    Serial.print("Subscribing at QoS 0, packetId: ");
    Serial.println(packetIdSub);
    break;
  }
}

void setup() {
  Serial.begin(115200);
  #ifdef I2C_DISPLAY
    pinMode(D4, OUTPUT);
    digitalWrite(D4, HIGH);
  #endif
  delay(500);
  display.begin();
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tf);
  Homie_setFirmware("DisplaySensorNode", "1.1.0");
  Homie.setLedPin(PIN_LED, LOW);
  Homie.onEvent(onHomieEvent);
  mqttClient.onMessage(onMqttMessage);
  Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);
  temperatureNode.advertise("unit");
  temperatureNode.advertise("degrees");
  humidityNode.advertise("unit");
  humidityNode.advertise("percent");
  temperatureIntervalSetting.setDefaultValue(DEFAULT_TEMPERATURE_INTERVAL).setValidator([] (long candidate) {
    return candidate > 0;
  });
  #ifdef CLEAN_UP
    Homie.reset();
  #endif
  Homie.setup();
}

void loop() {
   Homie.loop();
}
