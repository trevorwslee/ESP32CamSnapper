========== WORK IN PROGRESS ==========
---
title: Turn ESP32-CAM into a Snapshot Taker, for Selfies and Time Lapse Pictures
description: Turn ESP32-CAM (or equivalent) into a Android phone managed snapshot taker, for snapshots like selfies and time-lapse pictures
tags: 'esp32cam, snapshots'
cover_image: ./imgs/MAIN.jpg
---


# Turn ESP32-CAM into a Snapshot Taker, for Selfies and Time Lapse Pictures


The purpose of this project is to try to turn ESP32-CAM -- or equivalent like LILYGO T-Camera / T-Camera Plus -- microcontroller board into an Android phone managed snapshot taker, for snapshots like selfies and time-lapse pictures ... maybe ... just for fun!

> With your Android phone, connect to the ESP32-CAM, make your post and simile.
>  Then go back to your phone to select the pictures you like best and save to your phone directly.

> With your Android phone, connect to the ESP32-CAM, set how frequently a snapshot is to be taken.
>  Then disconnect the ESP32-CAM and let it does its job of taking time-lapse pictures.
>  Then reconnect to the ESP32-CAM to transfer the taken pictures to your phone.

The microcontroller program (sketch) is developed with Arduino framework using VS Code and PlatformIO, in the similar fashion as described by the post -- [A Way to Run Arduino Sketch With VSCode PlatformIO Directly](https://www.instructables.com/A-Way-to-Run-Arduino-Sketch-With-VSCode-PlatformIO/)

The remote UI -- control panel -- is realized with the help of the DumbDisplay Android app. For a brief description of DumbDisplay, please refer to the post -- [Blink Test With Virtual Display, DumbDisplay](https://www.instructables.com/Blink-Test-With-Virtual-Display-DumbDisplay/)

Please note that the UI is driven by the microcontroller program; i.e., the control flow of the UI is programmed in the sketch. No building of a mobile app is necessary, other than installing the DumbDisplay Android app.

# The UI

## Ways of taking Snapshots

There are several ways snapshot can be captured and saved to your phone

1) With your phone connected to your ESP32-CAM, certainly with Android DumbDisplay app, you click ***💾Save*** to save the image being shown. You can turn on / off the auto-save feature by clicking ***Auto❎*** / ***Auto☑️***. With auto-save enabled, whenever a snapshot is captured from your ESP32-CAM, it will be saved to your phone automatically.

2) Assuming you have connected your phone with your ESP32-CAM, clicking on the image canvas will pause capturing snapshots, and you will be provided with a slider to slide back in time to select the previously captured snapshots to save to your phone. You will have 20 such snapshots that you can slide back in time to. Once you see the snapshots you like, click ***💾Save*** to save it. When you are done, you can go back by clicking ***❌Cancel*** (or double click on the image canvas).

3) Select ***Offline📴*** to enable "offline" capturing and saving of snapshot to your ESP32-CAM. With "offline" enabled, when you disconnect (i.e. offline), your ESP32-CAM will start capturing "offline" snapshots saving them to its flash memory (or SD card). Whenever you reconnect, you will be asked if you want to transfer the saved "offline" snapshots to your phone. (Or in SD card case, you can physically transfer the "offline" snapshots saved the usual way -- plug the SD card to your computer)

The snapshots will be saved in ***JPEG*** format:

* Snapshots saved to your phone will be saved to the private folder of DumbDisplay app - `/<main-storage>/Android/data/nobody.trevorlee.dumbdisplay/files/Pictures/DumbDisplay/snaps/` -- with file name like `20240904_223249_001.jpg` (`YYYYMMDD-hhmmss_seq.jpg`)
* Offline snapshots transferred to your phone will be save to a subfolder of the above mentioned private folder of DumbDisplay app. The name of the subfolder depends on the time you do the snapshots transfer, and is like `20240904_231329_OFF`.
* Offline snapshots are saved to your ESP32-CAM's flash memory / SD card with name like `off_001.jpg`. Note that the "offline" ***JPEG*** files will only be properly time-stamped if there was no reset / reboot of ESP32-CAM after connection to your phone.


## Frequency of Capturing Snapshots

Snapshot capturing is continuous but not smooth. Indeed, there is a setting on how frequent a snapshot is captured and shipped to your phone. Depending on the resolution / quality of the snapshot and the connection method / condition, it can be as frequent as 5 snapshots per second.

Nevertheless, treat this frequency control as a feature, that allows taking of time-lapse pictures at a desired frequency 😁

There are 4 quick selections of frequency / frame rate -- ***5 PS*** / ***2 PS*** / ***1 PS*** / ***30 PM*** corresponding to *5 frames per second* / *2 frames per second* / *1 frame per second* / *30 frame per minute*

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
* Brightness slider ☀️ -- from -1 (dim) to 1 (bright)
* ***JPEG*** compression quality slider 🖼️ -- from 5 (high quality) to 60 (low quality)


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

#if defined(FOR_ESP32CAM)
  // *** ESP32CAM has SD (but not used)
  //#define OFFLINE_USE_SD
#elif defined(FOR_TCAMERAPLUS)
  // *** LILYGO TCameraPlus has SD
  #define OFFLINE_USE_SD
#endif

#include "esp32camsnapper/esp32camsnapper.ino"
```

The above `main.cpp` assumes that you prefer to use Bluetooth, and defines the Bluetooth device name to be ***ESP32CamSnapper*** like
```
#define BLUETOOTH "ESP32CamSnapper"
```
Note that using Bluetooth is the suggested way for this sketch.

However, if you prefer to use WiFi, you will need to define `WIFI_SSID` and `WIFI_PASSWORD` rather than `BLUETOOTH`
```
//#define BLUETOOTH "ESP32CamSnapper"
#define WIFI_SSID           "your-wifi-ssid"
#define WIFI_PASSWORD       "your-wifi-password"
```

In case of WiFi, you will need to find out the IP of your ESP32-CAM in order to make connect to it. This can be done easily by attaching it to a Serial monitor (set to baud 115200). There, when your ESP32-CAM tries to connect to DumbDisplay app, log lines like below will be printed:
```
binded WIFI TrevorWireless
listening on 192.168.0.155:10201 ...
listening on 192.168.0.155:10201 ...
listening on 192.168.0.155:10201 ...
```
That IP address is the IP address of your ESP32-CAM to make connection to.


