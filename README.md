# SmartHome-Addons-WeMos-Homie
**Addons for SmartHome Controllers** based on WeMos D1 mini (or similar ESP8266 based development boards)

with this code and some hardware (cheap development boards and shields), you are able to create *connectedobjects* for any mqtt based SmartHome Controller. These __connectedobjects__ are small IoT bricks offering additional actuators or sensor funcionality to your SmartHome Controller.

----
available objects:

1. __connectedobject SensorNode__,
a temperature/humidity sensor, mqtt enabled, OTA, (option: wemos battery shield with voltage control mod (need to solder) and deepsleep) and captive portal to configure.

2. __connectedobject DisplaySensorNode__,
a temperature/humidity sensor, mqtt enabled, 0.96" OLED Display (I2C or SPI), OTA and captive portal to configure.


----

Please note:

you have to solder a little bit for voltage metering and deepsleep :-).

1. deepsleep - bridge D0 & RST

2. voltage metering - 180k Ohm resistor between WeMos BatteryShield +Vbat & A0
