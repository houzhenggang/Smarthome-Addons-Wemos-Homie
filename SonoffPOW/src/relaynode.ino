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
//
#include <ArduinoOTA.h>
#include "power.h"
//

#define PIN_RELAY 12
#define PIN_LED 15
#define PIN_BUTTON 0
#define INTERVAL 60
ESP8266PowerClass power_read;
//#define CLEAN_UP
const int powerIntervalSetting = 10;
unsigned long lastSent = 0;
int relayState = LOW;
bool stateChange = false;
double voltage = 0, power = 0;
unsigned long lastPowerSent = 0, lastVoltageSent = 0;

int buttonState;
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

HomieNode switchNode("switch", "switch");
HomieNode powerNode("power", "power");
HomieNode voltageNode("voltage", "voltage");

bool switchHandler(HomieRange range, String value) {
  if (value == "true") {
    digitalWrite(PIN_RELAY, HIGH);
    switchNode.setProperty("on").send("true");
    Serial.println("Switch is on");
  } else if (value == "false") {
    digitalWrite(PIN_RELAY, LOW);
    switchNode.setProperty("on").send("false");
    Serial.println("Switch is off");
  } else {
    return false;
  }
  return true;
}

void getPower() {
  power = power_read.getPower();
  voltage = power_read.getVoltage();
}

void setupHandler() {
  powerNode.setProperty("unit").send("W");
  voltageNode.setProperty("unit").send("V");

}

void loopHandler() {
  if (millis() - lastPowerSent >= powerIntervalSetting * 1000UL || lastPowerSent == 0) {
    delay(6000);
  getPower();
  Homie.getLogger() << "Power: " << power << " W" << endl;
  Homie.getLogger() << "Voltage: " << voltage << " V" << endl;
  powerNode.setProperty("power").send(String(power));
  voltageNode.setProperty("voltage").send(String(voltage));
  lastVoltageSent = millis();
  lastPowerSent = millis();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);
  power_read.enableMeasurePower();
  power_read.selectMeasureCurrentOrVoltage(VOLTAGE);
  power_read.startMeasure();

  Homie_setFirmware("SonoffPOW", "1.1.4ota");
  Homie.setLedPin(PIN_LED, LOW).setResetTrigger(PIN_BUTTON, LOW, 5000);

  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);

  switchNode.advertise("on").settable(switchHandler);
  powerNode.advertise("unit");
  powerNode.advertise("watt");
  voltageNode.advertise("unit");
  voltageNode.advertise("volt");
  #ifdef CLEAN_UP
    Homie.reset();
  #endif
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  Homie.setup();
}

void loop() {
  ArduinoOTA.handle();
  int reading = digitalRead(PIN_BUTTON);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
       if (buttonState == HIGH) {
        stateChange = true;
        relayState = !relayState;
      }
    }
  }
  lastButtonState = reading;
  if (stateChange) {
    digitalWrite(PIN_RELAY, relayState);
    digitalWrite(PIN_LED, !relayState);
    switchNode.setProperty("on").send((relayState == HIGH)? "true" : "false");
    stateChange = false;
  }
  Homie.loop();
}
