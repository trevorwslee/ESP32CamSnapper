========== WORK IN PROGRESS ==========
---
title: Turn ESP32-CAM into a Snapshot Taker, for Selfies and Time Lapse Pictures
description: Turn ESP32-CAM (or equivalent) into a Android phone managed snapshot taker, for snapshots like selfies and time-lapse pictures
tags: 'esp32cam, snapshots'
cover_image: ./imgs/MAIN.jpg
---


# Turn ESP32-CAM into a Snapshot Taker, for Selfies and Time Lapse Pictures


This project is an attempt to turn ESP32-CAM -- or equivalent like LILYGO T-Camera / T-Camera Plus -- microcontroller board into an Android phone managed snapshot taker, for snapshots like selfies and time-lapse pictures ... maybe ... just for fun!

> With your Android phone, connect to the ESP32-CAM, make your post and smile.
>  Then go back to your phone to select the pictures you like best and save to your phone directly.

> With your Android phone, connect to the ESP32-CAM, set how frequently a snapshot is to be taken.
>  Then disconnect the ESP32-CAM and let it does its job of taking time-lapse pictures.
>  Then reconnect to the ESP32-CAM to transfer the taken pictures to your phone.

The microcontroller program (sketch) is developed with Arduino framework using VS Code and PlatformIO, in the similar fashion as described by the post -- [A Way to Run Arduino Sketch With VSCode PlatformIO Directly](https://www.instructables.com/A-Way-to-Run-Arduino-Sketch-With-VSCode-PlatformIO/)

The remote UI -- control panel -- is realized with the help of the DumbDisplay Android app. You have a choice of using Bluetooth (classic) or WiFi for the connection. For a brief description of DumbDisplay, please refer to the post -- [Blink Test With Virtual Display, DumbDisplay](https://www.instructables.com/Blink-Test-With-Virtual-Display-DumbDisplay/)

Please note that the UI is driven by the sketch; i.e., the flow of the UI is programmed in the sketch. No building of an separate mobile app is necessary, other than installing the DumbDisplay Android app.

# The UI

## Ways of taking Snapshots

There are 3 ways snapshots can be captured and saved to your phone

1) With your phone connected to the ESP32-CAM -- certainly through the DumbDisplay Android app, with Bluetooth or WiFi -- you click the ***💾Save*** button of the UI to save the image being shown. You can turn on / off the auto-save feature by clicking ***Auto❎*** / ***Auto☑️***. With auto-save enabled, whenever a snapshot is captured from the ESP32-CAM, it will be saved to your phone automatically.

2) Assuming you have connected your phone with the ESP32-CAM, clicking on the image canvas will pause capturing snapshots, and you will be provided with a slider to slide back in time to select the previously captured snapshots to save to your phone. You will have 20 such snapshots that you can slide back in time to. Once you see the snapshots you like, click ***💾Save*** to save it. When you are done, you can go back by clicking ***❌Cancel*** (or double click on the image canvas).

3) Select ***Offline📴*** to enable "offline" capturing and saving of snapshot to the ESP32-CAM. With "offline" enabled, when you disconnect (i.e. offline), the ESP32-CAM will start capturing "offline" snapshots saving them to its flash memory (or SD card). Whenever you reconnect, you will be asked if you want to transfer the saved "offline" snapshots to your phone. (Or in SD card case, you can physically transfer the "offline" snapshots saved the usual way -- plug the SD card to your computer)

The snapshots will be saved in ***JPEG*** format:

* Snapshots saved to your phone will be saved to the private folder of DumbDisplay app - `/<main-storage>/Android/data/nobody.trevorlee.dumbdisplay/files/Pictures/DumbDisplay/snaps/` -- with file name like `20240904_223249_001.jpg` (`YYYYMMDD-hhmmss_seq.jpg`)
* Offline snapshots transferred to your phone will be save to a subfolder of the above mentioned private folder of DumbDisplay app. The name of the subfolder depends on the time you do the snapshots transfer, and is like `20240904_231329_OFF`.
* Offline snapshots are saved to your ESP32-CAM's flash memory / SD card with name like `off_001.jpg`. Note that the "offline" ***JPEG*** files will only be properly time-stamped if there was no reset / reboot of ESP32-CAM after connecting to your phone.


## Frequency of Capturing Snapshots

Snapshot capturing is continuous but not smooth. Indeed, there is a setting on how frequent a snapshot is captured and shipped to your phone. Depending on the resolution / quality of the snapshot and the connection method / condition, it can be as frequent as 5 snapshots per second. (Treat this frequency control as a feature that allows taking of time-lapse pictures at a desired frequency 😁)

