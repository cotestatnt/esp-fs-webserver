# ESP32 RFID tag management system

Multi reader, multi user RFID tag mananager example.<br>
To run this example you need only a working MySQL database (remote or in your local LAN) and a RFID reader off course.

To manage RFID reader in this example, the library [MFRC522v2](https://github.com/OSSLibraries/Arduino_MFRC522v2) is used. This library can handle both SPI and I2C bus.<br>
To handle the MySQL connection istead, you will need to add also my library [Arduino-MySQL client](https://github.com/cotestatnt/Arduino-MySQL).

The pages served from ESP32 are static resources (PROGMEM flash stored literal strings):
  - `/login` prompt for user authentication
  - RFID logs and users management main page
  - `/setup` to configure DB host, database name, user, password etc etc.
  
Passwords are not stored as plain text inside DB, nor during handshaking between server and client but a SHA256 encrypted hash it's used instead. 
Since the **/login** pages can be loaded also without internet connection on a non-HTTPS server, a raw function for generate SHA256 string was added to that page. 

The SHA256 generation algorithm is freely inspired by the one available at [https://geraintluff.github.io/sha256/](https://geraintluff.github.io/sha256/)

### TO-DO:<br>
`/setup` user and password are hard coded in the ESP firmware, so a change for using admin user@password stored in DB is needed.

# Custom Login Page
![image](https://github.com/cotestatnt/esp-fs-webserver/assets/27758688/4a782808-c116-4e2a-a7dd-a87b636959ee)

# Main Web Page
![image](https://github.com/cotestatnt/esp-fs-webserver/assets/27758688/8dec7beb-aaa0-4d9d-ade1-409191cda4e6)

#

![image](https://github.com/cotestatnt/esp-fs-webserver/assets/27758688/7b6353b4-df83-4adb-8ac3-1d5a734f8ea6)



