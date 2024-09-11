---
title: 'Turn ESP32-CAM into a Snapshot Taker, for Selfies and Time-Lapse Pictures'
description: 'Turn ESP32-CAM (or equivalent) into an Android phone-managed snapshot taker, for snapshots like selfies and time-lapse pictures'
tags: 'esp32cam, snapshots'
cover_image: /imgs/snapper-main.png
published: true
id: 1996805
---


# Turn ESP32-CAM into a Snapshot Taker, for Selfies and Time Lapse Pictures


This project is an attempt to turn ESP32-CAM (or similar ones like LILYGO T-Camera / T-Camera Plus) microcontroller board into an Android phone-managed snapshot taker, for snapshots like selfies and time-lapse pictures ... maybe ... just for fun!

> With your Android phone, connect to the ESP32-CAM, make your post and smile.
>  Then go back to your phone to select the pictures you like best and save to your phone directly.

> With your Android phone, connect to the ESP32-CAM, set how frequently a snapshot is to be taken.
>  Then disconnect from the ESP32-CAM and let it do its job of taking time-lapse pictures.
>  Then reconnect to the ESP32-CAM to transfer the taken pictures to your phone.

The microcontroller program (sketch) is developed with Arduino framework using VS Code and PlatformIO, in the similar fashion as described by the post -- [A Way to Run Arduino Sketch With VSCode PlatformIO Directly](https://www.instructables.com/A-Way-to-Run-Arduino-Sketch-With-VSCode-PlatformIO/)

The remote UI -- control panel -- is realized with the help of the DumbDisplay Android app. You have a choice of using Bluetooth (classic) or WiFi for the connection. For a brief description of DumbDisplay, please refer to the post -- [Blink Test With Virtual Display, DumbDisplay](https://www.instructables.com/Blink-Test-With-Virtual-Display-DumbDisplay/)

Please note that the UI is driven by the sketch; i.e., the flow of the UI is programmed in the sketch. Other than installing the DumbDisplay Android app, no building of a separate mobile app is necessary.

# The UI -- Ways of taking Snapshots

There are 3 ways snapshots can be captured and saved to your phone

|  |  |
|--|--|
|With your phone connected to the ESP32-CAM -- certainly through the DumbDisplay Android app with Bluetooth or WiFi -- you click the ***💾Save*** button of the UI to save the image being shown. You can turn on / off the auto-save feature by clicking ***Auto❎*** / ***Auto☑️***. With auto-save enabled, whenever a snapshot is captured from the ESP32-CAM, it will be saved to your phone automatically.|![](/imgs/snapper-ss-00.jpg)|

|  |  |
|--|--|
|Assuming you have connected your phone with the ESP32-CAM, clicking on the image canvas will pause showing snapshots, and you will be provided with a slider to slide back in time to select the previously captured snapshots to save to your phone. You will have 20 such snapshots that you can slide back in time to. Once you see the snapshots you like, click ***💾Save*** to save it. When you are done, you can go back by clicking ***❌Cancel*** (or double click on the image canvas).|![](/imgs/snapper-ss-01.jpg)|

|  |  |
|--|--|
|Select ***Offline📴*** to enable "offline" capturing and saving of snapshots to the ESP32-CAM. With "offline" enabled, when you disconnect (i.e. offline), the ESP32-CAM will start capturing "offline" snapshots saving them to its flash memory (or SD card). Whenever you reconnect, you will be asked if you want to transfer the saved "offline" snapshots to your phone. Better yet, in SD card case, you can physically transfer the "offline" snapshot ***JPEG*** files saved the usual way -- insert the SD card to your computer, copy and delete the ***JPEG*** files on your SD card. *One point about using the SD card slot of ESP32-CAM -- since the SD card module of ESP32-CAM shares the same pin 4 used by the flashlight, whenever the SD card is accessed, the flashlight will light up bright (hence it is not a feature, but it is how it is)*|![](/imgs/snapper-ss-02.jpg)![](/imgs/snapper-ss-03.jpg)|

The snapshots will be saved in ***JPEG*** format:

* Snapshots saved to your phone will be saved to the private folder of DumbDisplay app - `/<main-storage>/Android/data/nobody.trevorlee.dumbdisplay/files/Pictures/DumbDisplay/snaps/` -- with file name like `20240904_223249_001.jpg` (`YYYYMMDD-hhmmss_seq.jpg`)
* Offline snapshots transferred to your phone will be saved to a subfolder of the above-mentioned private folder of DumbDisplay app. The name of the subfolder depends on the time you do the snapshots transfer, and is like `20240904_231329_OFF`.
* Offline snapshots are saved to your ESP32-CAM's flash memory / SD card with name like `off_001.jpg`. Note that the "offline" ***JPEG*** files will only be properly time-stamped if there was no reset / reboot of ESP32-CAM after connecting to your phone.

|  |  |  |
|--|--|--|
|By default, the storage for DumbDisplay app is a private folder, which DumbDisplay app needs to initialize. You can do this By selecting the ***Settings*** menu item of DumbDisplay app, and clicking on the ***Media Storage*** button|![](/imgs/dd-settings-menu.jpg)|![](/imgs/dd-prepare-store.jpg)|

|  |  |
|--|--|
|And you can use a folder manager app, like [***Files*** by ***Marc apps & software***](https://play.google.com/store/apps/details?id=com.marc.files&hl=en) to browse to that folder.|![](/imgs/snapper-folder.gif)|

# The UI -- Frequency of Capturing Snapshots

Actually, snapshot capturing is continuous (as fast as possible), but captured snapshot shipments to your phone is not as smooth; in fact there is a setting on how frequently a snapshot is shipped to your phone. Depending on the resolution / quality of the snapshot and the connection method / condition, it can be as frequent as 5 snapshots per second. Treat this frequency control as a feature that allows taking of time-lapse pictures at a desired frequency 😁

There are 4 quick selections of frequency / frame rate -- ***5 PS*** / ***2 PS*** / ***1 PS*** / ***30 PM*** corresponding to *5 frames per second* / *2 frames per second* / *1 frame per second* / *30 frames per minute*

And there is a custom per-hour frame rate you can set and select -- ***720 PH*** (720 is the default). Once you click on the ***720 PH*** selection, your phone's virtual keyword will pop up allowing you to enter the value (1 - 3600) you want (an empty value means previously set value). 


# The UI -- Snapshots Quality Adjustments

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

Note that the higher the snapshots quality / resolution, the more data needs be shipped from the ESP32-CAM to your phone, making the UI less responsive, especially when WiFi (not Bluetooth) since WiFi is not full-duplex.

# The UI -- ESP32 Idle Sleep

Another feature is that when "offline" snapshot capturing is enabled, after a while (60 seconds) of being idle (i.e. not connected), the ESP32-CAM will be put to sleep mode in order to save power. Of course, since "offline" capturing is enabled, the ESP32-CAM will wake up in due time to take a snapshot. However if "offline" frequency is too high, higher than 12 frames per minute, the ESP32-CAM will stay on.

In any case, ***if you want to connect to the ESP32-CAM when it is in sleep mode, you will need to reset / reboot the ESP32-CAM first***


# Developing and Building

As mentioned previously, the sketch will be developed using VS Code and PlatformIO.
Please clone the PlatformIO project [ESP32CamSnapper](https://github.com/trevorwslee/ESP32CamSnapper) GitHub repository.

The configurations for developing and building of the sketch are basically written down in the `platformio.ini` file
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

The above `main.cpp` assumes that you prefer to use Bluetooth (classic), and defines the Bluetooth device name to be ***ESP32CamSnapper***
```
#define BLUETOOTH "ESP32CamSnapper"
```
Note that, for the purposes of this sketch, Bluetooth (classic) is the suggested way of connecting with DumbDisplay app.

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

If you have SD card installed for ESP32-CAM (or LILYGO T-Camera Plus), you can use it to store "offline" snapshots, rather than the limited flash memory of the microcontroller board.

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

* By default, the ESP32-CAM will be put to sleep after 60 seconds being idle (i.e. not connected). You can change it to other value by changing the macro `IDLE_SLEEP_SECS`. If this 'idle put to sleep' behavior is not desirable, you can comment out that `#define` to disable such behavior 
  ```
  // *** number of seconds to put ESP to sleep when idle (not connected); if ESP went to sleep, will need to reset it in order to connect; comment out if not desired
  #define IDLE_SLEEP_SECS                     60
  ```  

* Most settings you change in the UI will be persisted to the EEPROM of the ESP32-CAM. These settings will be effective even you re-upload the program built. In case you want to reset those settings to their defaults, change the `HEADER` to some other value, build and re-upload
  ```
  // *** if want to reset settings / offline snap metadata saved in EEPROM, set the following to something else
  const int32_t HEADER = 20240910;
  ```  


# Sketch Highlight -- EEPROM

Firstly, in order to use the EEPROM, you will first need to reserve some number of bytes for it, like:
void setup() {
  Serial.begin(115200);
  EEPROM.begin(32);
  ...
}
Here, 32 bytes are reserved for the purpose.

Most settings are persisted to the EEPROM of ESP32-CAM like
```
  EEPROM.writeLong(0, HEADER);
  EEPROM.writeChar(4, cameraBrightness);
  EEPROM.writeBool(5, cameraVFlip);
  EEPROM.writeBool(6, cameraHMirror);
  EEPROM.writeChar(7, currentCachePMFrameRateIndex);
  EEPROM.writeChar(8, currentFrameSizeIndex);
  EEPROM.writeBool(9, enableOffline);
  EEPROM.writeShort(10, customCachePHFrameRate);
  EEPROM.writeBool(12, autoSave);
  EEPROM.writeChar(13, cameraQuality);
  EEPROM.commit();
```
> Note that:
> - **Long** is 4 bytes in size, and can be used for `int32_t`
> - **Char** is 1 byte in size, and can be used for `int8_t`
> - **Bool** is also 1 byte in size, and can be used for `bool`
> - **Short** is 2 bytes in size, and can be used for `int16_t`

And these settings are read back when ESP32-CAM starts up like
```
  int32_t header = EEPROM.readLong(0);
  if (header != HEADER) {
    dumbdisplay.logToSerial("EEPROM: not the expected header ... " + String(header) + " vs " + String(HEADER));
    return;
  }
  cameraBrightness = EEPROM.readChar(4);
  cameraVFlip = EEPROM.readBool(5);
  cameraHMirror = EEPROM.readBool(6);
  currentCachePMFrameRateIndex = EEPROM.readChar(7);
  currentFrameSizeIndex = EEPROM.readChar(8);
  enableOffline = EEPROM.readBool(9);
  customCachePHFrameRate = EEPROM.readShort(10);
  autoSave = EEPROM.readBool(12);
  cameraQuality = EEPROM.readChar(13);
```

# Sketch Highlight -- ESP32 Sleep Mode

ESP32 can be put to sleep like
```
esp_sleep_enable_timer_wakeup(sleepTimeoutMillis * 1000);  // in micro second
esp_deep_sleep_start(); 
```

When it wakes up, the sketch / program will be just like freshly started, except for variables like
```
RTC_DATA_ATTR int32_t tzMins = 0;
RTC_DATA_ATTR int64_t wakeupOfflineSnapMillis = -1;
```
Note that the variable prefixed with `RTC_DATA_ATTR` will keep its value through sleep.


# Sketch Highlight -- Camera

The ESP32-CAM Camera is initialized with `initializeCamera`
```
bool initializeCamera(framesize_t frameSize, int jpegQuality) {
  esp_camera_deinit();     // just in case, disable camera first
  delay(50);
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  ...
  config.pixel_format = PIXFORMAT_JPEG; 
  config.frame_size = frameSize;                
  config.jpeg_quality = jpegQuality;
  config.fb_count = 1;
  ..
  esp_err_t camerr = esp_camera_init(&config);  // initialize the camera
  if (camerr != ESP_OK) {
    dumbdisplay.logToSerial("ERROR: Camera init failed with error -- " + String(camerr));
  }
  resetCameraImageSettings();
  return (camerr == ESP_OK);
}
> It is important that `pixel_format` be set to `PIXFORMAT_JPEG`. 
```
Notice that at the end of `initializeCamera`, `resetCameraImageSettings` is called to set the various settings of the Camera, which is also called when you change camera settings with the UI
```
bool resetCameraImageSettings() {
  sensor_t *s = esp_camera_sensor_get();
  if (s == NULL) {
    dumbdisplay.logToSerial("Error: problem reading camera sensor settings");
    return 0;
  }
  ...
  s->set_brightness(s, cameraBrightness);  // (-2 to 2) - set brightness
  s->set_quality(s, cameraQuality);        // set JPEG quality (0 to 63)
  ...
  return 1;
}
```

Concerning capturing snapshots, here is how an "offline" snapshot is taken when ESP32-CAM wakes up from sleep
```
void setup() {
  Serial.begin(115200);
  EEPROM.begin(32);
  initRestoreSettings();
  initializeStorage();
  ...
  if (wakeupOfflineSnapMillis != -1) {  // wakeupOfflineSnapMillis will be -1 if ESP is reset
    String localTime;
    long now = getTimeNow(&localTime);
    Serial.println("*** woke up for offline snap ... millis=" + localTime + " ***");
    framesize_t frameSize = cameraFrameSizes[currentFrameSizeIndex];
    if (initializeCamera(frameSize, cameraQuality)) { 
      delay(SLEEP_WAKEUP_TAKE_SNAP_DELAY_MS);
      if (true) {
        // it appears that the snap taken will look better if the following "empty" steps are taken        
        camera_fb_t* camera_fb = esp_camera_fb_get();
        esp_camera_fb_return(camera_fb);
      }
      camera_fb_t* camera_fb = esp_camera_fb_get();
      saveOfflineSnap(camera_fb->buf, camera_fb->len);
      esp_camera_fb_return(camera_fb);
    } else {
      Serial.println("failed to initialize camera for offline snapping");
    }
    Serial.println("*** going back to sleep ... timeout=" + String(wakeupOfflineSnapMillis / 1000.0) + "s ***");
    Serial.flush();
    esp_sleep_enable_timer_wakeup(wakeupOfflineSnapMillis * 1000);  // in micro second
    esp_deep_sleep_start();
    // !!! the above call will not return !!!
  }    
  ...
}
``` 
Notice:
* the value of `wakeupOfflineSnapMillis` is used to determine whether the program is started due to normal boot up of wake up; it is set to the value of "how many milliseconds to sleep for next "offline" snap
* certain, `initializeCamera` will be called to initialize the camera
* after initializing the camera, the camera is given some time (2 seconds) to be ready
* then the camera's buffer is retrieved by calling `esp_camera_fb_get`, which contains the bytes (***JPEG*** bytes) to be saved by calling `saveOfflineSnap`
* after saving the bytes, the buffered is returned by calling `esp_camera_fb_return`
* at last, it goes back to sleep again

You will find that the tutorial [Change ESP32-CAM OV2640 Camera Settings: Brightness, Resolution, Quality, Contrast, and More](https://randomnerdtutorials.com/esp32-cam-ov2640-camera-settings/) by **Random Nerd Tutorials** gives more details on ESP32-CAM.



# Sketch Highlight -- DumbDisplay

Like all other use cases of using DumbDisplay, you first declare a global `DumbDisplay` object `dumbdisplay`
```
#if defined(BLUETOOTH)
  #include "esp32dumbdisplay.h"
  DumbDisplay dumbdisplay(new DDBluetoothSerialIO(BLUETOOTH));
#elif defined(WIFI_SSID)
  #include "wifidumbdisplay.h"
  DumbDisplay dumbdisplay(new DDWiFiServerIO(WIFI_SSID, WIFI_PASSWORD), DD_DEF_SEND_BUFFER_SIZE, 2 * DD_DEF_IDLE_TIMEOUT);
#else
  #include "dumbdisplay.h"
  DumbDisplay dumbdisplay(new DDInputOutput());
#endif
```
> In WiFi case, normally, you will use the defaults for `sendBufferSize` (2nd parameter) and `idleTimeout` (3rd parameter). However for this program, since a large amount of data might be shipped to DumbDisplay app, the `idleTimeout` better be longer than the default. 

Then, several global helper objects / pointers are declared
```
DDMasterResetPassiveConnectionHelper pdd(dumbdisplay, true);
GraphicalDDLayer* imageLayer;
JoystickDDLayer* frameSliderLayer;
SelectionDDLayer* frameSizeSelectionLayer;
...
BasicDDTunnel* generalTunnel;
```
> The construction of `DDMasterResetPassiveConnectionHelper` accepts two parameters
> 1) The DumbDisplay object
> 2) Whether to call `DumbDisplay::recordLayerCommands()` / `DumbDisplay::playbackLayerCommands()` before and after calling "initialize" lambda expression. The effect of calling `DumbDisplay::recordLayerCommands()` / `DumbDisplay::playbackLayerCommands()` is so the UI construction of the various layers is more smooth. 

The life-cycle of the above DumbDisplay layers and "tunnels" are managed by the global `pdd` object, which will monitor connection and disconnection of
DumbDisplay app, calling appropriate C++ lambda expressions in the appropriate time. It is cooperatively given "time slices" in the `loop()` block like
```
void loop() {
  pdd.loop([]() {
    initializeDD();
  }, []() {
    if (updateDD(!pdd.firstUpdated())) {
       saveSettings();
    }
  }, []() {
    deinitializeDD();
  });
  if (pdd.isIdle()) {
    handleIdle(pdd.justBecameIdle());
  }
}
```
* `pdd.loop()` accepts 3 C++ lambda expressions -- `[]() { ... }`
  - the 1st lambda expression is called when DumbDisplay app is connected, and DumbDisplay components need be initialized. Here `initializeDD()` is called for the purpose.
  - the 2nd lambda expression is called during DumbDisplay app is connected to update the DumbDisplay components. Here, `updatedDD()` for the purpose. Note that `pdd.firstUpdated()` tells if this 2nd lambda expression was previously called after DumbDisplay component initialization (hence not of that means first call)
  - the 3rd optional lambda expression is called when DumbDisplay app is disconnected. Here, `deinitializeDD()` is called for the purpose.
* After `pdd.loop()` is the code that will be run whether DumbDisplay app is connected or not.
* `pdd.isIdle()` tells if DumbDisplay is not connected (i.e. idle). In idle case, the function `handleIdle()` is called. Note that `pdd.justBecameIdle()` tells if DumbDisplay just disconnected making it idle.   

The first time when the ESP32-CAM is connected, ESP32-CAM's clock is synchronized with that of your phone via `generalTunnel` -- a "general tunnel" 
```
void initializeDD() {
  ...
  generalTunnel = dumbdisplay.createGeneralServiceTunnel();
  generalTunnel->reconnectTo(DD_CONNECT_FOR_GET_DATE_TIME);  // ask DumbDisplay app for current time, in order to set ESP's clock
  ...
}
...
bool updateDD(bool isFirstUpdate) {
  ...
  if (generalTunnel->pending()) {
    String response;
    if (generalTunnel->readLine(response)) {
      const String& endPoint = generalTunnel->getEndpoint();
      if (endPoint == DD_CONNECT_FOR_GET_DATE_TIME) {
        // got current time "feedback" from DumbDisplay app (initially requested)
        dumbdisplay.log("- got sync time " + response);
        DDDateTime dateTime;
        DDParseGetDataTimeResponse(response, dateTime, &tzMins);
        dumbdisplay.log("  . tz_mins=" + String(tzMins));
        Esp32SetDateTime(dateTime);
        ...
  ...
}
```
> Note that syncing and setting of the ESP32-CAM's clock is only done once when the ESP32-CAM connects with your phone. Also note that the ESP32-CAM's clock will keep running during sleep.

Other than the main scene / state, the UI has other scenes, like for selecting snapshots to save, and for transferring "offline" snapshots. When switching between the different scenes, the UI layers will be re-auto-pined by calling `pinLayers`
```
void pinLayers(State forState) {
  if (forState == STREAMING) {
    dumbdisplay.configAutoPin(DDAutoPinConfig('V')
      .beginGroup('H')
        .addLayer(frameSizeSelectionLayer)
        ...
        .addLayer(saveButtonLayer)
      .endGroup()  
      .build(), true);
  } else if (forState == CONFIRM_SAVE_SNAP) {
    dumbdisplay.configAutoPin(DDAutoPinConfig('V')
      .addLayer(imageLayer)
      ...
      .endGroup()
      .build(), true);
  } else if (forState == TRANSFER_OFFLINE_SNAPS) {
    dumbdisplay.configAutoPin(DDAutoPinConfig('V')
      .addLayer(imageLayer)
      .addLayer(progressLayer)
      .addLayer(cancelButtonLayer)
      .build(), true);
  }
}
```
> The last parameter of `configAutoPin` is `autoShowHideLayers`.  Which if `true`, auto-pinning will automatically hide all other not-involved layers. This relieves the need to explicitly hide all layers not involved in different scenes of the UI.

Selection and prompting for custom frame rate might be worth highlighting here. It is accomplished with the `customFrameRateSelectionLayer` layer
```
    fb = customFrameRateSelectionLayer->getFeedback();
    if (fb != NULL) {
      if (fb->type == CUSTOM) {
        int phFrameRate = fb->text.isEmpty() ? customCachePHFrameRate : fb->text.toInt();
        if (phFrameRate >= 1 && phFrameRate <= 3600) {
          customCachePHFrameRate = phFrameRate;
          ...
        } else {
          dumbdisplay.log("invalid custom frame rate!", true);
        }
      } else {
        customFrameRateSelectionLayer->explicitFeedback(0, 0, "'per-hour' frame rate; e.g. " + String(customCachePHFrameRate) , CUSTOM, "numkeys");
      }
    }
```
* normally the type of "feedback" (`fb->type`) from the UI will not be `CUSTOM`
* in such a non-`CUSTOM` case (i.e. "feedback" is because of clicking), call `customFrameRateSelectionLayer->explicitFeedback` like above
  - `explicitFeedback` will explicitly fake a "feedback" to the layer
  - but this time, the type of "feedback" is `CUSTOM` -- the 4th parameter
  - the first 3 parameters are for `x`, `y`, and `text` of the "feedback"
  - the last parameters -- option to `explicitFeedback` `numkeys` -- means that during the "feedback" routing, you will be prompted for numeric value with your phone's virtual keyboard, which will then replace the text of the "feedback"
  - note that the initial `text` passed in to `explicitFeedback` is used as the "hint" on the virtual keyboard
* in `CUSTOM` case, `fb->text` will be the value you entered 

Another opportunity of prompting is confirmation for transferring "offline" snapshots to your phone. It is accomplished via `generalTunnel` like
```
bool updateDD(bool isFirstUpdate) {
  ...
  if (generalTunnel->pending()) {
    String response;
    if (generalTunnel->readLine(response)) {
      const String& endPoint = generalTunnel->getEndpoint();
      if (endPoint == DD_CONNECT_FOR_GET_DATE_TIME) {
        // got current time "feedback" from DumbDisplay app (initially requested)
        ...
        if (state == STREAMING && offlineSnapCount > 0) {
          generalTunnel->reconnectTo("confirm?title=Transfer Snaps&message=There are " + String(offlineSnapCount) + " offline snaps. Transfer them%3F&ok=Yes&cancel=No");
        }
#endif
      } else {
        if (state == STREAMING && response == "Yes") {
          setStateToTransferOfflineSnaps();  // assume the response if from the "confirm" dialog [for transferring offline snaps]
        }
        ...
      }
    }
  ...
}
```
* Right after gotten the "sync time" from DumbDisplay app, and when need to prompt for "yes/no" confirmation for transferring "offline" snaps, `generalTunnel->reconnectTo` is called to handle the "confirm" request.
* The "Yes" or "No" response will come back through `generalTunnel` again (the same way as `DD_CONNECT_FOR_GET_DATE_TIME`)

After the "offline" transfer, an "alert" is popped up indicating the transfer is done. This is done by calling `dumbdispaly.alert`
```
bool updateDD(bool isFirstUpdate) {
  ...
  if (state == TRANSFER_OFFLINE_SNAPS) {
    if (offlineSnapCount == 0 || clickedCancelButton) {
      ...
      dumbdisplay.alert("Transferred " + String(startTransferOfflineSnapCount - offlineSnapCount) + " offline snaps!");
      ...
  }
  ...
}
```


# Build and Upload

Build, upload the sketch and try it out! 

If it interests you, you may use a Serial monitor (set to baud-rate 115200) to observe when things are happening

```
Initialize offline snap storage ...
... offlineSnapCount=3 ...
... existing STORAGE ...
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
... existing STORAGE ...
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

Hope that you will have fun with it! Enjoy!

> Peace be with you!
> May God bless you!
> Jesus loves you!
> Amazing Grace!