There are 4 quick selections of frequency / frame rate -- ***5 PS*** / ***2 PS*** / ***1 PS*** / ***30 PM*** corresponding to *5 frames per second* / *2 frames per second* / *1 frame per second* / *30 frames per minute*

And there is a custom per-hour frame rate you can set and select -- ***720 PH*** (720 is the default). Once you click on the ***720 PH*** selection, your phone's virtual keyword will popup allowing you to enter the value (1 - 3600) you want. 


## Snapshots Quality Adjustments

There are several camera adjustments that will affect the quality of the captured snapshots:
* Snapshot resolution / size selections:
  - ***QVGA*** (320x240)
  - ***VGA*** (640x480)
  - ***SVGA*** (800x600)
  - ***XGA*** (1024x768)
  - ***HD*** (1280x720)
  - ***SXGA*** (1280x1024)
  - ***UXGA*** (1600x1200)
* ***JPEG*** compression quality slider 🖼️ -- from 5 (high quality) to 60 (low quality)
* Brightness slider ☀️ -- from -2 (dim) to 2 (bright)

Note that the higher the snapshots quality / resolution, the more data will be shipped from the ESP32-CAM to your phone, making the UI less responsive, especially when not using Bluetooth.

## ESP32 Idle Sleep

Another feature is that after a while (60 seconds) of being idle (i.e. not connected), the ESP32-CAM will be put to sleep mode in order to save power, this applies to whether "offline" snapshots taking is enabled or not. Of course, if "offline" capturing is enabled, the ESP32-CAM will wake up in due time to take a snapshot. (Note that if "offline" frequency is too high, higher than 12 frames per minute, the ESP32-CAM will stay on.)

In any case, ***if you want to connect to the ESP32-CAM when it is in sleep mode, you will need to reset the ESP32-CAM first***


# Developing and Building

