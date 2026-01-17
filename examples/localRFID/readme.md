In this example, a simple access control system has been implemented using RFID tags and the [Arduino_MFRC522v2](https://github.com/OSSLibraries/Arduino_MFRC522v2) library. 

The system allows enabling or disabling any type of user, each of whom can be assigned a different role. 

![image](https://github.com/user-attachments/assets/5ad7afe1-2e95-4df9-8549-698a9442e01a)

For instance, modifying or deleting user accounts requires logging in with a role level of at least 5 (such as the admin user). 
Users with lower levels can still log in, but only to view data.

# Logs
![image](https://github.com/user-attachments/assets/4aab4a8a-3ce7-473c-a96b-ee79a70b7ad1)

# User management
![image](https://github.com/user-attachments/assets/b9663a4c-50f8-4a00-b26f-ab2369839ffb)

# Customization
By default, this example includes web pages embedded in the firmware as C arrays, compressed using Gzip.

To customize the appearance of the web pages, 
upload the [`login`](https://github.com/cotestatnt/async-esp-fs-webserver/blob/master/examples/localRFID/data/login) 
and [`rfid`](https://github.com/cotestatnt/async-esp-fs-webserver/blob/master/examples/localRFID/data/rfid) files (without extensions) to the microcontroller's flash memory using built-in functionality (/setup webpage). 
You can find both files in the `"/data"` folder.

Then set `#define USE_EMBEDDED_HTM false` in order to serve files webpage instead embedded.

In the [mysqlRFID.ino](https://github.com/cotestatnt/async-esp-fs-webserver/blob/master/examples/mysqlRFID/mysqlRFID.ino) example, a MySQL database is used through the [Arduino-MySQL](https://github.com/cotestatnt/Arduino-MySQL) library. 
Since the database is centralized, it supports multiple RFID tag readers, logging each access point for tracking purposes. 
The web pages in this example are embedded as `const char*` strings, making them easy to edit directly within the project.
