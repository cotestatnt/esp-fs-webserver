
# esp-fs-webserver
From FSBrowser.ino example to an Arduino library

When you need a webserver running on ESP32 or ESP8266 device, [FSBrowser@ESP8266](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/examples/FSBrowser) or [FSBrowser@ESP32](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer/examples/FSBrowser) is the best examples as start point because you can simply put your webserver HTML/CSS/JavaScript source files in the flash memory of device (for example with filesystem-type dedicated plugins) or in an external SD.

Unfortunately are a little complex examples for a novice due to a lot of handling functions "under the cover". 
This library makes it easier by incorporating this functions into a specific class and more, it adds some convenience functionality like WiFi / Custom Options manager.


## ACE web broswer editor
Thanks to the built-in **/edit** page (enabled by default), is possible upload, delete and edit the HTML/CSS/JavaScript source files directly from browser and immediately displaying the changes introduced at runtime without having to recompile the device firmware.

![editor](https://user-images.githubusercontent.com/27758688/122570105-b6a01080-d04b-11eb-832c-f60c0a886efd.png)



## WiFi manager
I've added also another built-in page **/setup**, also enabled by default, with which it is possible to set the WiFi credentials and other freely configurable parameters.

![image](https://user-images.githubusercontent.com/27758688/122721275-c187ba80-d270-11eb-9960-9b21fe40fe95.png)