As mentioned previously, the sketch will be developed using VS Code and PlatformIO.
Please clone the PlatformIO project [ESP32CamSnapper](https://github.com/trevorwslee/ESP32CamSnapper) GitHub repository.

The configurations for developing and building of the sketch in basically captured in the `platformio.ini` file
```
[env]
monitor_speed = 115200

[env:ESP32CAM]
platform = espressif32
board = esp32cam
framework = arduino
lib_deps =
    https://github.com/trevorwslee/Arduino-DumbDisplay
build_flags =
    -D FOR_ESP32CAM

[env:TCAMERA]  ; v7
platform = espressif32
board = esp-wrover-kit
framework = arduino
board_build.partitions = huge_app.csv
lib_deps =
    https://github.com/trevorwslee/Arduino-DumbDisplay
build_flags =
    -D BOARD_HAS_PSRAM
    -D FOR_TCAMERA

[env:TCAMERAPLUS]
platform = espressif32
board = esp32dev
framework = arduino
board_build.partitions = huge_app.csv
lib_deps =
    https://github.com/trevorwslee/Arduino-DumbDisplay
build_flags =
    -D BOARD_HAS_PSRAM
    -D FOR_TCAMERAPLUS
```

***Please make sure you select the correct PlatformIO project environment*** -- `ESP32CAM` / `TCAMERA` / `TCAMERAPLUS`

The program entry point is `src/main.cpp`
```
// *** use Bluetooth with the following device name
#define BLUETOOTH "ESP32CamSnapper"
#include "esp32camsnapper/esp32camsnapper.ino"
```

The above `main.cpp` assumes that you prefer to use Bluetooth (classic), and defines the Bluetooth device name to be ***ESP32CamSnapper*** like
```
#define BLUETOOTH "ESP32CamSnapper"
```
Note that using Bluetooth (classic) is the suggested way of connecting with DumbDisplay app.

However, if you prefer to use WiFi, you will need to define `WIFI_SSID` and `WIFI_PASSWORD` rather than `BLUETOOTH`
```
//#define BLUETOOTH "ESP32CamSnapper"
#define WIFI_SSID           "your-wifi-ssid"
#define WIFI_PASSWORD       "your-wifi-password"
```

In case of WiFi, you will need to find out the IP of your ESP32-CAM in order to make connect to it. This can be done easily by attaching it to a Serial monitor (set to baud-rate 115200). There, when your ESP32-CAM tries to connect to DumbDisplay app, log lines like below will be printed:
```
binded WIFI TrevorWireless
listening on 192.168.0.155:10201 ...
listening on 192.168.0.155:10201 ...
listening on 192.168.0.155:10201 ...
```
That IP address is the IP address of your ESP32-CAM to make connection to.

If you have SD card installed for ESP32-CAM or LILYGO T-Camera Plus, you can use it to store "offline" snapshots, rather than the microcontroller's flash memory.

To configure the sketch for such setup, you will need to define the macro `OFFLINE_USE_SD`, like by uncomment the corresponding line of the sketch
```
// *** uncomment the following if offline snaps are to be stored to SD (ESP32CAM / TCameraPlus), rather than LittleFS
#define OFFLINE_USE_SD 
```

or put the `#define` line in `main.cpp` like
```
#define OFFLINE_USE_SD 
#define BLUETOOTH "ESP32CamSnapper"
#include "esp32camsnapper/esp32camsnapper.ino"
```

# The Sketch

The sketch of the project is `esp32camsnapper/esp32camsnapper.ino`. There are several customizations you may want to make:

* By default, you have 20 snapshots you can go back in time to. This number is controlled by the macro `STREAM_KEEP_IMAGE_COUNT`
  ```
  // *** maximum number of images to keep cached (on app side) when streaming snaps to DD app; this number affects how much you can go back to select which image to save
  #define STREAM_KEEP_IMAGE_COUNT             20
  ```

* By default, you can enable "offline" snapshot capturing. If you don't want "offline" at all, you can comment out the corresponding `#define`
  ```
  // *** support offline snap taking; comment out if not desired
  #define SUPPORT_OFFLINE
  ```

* By default, the ESP32-CAM will be put to sleep after 60 seconds being idle (i.e. not connected). You can change the 60 to other value by changing the macro `IDLE_SLEEP_SECS`. If this 'idle put to sleep' behavior is not desirable, you can comment out that `#define` to disable such behavior 
  ```
  // *** number of seconds to put ESP to sleep when idle (not connected); if ESP went to sleep, will need to reset it in order to connect; comment out if not desired
  #define IDLE_SLEEP_SECS                     60
  ```  

* Most settings you change in the UI will be persisted to the EEPROM of the ESP32-CAM. These settings will be effective even you re-upload the program built. In case you want to "reset" those settings, change the `HEADER` to some other value, build and re-upload
  ```
  // *** if want to reset settings / offline snap metadata saved in EEPROM, set the following to something else
  const int32_t HEADER = 20240907;
  ```  

# Build and Upload

Build, upload the sketch and try it out! 

Use a Serial monitor (set baud-rate to 115200) to observe when things are happening

```
Initialize offline snap storage ...
... offlineSnapCount=3 ...
... existing STORAGE_FS ...
    $ total: 896 KB
    $ used: 180 KB
    $ free: 716 KB (79.91%)
... offline snap storage ...
    - startOfflineSnapIdx=0
    - offlineSnapCount=3
... offline snap storage initialized
bluetooth address: 84:0D:8E:D2:90:EE
*** just became idle ... millis=1067 ***
bluetooth address: 84:0D:8E:D2:90:EE
bluetooth address: 84:0D:8E:D2:90:EE
bluetooth address: 84:0D:8E:D2:90:EE
...
**********
* _SendBufferSize=128
**********
*** just connected ... millis=7133 ***
- got sync time 2024-09-07-10-17-41-+0800
  . tz_mins=480
Initializing camera ...
... initialized camera!
...
*** just became idle ... millis=93898(Saturday, September 07 2024 10:19:07+0800) ***
bluetooth address: 84:0D:8E:D2:90:EE
! written offline snap to [/off_000.jpg] ... size=48.39KB
bluetooth address: 84:0D:8E:D2:90:EE
bluetooth address: 84:0D:8E:D2:90:EE
...
*** going to sleep ... millis=153898(Saturday, September 07 2024 10:20:07+0800) ***
...
Initialize offline snap storage ...
... offlineSnapCount=2 ...
... existing STORAGE_FS ...
    $ total: 896 KB
    $ used: 112 KB
    $ free: 784 KB (87.50%)
... offline snap storage ...
    - startOfflineSnapIdx=0
    - offlineSnapCount=2
... offline snap storage initialized
*** woke up for offline snap ... millis=542(Saturday, September 07 2024 10:20:33+0800) ***
! written offline snap to [/off_002.jpg] ... size=78.91KB
*** going back to sleep ... timeout=27.20s ***
...
```


# Enjoy!

Try it! Hope that you will have fun with it! Enjoy!

> Peace be with you!
> May God bless you!
> Jesus loves you!
> Amazing Grace!

