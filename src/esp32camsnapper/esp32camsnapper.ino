#include "Arduino.h"

// *** if want to use classical Bluetooth, define BLUETOOTH as Bluetooth device name below
// #define BLUETOOTH "ESP32CamSnapper"
// *** otherwise, if want to use wifi, define the following WIFI_SSID and WIFI_PASSWORD
// #define WIFI_SSID           "your-wifi-ssid"
// #define WIFI_PASSWORD       "your-wifi-password"
// *** if not using wifi or bluetooth, will assume much slower usb (otg) connectivity 


// *** maximum number of images to keep cached (on app side) when streaming snaps to DD app; this number affects how much you can go back to select which image to save
#define STREAM_KEEP_IMAGE_COUNT             20


// *** support offline snap taking; comment out if not desired
#define SUPPORT_OFFLINE
// *** uncomment the following if offline snaps are to be stored to SD (ESP32CAM / TCameraPlus), rather than LittleFS
// #define OFFLINE_USE_SD 
// *** minimum free storage (SD / LittleFS storage) needed for offline snap taking
#define MIN_STORE_FREE_BYTE_COUNT           128 * 1024
// *** if desired to disable offline snap taking upon reset of ESP, uncomment the following
// #define DISABLE_OFFLINE_UPON_RESET


// *** number of seconds to put ESP to sleep when idle (not connected); if ESP went to sleep, will need to reset it in order to connect; comment out if not desired
#define IDLE_SLEEP_SECS                     60
// *** maximum PER-HOUR frame rate for ESP to go to sleep during offline snapping (i.e. honor IDLE_SLEEP_SECS)
#define MAX_SLEEP_PH_FRAME_RATE             720
// *** delay in ms for the camera to be ready after ESP woke up
#define SLEEP_WAKEUP_TAKE_SNAP_DELAY_MS     2000
// *** ESP sleep wake up timer overhead in ms
#define SLEEP_TIMER_OVERHEAD_MS             800


// *** if want to reset settings / offline snap metadata saved in EEPROM, set the following to something else
const int32_t HEADER = 20240910;


#if defined(BLUETOOTH)
  #include "esp32dumbdisplay.h"
  DumbDisplay dumbdisplay(new DDBluetoothSerialIO(BLUETOOTH));
  #define INIT_FRAME_SIZE_INDEX 1
  #define INIT_FRAME_RATE_INDEX 1
#elif defined(WIFI_SSID)
  #include "wifidumbdisplay.h"
  DumbDisplay dumbdisplay(new DDWiFiServerIO(WIFI_SSID, WIFI_PASSWORD), DD_DEF_SEND_BUFFER_SIZE, 2 * DD_DEF_IDLE_TIMEOUT);
  #define INIT_FRAME_SIZE_INDEX 1
  #define INIT_FRAME_RATE_INDEX 2
#else
  // for direct USB connection to phone
  // . via OTG -- see https://www.instructables.com/Blink-Test-With-Virtual-Display-DumbDisplay/
  #include "dumbdisplay.h"
  DumbDisplay dumbdisplay(new DDInputOutput());
  #define INIT_FRAME_SIZE_INDEX 0
  #define INIT_FRAME_RATE_INDEX 2
#endif


#define INIT_CAMERA_QUALITY 15


#include "esp_camera.h"
#include <EEPROM.h> 

bool initializeCamera(framesize_t frameSize, int jpegQuality);
void deinitializeCamera();
bool resetCameraImageSettings();

void initRestoreSettings();
void saveSettings();


#if defined(SUPPORT_OFFLINE)
  #include <FS.h>
  #if defined(OFFLINE_USE_SD)
    #if defined(FOR_TCAMERAPLUS)
      #define STORAGE SD
      #include "SD.h"
      SPIClass spi = SPIClass();
    #elif defined(FOR_XIAO_S3SENSE)
      #define STORAGE SD
      #include "SD.h"
      SPIClass spi = SPIClass();
    #else
      #define STORAGE SD_MMC
      #include "SD_MMC.h"
    #endif
  #else
    #include <LittleFS.h>
  #endif
  bool initializeStorage();
  bool checkFreeStorage();
  bool verifyOfflineSnaps();
  void saveOfflineSnap(uint8_t *bytes, int byteCount);
  bool transferOneOfflineSnap();  
  void writeOfflineSnapPosition();
#endif


#if defined(IDLE_SLEEP_SECS)   
  // the following variables should have values kept across sleep
  RTC_DATA_ATTR int32_t tzMins = 0;
  RTC_DATA_ATTR int64_t wakeupOfflineSnapMillis = -1;
#else
  int32_t tzMins = 0;
#endif

long getTimeNow(String* timeNow) {
  long now = millis();
  if (timeNow == NULL) {
    return now;
  }
  *timeNow = String(now);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 0)) {
    return now;  // ESP's clock not set 
  }
  char buf[64];
  strftime(buf, 64, "%A, %B %d %Y %H:%M:%S", &timeinfo);
  String localTime = String(buf);
  if (tzMins != 0) {
    int tzM = abs(tzMins);
    localTime += (tzMins > 0 ? "+" : "-") + String(100 + (tzM / 60)).substring(1) + String(100 + (tzM % 60)).substring(1);
  }
  *timeNow = *timeNow + String("(") + localTime + String(")");
  return now;
}


extern DDMasterResetPassiveConnectionHelper pdd;
void initializeDD();
bool updateDD(bool isFirstUpdate);
void deinitializeDD();
void handleIdle(bool justBecameIdle);

// 7 frame size selections
const int cameraFrameSizeCount = 7;
framesize_t cameraFrameSizes[cameraFrameSizeCount] = {FRAMESIZE_QVGA, FRAMESIZE_VGA, FRAMESIZE_SVGA, FRAMESIZE_XGA, FRAMESIZE_HD, FRAMESIZE_SXGA, FRAMESIZE_UXGA};

// 4 PER-MINUTE frame rate selections
const int cachePMFrameRateCount = 4; 
int cachePMFrameRates[cachePMFrameRateCount] = {60 * 5, 60 * 2, 60, 30};

// phone storage directory name for saved images
const String snapDir = "snaps";
// offline snap file name prefix (in SD or LittleFS)
const String offlineSnapNamePrefix = "off_";

// the following settings are stored in EEPROM
int8_t cameraBrightness = 0;
uint8_t cameraQuality = INIT_CAMERA_QUALITY;
bool cameraVFlip = false;
bool cameraHMirror = false;
int8_t currentFrameSizeIndex = INIT_FRAME_SIZE_INDEX;
int8_t currentCachePMFrameRateIndex = INIT_FRAME_RATE_INDEX;
int16_t customCachePHFrameRate = MAX_SLEEP_PH_FRAME_RATE;
bool enableOffline = false;
bool autoSave = false;



bool offlineStorageInitialized = false;
int startOfflineSnapIdx = 0;
int offlineSnapCount = 0;
int startTransferOfflineSnapCount = 0;
String transferImageFileNamePrefixForOfflineSnaps;
long idleStartMillis = 0;


void setup() {
  delay(10000);
  Serial.begin(115200);
  EEPROM.begin(32);
  initRestoreSettings();
#if defined(SUPPORT_OFFLINE)
  initializeStorage();
#endif

#if defined(IDLE_SLEEP_SECS)   
  if (wakeupOfflineSnapMillis != -1) {  // wakeupOfflineSnapMillis will be -1 if ESP is reset
    String localTime;
    long now = getTimeNow(&localTime);
    Serial.println("*** woke up for offline snap ... millis=" + localTime + " ***");

  #if defined(SUPPORT_OFFLINE)
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
  #endif

    Serial.println("*** going back to sleep ... timeout=" + String(wakeupOfflineSnapMillis / 1000.0) + "s ***");
    Serial.flush();
    esp_sleep_enable_timer_wakeup(wakeupOfflineSnapMillis * 1000);  // in micro second
    esp_deep_sleep_start();
    // !!! the above call will not return !!!
  }    
#endif
#if defined(SUPPORT_OFFLINE) && defined(DISABLE_OFFLINE_UPON_RESET)
  if (enableOffline) {
    dumbdisplay.logToSerial("*** disable offline snap upon reset ***");
    enableOffline = false;
    saveSettings();
  }
#endif
}

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



// **********
// ***** main *****
// **********


#define IMAGE_LAYER_WIDTH  640
#define IMAGE_LAYER_HEIGHT 480


DDMasterResetPassiveConnectionHelper pdd(dumbdisplay, true);

GraphicalDDLayer* imageLayer;
JoystickDDLayer* frameSliderLayer;
SelectionDDLayer* frameSizeSelectionLayer;
SelectionDDLayer* cameraOptionSelectionLayer;
LcdDDLayer* qualityLabelLayer;
JoystickDDLayer* qualitySliderLayer;
LcdDDLayer* brightnessLabelLayer;
JoystickDDLayer* brightnessSliderLayer;
LcdDDLayer* savedCountLabelLayer;
SevenSegmentRowDDLayer* savedCountLayer;
LcdDDLayer* saveButtonLayer;
LcdDDLayer* cancelButtonLayer;
SelectionDDLayer* frameRateSelectionLayer;
SelectionDDLayer* customFrameRateSelectionLayer;
SelectionDDLayer* autoSaveSelectionLayer;
SelectionDDLayer* enableOfflineSelectionLayer;
LedGridDDLayer* progressLayer;
BasicDDTunnel* generalTunnel;

bool cameraReady = false;
//bool cachedImageOnce = false;
long lastCachedImageMs = 0;

long fps_lastMs = -1;
long fps_lastLastMs = -1;

long cache_lastMs = -1;
long cache_lastLastMs = -1;

int streamImageNameIndex = 0;
int streamImageSlotLeft = STREAM_KEEP_IMAGE_COUNT;
String currentStreamImageFileName;

String currentSnapFileName;

int savedSnapCount = 0;

enum State {
  STREAMING,
  CONFIRM_SAVE_SNAP,
  TRANSFER_OFFLINE_SNAPS
};

State state;
unsigned long retrieveStartMillis;

void cameraFrameSizeToText(framesize_t frameSize, String& text) {
    if (frameSize == FRAMESIZE_QVGA) {
      text = "QVGA";
    } else if (frameSize == FRAMESIZE_VGA) {
      text = "VGA";
    } else if (frameSize == FRAMESIZE_SVGA) {
      text = "SVGA";
    } else if (frameSize == FRAMESIZE_XGA) {
      text = "XGA";
    } else if (frameSize == FRAMESIZE_HD) {
      text = "HD";
    } else if (frameSize == FRAMESIZE_SXGA) {
      text = "SXGA";
    } else if (frameSize == FRAMESIZE_UXGA) {
      text = "UXGA";
    } else {
      text = "???";
    }
}

void cachePMFrameRateToText(int framePerMinute, String& text) {
  if (framePerMinute >= 60) {
    text = String(framePerMinute / 60) + " PS";
  } else {
    text = String(framePerMinute) + " PM";
  }
}


void setupCurrentStreamImageName() {
  currentStreamImageFileName = "_" + String(streamImageNameIndex++) + ".jpg";
  if (streamImageNameIndex >= STREAM_KEEP_IMAGE_COUNT) {
    streamImageNameIndex = 0;
  }
  if (streamImageSlotLeft > 0) {
    streamImageSlotLeft--;
  }
}
bool setupCurrentStreamImageNameOfOffset(int offset) {
  if (streamImageSlotLeft > 0) {
    int maxOffset = (STREAM_KEEP_IMAGE_COUNT - streamImageSlotLeft) - 1;
    if (offset > maxOffset) {
      offset = maxOffset;
    }
  }
  int idx = (STREAM_KEEP_IMAGE_COUNT + (streamImageNameIndex - 1) - offset) % STREAM_KEEP_IMAGE_COUNT;
  String newCurrentStreamImageFileName = "_" + String(idx) + ".jpg";
  if (currentStreamImageFileName != newCurrentStreamImageFileName) {
    currentStreamImageFileName = newCurrentStreamImageFileName;
    return true;
  } else {
    return false;
  }
}
void formatDataTimeForFilename(String& filenameDateTime, const DDDateTime* pDateTime) {
  if (pDateTime == NULL) {
    filenameDateTime = "00000000_000000";
  } else {
    filenameDateTime = String(pDateTime->year) + "-" + String(100 + pDateTime->month).substring(1) + "-" + String(100 + pDateTime->day).substring(1) + "_" +
                       String(pDateTime->hour + 100).substring(1) + String(pDateTime->minute + 100).substring(1) + String(pDateTime->second + 100).substring(1);
  }
}
void setupCurrentSnapFileName() {
  DDDateTime dateTime;
  const DDDateTime* pDateTime = NULL;
  if (Esp32GetDateTime(dateTime)) {
    pDateTime = &dateTime;
  }
  String startDateTime;
  formatDataTimeForFilename(startDateTime, pDateTime);
  String seq = savedSnapCount < 1000 ? String(1000 + savedSnapCount).substring(1) : String(savedSnapCount);
  currentSnapFileName = snapDir + "/" + startDateTime + "_" + seq + ".jpg";
  if (++savedSnapCount <= 999) {
    savedCountLayer->showNumber(savedSnapCount, "0");
  } else {
    savedCountLayer->showFormatted("---");
  }
}

#if defined(SUPPORT_OFFLINE)
void setTransferImageFileNamePrefixForOfflineSnap() {
  DDDateTime dateTime;
  const DDDateTime* pDateTime = NULL;
  if (Esp32GetDateTime(dateTime)) {
    pDateTime = &dateTime;
  }
  String startDateTime;
  formatDataTimeForFilename(startDateTime, pDateTime);
  transferImageFileNamePrefixForOfflineSnaps = snapDir + "/" + startDateTime + "_OFF";
}
void updateTransferProgress(bool initUpdate) {
  if (initUpdate) {
    startTransferOfflineSnapCount = offlineSnapCount;
    setTransferImageFileNamePrefixForOfflineSnap();    
  }
  int percent = (startTransferOfflineSnapCount > 0) ? (100 * (startTransferOfflineSnapCount - offlineSnapCount) / startTransferOfflineSnapCount) : 100;
  progressLayer->horizontalBar(percent);
}
#endif

void pinLayers(State forState) {
  if (forState == STREAMING) {
    dumbdisplay.configAutoPin(DDAutoPinConfig('V')
      .beginGroup('H')
        .addLayer(frameSizeSelectionLayer)
        .beginGroup('V')
          .addLayer(cameraOptionSelectionLayer)
          .beginGroup('H')
            .addLayer(brightnessLabelLayer)
            .addLayer(brightnessSliderLayer)
          .endGroup()
          .beginGroup('H')
            .addLayer(qualityLabelLayer)
            .addLayer(qualitySliderLayer)
          .endGroup()
          .addLayer(imageLayer)
        .endGroup()  
      .endGroup()
      .beginGroup('H')
        .addLayer(frameRateSelectionLayer)
        .beginGroup('V')
          .addLayer(enableOfflineSelectionLayer)
          .addLayer(customFrameRateSelectionLayer)
        .endGroup()
      .endGroup()
      .beginGroup('H')
        .beginGroup('V')
          .beginGroup('H')
            .addLayer(savedCountLabelLayer)
            .addLayer(savedCountLayer)
          .endGroup()
          .addLayer(autoSaveSelectionLayer)
        .endGroup()
        .addLayer(saveButtonLayer)
      .endGroup()  
      .build(), true);
  } else if (forState == CONFIRM_SAVE_SNAP) {
    dumbdisplay.configAutoPin(DDAutoPinConfig('V')
      .addLayer(imageLayer)
      .addLayer(frameSliderLayer)
      .beginGroup('V')
        .beginGroup('H')
          .addLayer(savedCountLabelLayer)
          .addLayer(savedCountLayer)
        .endGroup()
        .beginGroup('H')
          .addLayer(cancelButtonLayer)
          .addLayer(saveButtonLayer)
        .endGroup()
      .endGroup()
      .build(), true);
  } else if (forState == TRANSFER_OFFLINE_SNAPS) {
    dumbdisplay.configAutoPin(DDAutoPinConfig('V')
      .addLayer(imageLayer)
      .addLayer(progressLayer)
      .addLayer(cancelButtonLayer)
      .build(), true);
  } else {
    dumbdisplay.log("!!! unknown state for pin layers !!!", true);
  }
}

void initializeDD() {
  if (cameraReady) {
    deinitializeCamera();
    cameraReady = false;
  }

  imageLayer = dumbdisplay.createGraphicalLayer(IMAGE_LAYER_WIDTH, IMAGE_LAYER_HEIGHT);
  imageLayer->border(10, DD_COLOR_blue, "round");  
  imageLayer->padding(5);
  imageLayer->noBackgroundColor();
  imageLayer->setTextSize(18);
  imageLayer->enableFeedback();

  frameSliderLayer = dumbdisplay.createJoystickLayer(1023, "rl", 0.5);
  frameSliderLayer->valueRange(1, STREAM_KEEP_IMAGE_COUNT);
  frameSliderLayer->padding(1);
  frameSliderLayer->border(1, DD_COLOR_navy);
  frameSliderLayer->snappy();
  frameSliderLayer->showValue(true, "a80%white");

  saveButtonLayer = dumbdisplay.createLcdLayer(6, 2);
  saveButtonLayer->backgroundColor(DD_COLOR_ivory);
  saveButtonLayer->border(1, DD_COLOR_darkred, "raised");
  saveButtonLayer->padding(1);
  saveButtonLayer->writeCenteredLine("💾", 0);
  saveButtonLayer->writeCenteredLine("Save", 1);
  saveButtonLayer->enableFeedback();

  cancelButtonLayer = dumbdisplay.createLcdLayer(6, 2);
  cancelButtonLayer->backgroundColor(DD_COLOR_ivory);
  cancelButtonLayer->border(1, DD_COLOR_darkgreen, "raised");
  cancelButtonLayer->padding(1);
  cancelButtonLayer->writeCenteredLine("❌", 0);
  cancelButtonLayer->writeCenteredLine("Cancel", 1);
  cancelButtonLayer->enableFeedback();


  frameSizeSelectionLayer = dumbdisplay.createSelectionLayer(4, 1, 1, cameraFrameSizeCount);
  frameSizeSelectionLayer->backgroundColor(DD_COLOR_beige);
  frameSizeSelectionLayer->highlightBorder(true, DD_COLOR_blue);
  for (int i = 0; i < cameraFrameSizeCount; i++) {
    String fs;
    cameraFrameSizeToText(cameraFrameSizes[i], fs);
    frameSizeSelectionLayer->text(fs, 0, i);
  }

  savedCountLabelLayer = dumbdisplay.createLcdLayer(8, 1);
  savedCountLabelLayer->backgroundColor(DD_COLOR_azure);
  savedCountLabelLayer->border(1, DD_COLOR_green, "hair");
  savedCountLabelLayer->padding(1);
  savedCountLabelLayer->writeLine("Saved🗂️");

  savedCountLayer = dumbdisplay.create7SegmentRowLayer(3);
  savedCountLayer->backgroundColor(DD_COLOR_azure);
  savedCountLayer->border(10, DD_COLOR_red, "flat");
  savedCountLayer->padding(10);
  savedCountLayer->showNumber(savedSnapCount, "0");

  frameRateSelectionLayer = dumbdisplay.createSelectionLayer(5, 1, cachePMFrameRateCount / 2, 2);
  frameRateSelectionLayer->backgroundColor(DD_COLOR_beige);
  for (int i = 0; i < cachePMFrameRateCount; i++) {
    String fr;
    cachePMFrameRateToText(cachePMFrameRates[i], fr);
    frameRateSelectionLayer->text(fr, 0, i);
  }

  customFrameRateSelectionLayer = dumbdisplay.createSelectionLayer(9, 1);
  customFrameRateSelectionLayer->backgroundColor(DD_COLOR_lightgreen);

  enableOfflineSelectionLayer = dumbdisplay.createSelectionLayer(9, 1);
  enableOfflineSelectionLayer->backgroundColor(DD_COLOR_lightblue);
  enableOfflineSelectionLayer->highlightBorder(true, DD_COLOR_red);
  enableOfflineSelectionLayer->textCentered("Offline📴");
  enableOfflineSelectionLayer->enableFeedback();  // reenable to not to auto flash


  cameraOptionSelectionLayer = dumbdisplay.createSelectionLayer(8, 1, 2);
  cameraOptionSelectionLayer->backgroundColor(DD_COLOR_azure);
  cameraOptionSelectionLayer->highlightBorder(false, DD_COLOR_lightgray);
  cameraOptionSelectionLayer->textCentered("V Flip", 0, 0);
  cameraOptionSelectionLayer->textCentered("H Mirror", 0, 1);

  brightnessLabelLayer = dumbdisplay.createLcdLayer(2, 1);
  brightnessLabelLayer->backgroundColor(DD_COLOR_azure);
  brightnessLabelLayer->border(1, DD_COLOR_green, "hair");
  brightnessLabelLayer->padding(1);
  brightnessLabelLayer->writeLine("☀️");

  brightnessSliderLayer = dumbdisplay.createJoystickLayer(1023, "lr", 0.5);
  brightnessSliderLayer->valueRange(-2, 2);
  brightnessSliderLayer->padding(1);
  brightnessSliderLayer->border(1, DD_COLOR_khaki);
  brightnessSliderLayer->snappy();
  brightnessSliderLayer->showValue(true, "a80%ivory");

  qualityLabelLayer = dumbdisplay.createLcdLayer(2, 1);
  qualityLabelLayer->backgroundColor(DD_COLOR_azure);
  qualityLabelLayer->border(1, DD_COLOR_green, "hair");
  qualityLabelLayer->padding(1);
  qualityLabelLayer->writeLine("🖼️");

  qualitySliderLayer = dumbdisplay.createJoystickLayer(1023, "lr", 0.5);
  qualitySliderLayer->valueRange(5, 60, 5);
  qualitySliderLayer->padding(1);
  qualitySliderLayer->border(1, DD_COLOR_khaki);
  qualitySliderLayer->snappy();
  qualitySliderLayer->showValue(true, "a80%ivory");

  autoSaveSelectionLayer = dumbdisplay.createSelectionLayer(8, 1);
  autoSaveSelectionLayer->backgroundColor(DD_COLOR_ivory);
  autoSaveSelectionLayer->highlightBorder(false, DD_COLOR_lightgray);
  autoSaveSelectionLayer->textCentered("Auto☑️");
  autoSaveSelectionLayer->unselectedTextCentered("Auto❎");
  autoSaveSelectionLayer->enableFeedback();  // reenable to not to auto flash

  progressLayer = dumbdisplay.createLedGridLayer(100, 1, 1, 4);
  progressLayer->border(0.5, DD_COLOR_navy);
  progressLayer->onColor(DD_COLOR_green);
  
  generalTunnel = dumbdisplay.createGeneralServiceTunnel();
  generalTunnel->reconnectTo(DD_CONNECT_FOR_GET_DATE_TIME);  // ask DumbDisplay app for current time, in order to set ESP's clock

  dumbdisplay.writeComment("initialized");

#if defined(SUPPORT_OFFLINE)
  if (offlineStorageInitialized) {
  #if defined(OFFLINE_USE_SD)
    uint64_t total = STORAGE.totalBytes();
    uint64_t used = STORAGE.usedBytes();
    uint64_t free = total - used;
  #else
    size_t total = LittleFS.totalBytes();
    size_t used = LittleFS.usedBytes();
    size_t free = total - used;
  #endif
    int freePercent = 100.0 * free / total;
    dumbdisplay.writeComment("- STORAGE");
    dumbdisplay.writeComment("  $ total: " + String(total / 1024) + "KB");
    dumbdisplay.writeComment("  $ used: " + String(used / 1024) + "KB");
    dumbdisplay.writeComment("  $ free: " + String(free / 1024) + "KB (" + String(freePercent) + "%)");
  } else {
    dumbdisplay.writeComment("STORAGE not ready!");
    enableOfflineSelectionLayer->disabled(true);
  }
#else
    enableOfflineSelectionLayer->disabled(true);
#endif
}


void setStateToStreaming() {
  pinLayers(STREAMING);
  frameSliderLayer->moveToPos(0, 0);
  state = STREAMING;
}
void setStateToConfirmSave() {
  pinLayers(CONFIRM_SAVE_SNAP);
  state = CONFIRM_SAVE_SNAP;
}
#if defined(SUPPORT_OFFLINE)
void setStateToTransferOfflineSnaps() {
  pinLayers(TRANSFER_OFFLINE_SNAPS);
  updateTransferProgress(true);
  state = TRANSFER_OFFLINE_SNAPS;
}
#endif

void handleSaveImage(bool then_to_stream) {
  setupCurrentSnapFileName();
  imageLayer->saveCachedImageFile(currentStreamImageFileName, currentSnapFileName);
  imageLayer->drawImageFileFit(currentSnapFileName);
#if defined(MORE_LOGGING)
    dumbdisplay.writeComment("* saved: " + currentSnapFileName);
#endif
}

void updateCameraOptionSelectionLayer() {
  cameraOptionSelectionLayer->selected(cameraVFlip, 0);
  cameraOptionSelectionLayer->selected(cameraHMirror, 1);
}

void updateBrightnessLabel() {
  int alpha = 150;
  if (cameraBrightness >= -1) {
    alpha -= 30;
  }
  if (cameraBrightness >= 0) {
    alpha -= 30;
  }
  if (cameraBrightness >= 1) {
    alpha -= 30;
  }
  if (cameraBrightness >= 2) {
    alpha -= 30;
  }
  brightnessLabelLayer->blend(DD_COLOR_gray, alpha);
}
void updateQualityLabel() {
  int alpha = 4 * cameraQuality;
  qualityLabelLayer->blend(DD_COLOR_darkgray, alpha, "NOISE");
}

void updateCustomFrameRateSelectionLayer() {
  String fr = String(customCachePHFrameRate) + " PH";
  customFrameRateSelectionLayer->textCentered(fr);
}
void updateAutoSaveSelectionLayer() {
    if (autoSave) {
      autoSaveSelectionLayer->select();
    } else {
      autoSaveSelectionLayer->deselect();
    }
}
#if defined(SUPPORT_OFFLINE)
void updateEnableOfflineSelectionLayer() {
    if (offlineStorageInitialized && enableOffline) {
      enableOfflineSelectionLayer->select();
      enableOfflineSelectionLayer->blend(DD_COLOR_red, 64);
    } else {
      enableOfflineSelectionLayer->deselect();
      enableOfflineSelectionLayer->noblend();
    }
}
#endif

long getCurrentPHFrameRate() {
    if (currentCachePMFrameRateIndex != -1) {
      return 60 * cachePMFrameRates[currentCachePMFrameRateIndex];
    } else {
      return customCachePHFrameRate;
    }
}

bool updateDD(bool isFirstUpdate) {
  bool eepromChanged = false;

  if (isFirstUpdate) {
    if (true) {
      String localTime;
      long now = getTimeNow(&localTime);
      dumbdisplay.logToSerial("*** just connected ... millis=" + localTime + " ***");
    }
    frameSizeSelectionLayer->select(currentFrameSizeIndex);
    if (currentCachePMFrameRateIndex != -1) {
      frameRateSelectionLayer->select(currentCachePMFrameRateIndex);
    } else {
      customFrameRateSelectionLayer->select();
    }
    updateCustomFrameRateSelectionLayer();
    updateAutoSaveSelectionLayer();
#if defined(SUPPORT_OFFLINE)
    updateEnableOfflineSelectionLayer();
#endif
    setStateToStreaming();
  }

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
#if defined(SUPPORT_OFFLINE)
        if (state == STREAMING && offlineSnapCount > 0) {
          generalTunnel->reconnectTo("confirm?title=Transfer Snaps&message=There are " + String(offlineSnapCount) + " offline snaps. Transfer them%3F&ok=Yes&cancel=No");
        }
#endif
      } else {
#if defined(SUPPORT_OFFLINE)        
        if (state == STREAMING && response == "Yes") {
          setStateToTransferOfflineSnaps();  // assume the response if from the "confirm" dialog [for transferring offline snaps]
        }
 #else
        dumbdisplay.log("unexpected service endpoint: [" + endPoint + "] with response [" + response + "]", true);
 #endif         
      }
    }
    return eepromChanged;
  }

  if (!cameraReady) {
    framesize_t frameSize = cameraFrameSizes[currentFrameSizeIndex];
    dumbdisplay.logToSerial("Initializing camera ...");
    cameraReady = initializeCamera(frameSize, cameraQuality); 
    //cachedImageOnce = false;
    if (cameraReady) {
      dumbdisplay.logToSerial("... initialized camera!");
      String fs;
      cameraFrameSizeToText(frameSize, fs);
      dumbdisplay.writeComment("initialized camera to " + fs);
      updateCameraOptionSelectionLayer();
      brightnessSliderLayer->moveToPos(cameraBrightness, 0);
      updateBrightnessLabel();
      //aeLevelSliderLayer->moveToPos(cameraAELevel, 0);
      qualitySliderLayer->moveToPos(cameraQuality, 0);
      updateQualityLabel();
      imageLayer->unloadAllImageFiles();
      streamImageNameIndex = 0;
      streamImageSlotLeft = STREAM_KEEP_IMAGE_COUNT;
      imageLayer->clear();
      //frameSizeSelectionLayer->select(currentFrameSizeIndex % 2, currentFrameSizeIndex / 2);
      frameSizeSelectionLayer->select(currentFrameSizeIndex);
    } else {
      dumbdisplay.logToSerial("... failed to initialize camera!");
    }
    if (cameraReady) {
      return eepromChanged;
    }
    dumbdisplay.writeComment("xxx camera not ready xxx");
    delay(5000);
    return eepromChanged;
  }

  if (autoSaveSelectionLayer->getFeedback() != NULL) {
    autoSave = !autoSave;
    eepromChanged = true;
    updateAutoSaveSelectionLayer();
  }  

  #if defined(SUPPORT_OFFLINE)
  if (offlineStorageInitialized && enableOfflineSelectionLayer->getFeedback() != NULL) {
    enableOffline = !enableOffline;
    eepromChanged = true;
    updateEnableOfflineSelectionLayer();
  }  
  #endif

  const DDFeedback* imageLayerFeedback = imageLayer->getFeedback();
  bool clickedSaveButton = saveButtonLayer->getFeedback() != NULL;
  bool clickedCancelButton = cancelButtonLayer->getFeedback() != NULL;

  if (state == STREAMING) {
    const DDFeedback* fb = frameSizeSelectionLayer->getFeedback();
    if (fb != NULL) {
      int newFrameSizeIndex = fb->y;
      if (newFrameSizeIndex != currentFrameSizeIndex) {
        currentFrameSizeIndex = newFrameSizeIndex;
        eepromChanged = true;
        cameraReady = false;
        return eepromChanged;
      }
    }
    fb = frameRateSelectionLayer->getFeedback();
    if (fb != NULL) {
      int newFrameRateIndex = 2 * fb->y + fb->x;
      if (newFrameRateIndex != currentCachePMFrameRateIndex) {
        frameRateSelectionLayer->select(newFrameRateIndex);
        customFrameRateSelectionLayer->deselect();
        currentCachePMFrameRateIndex = newFrameRateIndex;
        eepromChanged = true;
      }
    }
    fb = customFrameRateSelectionLayer->getFeedback();
    if (fb != NULL) {
      if (fb->type == CUSTOM) {
        int phFrameRate = fb->text.isEmpty() ? customCachePHFrameRate : fb->text.toInt();
        if (phFrameRate >= 1 && phFrameRate <= 3600) {
          customCachePHFrameRate = phFrameRate;
          currentCachePMFrameRateIndex = -1;
          eepromChanged = true;
          frameRateSelectionLayer->deselectAll();
          customFrameRateSelectionLayer->select();
          updateCustomFrameRateSelectionLayer();
        } else {
          dumbdisplay.log("invalid custom frame rate!", true);
        }
      } else {
        customFrameRateSelectionLayer->explicitFeedback(0, 0, "'per-hour' frame rate; e.g. " + String(customCachePHFrameRate) , CUSTOM, "numkeys");
      }
    }
    fb = cameraOptionSelectionLayer->getFeedback();
    if (fb != NULL) {
      int idx = fb->x;
      if (idx == 0) {
        cameraVFlip = !cameraVFlip;
        eepromChanged = true;
      } else if (idx == 1) {
        cameraHMirror = !cameraHMirror;
        eepromChanged = true;
      }
      resetCameraImageSettings();
      updateCameraOptionSelectionLayer();
    }
    fb = brightnessSliderLayer->getFeedback();
    if (fb != NULL) {
      int newBrightness = fb->x;
      if (newBrightness != cameraBrightness) {
        cameraBrightness = newBrightness;
        eepromChanged = true;
        updateBrightnessLabel();
        resetCameraImageSettings();
      }
    }
    fb = qualitySliderLayer->getFeedback();
    if (fb != NULL) {
      int newQuality = fb->x;
      if (newQuality != cameraQuality) {
        cameraQuality = newQuality;
        eepromChanged = true;
        updateQualityLabel();
        resetCameraImageSettings();
      }
    }
    camera_fb_t* camera_fb = esp_camera_fb_get();
    long fps_diffMs = -1;
    if (true) {
      fps_lastLastMs = fps_lastMs;
      fps_lastMs = millis();
      if (fps_lastLastMs != -1) {
        fps_diffMs = fps_lastMs - fps_lastLastMs;
      }
    }
    boolean cacheImage = true;
    int cameraFramePerHour = getCurrentPHFrameRate();
    long captureImageGapMs = (60 * 60 * 1000) / cameraFramePerHour;
    long diffMs = millis() - lastCachedImageMs;
    if ((captureImageGapMs - diffMs) > (captureImageGapMs / 5.0)) {  // 30ms is some allowance 
      cacheImage = false;
    }
    if (imageLayerFeedback != NULL) {
      imageLayer->flash();
      cacheImage = true;  // force cache one
    } 
    // if (!cachedImageOnce) {
    //   cacheImage = true;
    //   cachedImageOnce = true;
    // }
    bool justCachedImage = false;
    if (cacheImage) {
      setupCurrentStreamImageName();
      if (camera_fb != NULL) {
#if defined(MORE_LOGGING)
        bool timeTransfer = getCurrentPHFrameRate() <= 3600;        
        if (timeTransfer) {
          dumbdisplay.writeComment("{{ST}}");
        }
#endif    
        imageLayer->cacheImage(currentStreamImageFileName, camera_fb->buf, camera_fb->len);
#if defined(MORE_LOGGING)        
        if (timeTransfer) {
          dumbdisplay.writeComment(String("{{ET}}::bc:") + String(camera_fb->len));
        }
#endif
        imageLayer->drawImageFileFit(currentStreamImageFileName);
        if (fps_diffMs > 0) {
          int xOff = 20;
          int yOffset = IMAGE_LAYER_HEIGHT - 25;
          double fps = 1000.0 / (double) fps_diffMs;
          String str = "capture: " + String(fps) + " FPS  ";
          imageLayer->drawStr(xOff, yOffset, str, DD_COLOR_darkred, DD_COLOR_yellow);
        }
        if (true) {
          cache_lastLastMs = cache_lastMs;
          cache_lastMs = millis();
          if (cache_lastLastMs != -1) {
            long cache_diffMs = cache_lastMs - cache_lastLastMs;
            if (cache_diffMs > 0) {
              int xOff = IMAGE_LAYER_WIDTH / 2 + 10;
              int yOffset = IMAGE_LAYER_HEIGHT - 25;
              double fps = 1000.0 / (double) cache_diffMs;
              String str = " cache: " + String(fps) + " FPS  ";
              imageLayer->drawStr(xOff, yOffset, str, DD_COLOR_darkgreen, DD_COLOR_beige);
            }
          }
        }
#if defined(MORE_LOGGING_EX)
        dumbdisplay.logToSerial("$ currentStreamImageFileName: " + currentStreamImageFileName);
#endif
        lastCachedImageMs = millis();
        justCachedImage = true;
      } else {
        dumbdisplay.log("Failed to capture image!", true);
        delay(1000);
      }
    }
    if (imageLayerFeedback != NULL && imageLayerFeedback->type == CLICK) {
#if defined(MORE_LOGGING)
      dumbdisplay.logToSerial("~ FREEZE @ currentStreamImageFileName: " + currentStreamImageFileName);
#endif
      imageLayer->clear();
      imageLayer->drawImageFileFit(currentStreamImageFileName);
      setStateToConfirmSave();
    } else if (clickedSaveButton || (autoSave && justCachedImage)) {
      saveButtonLayer->flash();
      handleSaveImage(true);
    }
    if (camera_fb != NULL) {
      esp_camera_fb_return(camera_fb);
    }
    return eepromChanged;
  }

  if (state == CONFIRM_SAVE_SNAP) {
      if (clickedCancelButton) {
          setStateToStreaming();  // unfreeze and go back to streaming
      } else if (imageLayerFeedback != NULL) {
        if (imageLayerFeedback->type == DOUBLECLICK) {
          imageLayer->flash();
          setStateToStreaming();  // if double-click, unfreeze and go back to streaming
        }
      } else if (clickedSaveButton) {
        saveButtonLayer->flash();
        handleSaveImage(false);
      } else {
        const DDFeedback* fb = frameSliderLayer->getFeedback();
        if (fb != NULL) {
          int offset = fb->x - 1;
          if (setupCurrentStreamImageNameOfOffset(offset)) {
#if defined(MORE_LOGGING_EX)
            dumbdisplay.logToSerial("~ MOVE offset: " + String(offset) + " ... currentStreamImageFileName: " + currentStreamImageFileName);
#endif          
            imageLayer->drawImageFileFit(currentStreamImageFileName);
          }
        }
      }
      return eepromChanged;
  }

#if defined(SUPPORT_OFFLINE)
  if (state == TRANSFER_OFFLINE_SNAPS) {
    if (offlineSnapCount == 0 || clickedCancelButton) {
      if (offlineSnapCount == 0) {
        startOfflineSnapIdx = 0;
        writeOfflineSnapPosition();
      }
      setStateToStreaming();
      dumbdisplay.alert("Transferred " + String(startTransferOfflineSnapCount - offlineSnapCount) + " offline snaps!");
    } else {
      if (transferOneOfflineSnap()) {
        updateTransferProgress(false);
      } else {
        dumbdisplay.log("Failed to transfer offline snap!", true);
        setStateToStreaming();
      }
    }
    return eepromChanged;
  }
#endif

  return eepromChanged;
}

void deinitializeDD() {
 #if defined(SUPPORT_OFFLINE)  
  if (!enableOffline) {
    deinitializeCamera();
    cameraReady = false;
  }
#else
  deinitializeCamera();
  cameraReady = false;
#endif  
}

void handleIdle(bool justBecameIdle) {
    if (justBecameIdle) {
      String localTime;
      long now = getTimeNow(&localTime);
      dumbdisplay.logToSerial("*** just became idle ... millis=" + localTime + " ***");
      idleStartMillis = now;
    }
#if defined(SUPPORT_OFFLINE)  
  if (offlineStorageInitialized && enableOffline) {
    if (justBecameIdle) {
      if (!cameraReady) {
        framesize_t frameSize = cameraFrameSizes[currentFrameSizeIndex];
        cameraReady = initializeCamera(frameSize, cameraQuality); 
      }
      lastCachedImageMs = millis();  // treat it as just done so
    } else {
      if (cameraReady) {
        boolean saveImage = true;
        int cameraFramePerHour = getCurrentPHFrameRate();
        long captureImageGapMs = (60 * 60 * 1000) / cameraFramePerHour;
        long diffMs = millis() - lastCachedImageMs;
        if (lastCachedImageMs > 0 && (captureImageGapMs - diffMs) > 0) { 
          saveImage = false;
        }
        if (saveImage) {
          camera_fb_t* camera_fb = esp_camera_fb_get();
          saveOfflineSnap(camera_fb->buf, camera_fb->len);
          esp_camera_fb_return(camera_fb);
          lastCachedImageMs = millis();
        }
      }
    }
  }
#endif
#if defined(IDLE_SLEEP_SECS)   
    long diff = millis() - idleStartMillis;
    if (diff >= (1000 * IDLE_SLEEP_SECS)) {
      bool goToSleep = false;
  #if defined(SUPPORT_OFFLINE)  
      int cameraFramePerHour = getCurrentPHFrameRate();
      if (enableOffline && cameraFramePerHour <= MAX_SLEEP_PH_FRAME_RATE) {
        long allowanceMillis = SLEEP_WAKEUP_TAKE_SNAP_DELAY_MS + SLEEP_TIMER_OVERHEAD_MS;
        long sleepTimeoutMillis = ((60 * 60 * 1000) / cameraFramePerHour) - allowanceMillis;
        wakeupOfflineSnapMillis = sleepTimeoutMillis;
        if (lastCachedImageMs > 0) {
            long diffMs = millis() - lastCachedImageMs - allowanceMillis;
            if (diffMs > 0) {
              sleepTimeoutMillis = diffMs;
            }
        }
        Serial.println("*** set timer waking from sleep ...");
        Serial.println("    . sleepTimeoutMillis=" + sleepTimeoutMillis);
        Serial.println("    . wakeupOfflineSnapMillis=" + wakeupOfflineSnapMillis);
        esp_sleep_enable_timer_wakeup(sleepTimeoutMillis * 1000);  // in micro second
        goToSleep = true;
      }
  #endif   
      if (goToSleep) { 
        String localTime;
        getTimeNow(&localTime);
        Serial.println("*** going to sleep ... millis=" + localTime + " ***");
        Serial.flush();
        esp_deep_sleep_start();    
        // !!! the above call will not return !!!
      }
    }
#endif
}



// **********
// ***** for camera *****
// **********

#if defined(FOR_ESP32CAM)
  // for CAMERA_MODEL_AI_THINKER
  #define PWDN_GPIO_NUM     32      // power to camera (on/off)
  #define RESET_GPIO_NUM    -1      // -1 = not used
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26      // i2c sda
  #define SIOC_GPIO_NUM     27      // i2c scl
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25      // vsync_pin
  #define HREF_GPIO_NUM     23      // href_pin
  #define PCLK_GPIO_NUM     22      // pixel_clock_pin
#elif defined(FOR_TCAMERA)
  // for TCAMERA v1.7
  #define PWDN_GPIO_NUM     26
  #define RESET_GPIO_NUM    -1      // -1 = not used
  #define XCLK_GPIO_NUM     32
  #define SIOD_GPIO_NUM     13      // i2c sda
  #define SIOC_GPIO_NUM     12      // i2c scl
  #define Y9_GPIO_NUM       39
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       23
  #define Y6_GPIO_NUM       18
  #define Y5_GPIO_NUM       15
  #define Y4_GPIO_NUM        4
  #define Y3_GPIO_NUM       14
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    27      // vsync_pin
  #define HREF_GPIO_NUM     25      // href_pin
  #define PCLK_GPIO_NUM     19      // pixel_clock_pin
  //#define VFLIP
#elif defined(FOR_TCAMERAPLUS)
  // for T-CAMERA PLUS
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    -1      // -1 = not used
  #define XCLK_GPIO_NUM     4
  #define SIOD_GPIO_NUM     18      // i2c sda
  #define SIOC_GPIO_NUM     23      // i2c scl
  #define Y9_GPIO_NUM       36
  #define Y8_GPIO_NUM       37
  #define Y7_GPIO_NUM       38
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM       35
  #define Y4_GPIO_NUM       26
  #define Y3_GPIO_NUM       13
  #define Y2_GPIO_NUM       34
  #define VSYNC_GPIO_NUM    5       // vsync_pin
  #define HREF_GPIO_NUM     27      // href_pin
  #define PCLK_GPIO_NUM     25      // pixel_clock_pin
#elif defined(FOR_32S3EYE)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    -1      // -1 = not used
  #define XCLK_GPIO_NUM     15
  #define SIOD_GPIO_NUM      4      // i2c sda
  #define SIOC_GPIO_NUM      5      // i2c scl
  #define Y9_GPIO_NUM       16
  #define Y8_GPIO_NUM       17
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       12
  #define Y5_GPIO_NUM       10
  #define Y4_GPIO_NUM        8
  #define Y3_GPIO_NUM        9
  #define Y2_GPIO_NUM       11
  #define VSYNC_GPIO_NUM     6      // vsync_pin
  #define HREF_GPIO_NUM      7      // href_pin
  #define PCLK_GPIO_NUM     13      // pixel_clock_pin
#elif defined(FOR_XIAO_S3SENSE)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    -1      // -1 = not used
  #define XCLK_GPIO_NUM     10
  #define SIOD_GPIO_NUM     40      // i2c sda
  #define SIOC_GPIO_NUM     39      // i2c scl
  #define Y9_GPIO_NUM       48
  #define Y8_GPIO_NUM       11
  #define Y7_GPIO_NUM       12
  #define Y6_GPIO_NUM       14
  #define Y5_GPIO_NUM       16
  #define Y4_GPIO_NUM       18
  #define Y3_GPIO_NUM       17
  #define Y2_GPIO_NUM       15
  #define VSYNC_GPIO_NUM    38      // vsync_pin
  #define HREF_GPIO_NUM     47      // href_pin
  #define PCLK_GPIO_NUM     13      // pixel_clock_pin
#else
  #error board not supported  
#endif



bool resetCameraImageSettings() {
  sensor_t *s = esp_camera_sensor_get();
  if (s == NULL) {
    dumbdisplay.logToSerial("Error: problem reading camera sensor settings");
    return 0;
  }

  // enable auto adjust
  s->set_gain_ctrl(s, 1);                  // auto gain on
  s->set_exposure_ctrl(s, 1);              // auto exposure on
  s->set_awb_gain(s, 1);                   // Auto White Balance enable (0 or 1)
  s->set_brightness(s, cameraBrightness);  // (-2 to 2) - set brightness
  s->set_quality(s, cameraQuality);        // set JPEG quality (0 to 63)

  s->set_vflip(s, cameraVFlip);
  s->set_hmirror(s, cameraHMirror);

  //dumbdisplay.logToSerial("Done applying camera settings");

  return 1;
}


bool initializeCamera(framesize_t frameSize, int jpegQuality) {
  esp_camera_deinit();     // disable camera
  delay(50);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;               // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
  config.pixel_format = PIXFORMAT_JPEG;         // Options =  YUV422, GRAYSCALE, RGB565, JPEG, RGB888
  config.frame_size = frameSize;                // Image sizes: 160x120 (QQVGA), 128x160 (QQVGA2), 176x144 (QCIF), 240x176 (HQVGA), 320x240 (QVGA),
                                                //              400x296 (CIF), 640x480 (VGA, default), 800x600 (SVGA), 1024x768 (XGA), 1280x1024 (SXGA),
                                                //              1600x1200 (UXGA)
  config.jpeg_quality = jpegQuality/*15*/;      // 0-63 lower number means higher quality
  config.fb_count = 1;                          // if more than one, i2s runs in continuous mode. Use only with JPEG

  // check the esp32cam board has a psram chip installed (extra memory used for storing captured images)
  //    Note: if not using "AI thinker esp32 cam" in the Arduino IDE, SPIFFS must be enabled
  if (!psramFound()) {
    dumbdisplay.logToSerial("Error: No PSRam found!");
    return false;
  }

  esp_err_t camerr = esp_camera_init(&config);  // initialize the camera
  if (camerr != ESP_OK) {
    dumbdisplay.logToSerial("ERROR: Camera init failed with error -- " + String(camerr));
  }

  resetCameraImageSettings();                   // apply custom camera settings

  return (camerr == ESP_OK);                    // return boolean result of camera initialisation
}

void deinitializeCamera() {
  esp_camera_deinit();     // disable camera
  delay(50);
}



// **********
// ***** for EEPROM *****
// **********

void readOfflineSnapPosition() {
  startOfflineSnapIdx = EEPROM.readInt(24);
  offlineSnapCount = EEPROM.readInt(28);
  if (startOfflineSnapIdx < 0) {
    startOfflineSnapIdx = 0;
  }
  if (offlineSnapCount < 0) {
    offlineSnapCount = 0;
  }
}
void writeOfflineSnapPosition() {
  EEPROM.writeInt(24, startOfflineSnapIdx);
  EEPROM.writeInt(28, offlineSnapCount);
  EEPROM.commit();
// #if defined(MORE_LOGGING)  
//   dumbdisplay.logToSerial("EEPROM: written startOfflineSnapIdx=" + String(startOfflineSnapIdx) + "; offlineSnapCount=" + String(offlineSnapCount));
// #endif
}

void initRestoreSettings() {
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
  if (cameraBrightness < -2 || cameraBrightness > 2) {
    cameraBrightness = 0;
  }
  if (cameraQuality < 5 || cameraQuality > 60) {
    cameraQuality = INIT_CAMERA_QUALITY;
  }
  if (currentCachePMFrameRateIndex != -1 && (currentCachePMFrameRateIndex < 0 || currentCachePMFrameRateIndex > cachePMFrameRateCount)) {
    currentCachePMFrameRateIndex = INIT_FRAME_RATE_INDEX;
  }
  if (currentFrameSizeIndex < 0 || currentFrameSizeIndex > cameraFrameSizeCount) {
    currentFrameSizeIndex = INIT_FRAME_SIZE_INDEX;
  }
  if (customCachePHFrameRate < 0 || customCachePHFrameRate > 3600) {
    customCachePHFrameRate = MAX_SLEEP_PH_FRAME_RATE;
  }
  //dumbdisplay.logToSerial("EEPROM: init restored settings from EEPROM");
#if defined(SUPPORT_OFFLINE)
  readOfflineSnapPosition();
#endif
  //dumbdisplay.logToSerial("*** READ ... currentFrameSizeIndex=" + String(currentFrameSizeIndex));
}

void saveSettings() {
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
  //dumbdisplay.logToSerial("EEPROM: saved settings to EEPROM");
  //dumbdisplay.logToSerial("*** WRITE ... currentFrameSizeIndex=" + String(currentFrameSizeIndex));
}



#if defined(SUPPORT_OFFLINE)


// **********
// ***** for storage *****
// **********

bool initializeStorage() {
    dumbdisplay.logToSerial("Initialize offline snap storage ...");
    dumbdisplay.logToSerial("... offlineSnapCount=" + String(offlineSnapCount) + " ...");
#if defined(OFFLINE_USE_SD)
  #if defined(FOR_TCAMERAPLUS)
    spi.begin(21, 22, 19);
    bool successful = SD.begin(0, spi);
  #elif defined(FOR_XIAO_S3SENSE)
    spi.begin(7, 8, 9);
    bool successful = SD.begin(21, spi);
    //bool successful = SD.begin(21);
  #else
    bool successful = SD_MMC.begin();
  #endif    
    if (!successful) {
      dumbdisplay.logToSerial("... failed to begin() SD MMC ...");
      return false;
    }
    uint8_t cardType = STORAGE.cardType();
    if (cardType == CARD_NONE) {
      dumbdisplay.logToSerial("... no SD card ...");
      return false;
    }
    uint64_t total = STORAGE.totalBytes();
    uint64_t used = STORAGE.usedBytes();
    uint64_t free = total - used;
    dumbdisplay.logToSerial("    $ total: " + String(total / 1024) + " KB");
    dumbdisplay.logToSerial("    $ used: " + String(used / 1024) + " KB");
    dumbdisplay.logToSerial("    $ free: " + String(free / 1024) + " KB (" + String(100.0 * free / total) + "%)");
#else        
    if (offlineSnapCount > 0 && LittleFS.begin()) {
      dumbdisplay.logToSerial("... existing STORAGE ...");
      size_t total = LittleFS.totalBytes();
      size_t used = LittleFS.usedBytes();
      size_t free = total - used;
      dumbdisplay.logToSerial("    $ total: " + String(total / 1024) + " KB");
      dumbdisplay.logToSerial("    $ used: " + String(used / 1024) + " KB");
      dumbdisplay.logToSerial("    $ free: " + String(free / 1024) + " KB (" + String(100.0 * free / total) + "%)");
    } else {
      dumbdisplay.logToSerial("... force format ...");
      if (!LittleFS.begin(true)) {
        dumbdisplay.logToSerial("... unable to begin ... abort");
        //delay(2000);
        return false;
      }
      if (!LittleFS.format()) {
        dumbdisplay.logToSerial("... unable to format ... abort");
        //delay(2000);
        return false;
      }
    }    
#endif
    if (!verifyOfflineSnaps()) {
      dumbdisplay.logToSerial("... offline snap metadata looked invalid ...");
#if defined(OFFLINE_USE_SD)
#else
      dumbdisplay.logToSerial("... reformat ...");
      if (!LittleFS.format()) {
        dumbdisplay.logToSerial("... unable to format ... abort");
        return false;
      }
#endif
      offlineSnapCount = 0;
    }
    if (offlineSnapCount == 0) {
      startOfflineSnapIdx = 0;
      offlineSnapCount = 0;
      writeOfflineSnapPosition();
    }
    dumbdisplay.logToSerial("... offline snap storage ...");
    dumbdisplay.logToSerial("    - startOfflineSnapIdx=" + String(startOfflineSnapIdx));
    dumbdisplay.logToSerial("    - offlineSnapCount=" + String(offlineSnapCount));
    dumbdisplay.logToSerial("... offline snap storage initialized");
    offlineStorageInitialized = true;
    return true;
}

void getOfflineSnapFileName(String& offlineSnapFileName, int offlineSnapIdx) {
  String seq = offlineSnapIdx < 1000 ? String(1000 + offlineSnapIdx).substring(1) : String(offlineSnapIdx);
  offlineSnapFileName = "/" + offlineSnapNamePrefix + seq + ".jpg";
}
bool checkFreeStorage() {
#if defined(OFFLINE_USE_SD)
    uint64_t total = STORAGE.totalBytes();
    uint64_t used = STORAGE.usedBytes();
    uint64_t free = total - used;
#else
    size_t total = LittleFS.totalBytes();
    size_t used = LittleFS.usedBytes();
    size_t free = total - used;
#endif
    return free >= MIN_STORE_FREE_BYTE_COUNT;
}
bool verifyOfflineSnaps() {
  if (offlineSnapCount == 0) {
    return true;
  }
  String snapFileName0;
  getOfflineSnapFileName(snapFileName0, startOfflineSnapIdx);
  String snapFileNameN;
  getOfflineSnapFileName(snapFileNameN, startOfflineSnapIdx + offlineSnapCount - 1);
#if defined(OFFLINE_USE_SD)
  bool verified = STORAGE.exists(snapFileName0) && STORAGE.exists(snapFileNameN);
#else
  bool verified = LittleFS.exists(snapFileName0) && LittleFS.exists(snapFileNameN);
#endif
  return verified;
}
void saveOfflineSnap(uint8_t *bytes, int byteCount) {
  if (!offlineStorageInitialized) {
    dumbdisplay.logToSerial("! storage for offline snap not ready !");
    return;
  }
  if (!checkFreeStorage()) {
    dumbdisplay.logToSerial("! insufficient storage for offline snap !");
    return;
  }
  String offlineSnapFileName;
  getOfflineSnapFileName(offlineSnapFileName, startOfflineSnapIdx + offlineSnapCount);
#if defined(OFFLINE_USE_SD)
  File f = STORAGE.open(offlineSnapFileName, FILE_WRITE);
#else
  File f = LittleFS.open(offlineSnapFileName, "w");
#endif
  if (f) {
    f.write(bytes, byteCount);
    f.flush();
    int size = f.size();
    long ts = f.getLastWrite();
#if defined(MORE_LOGGING)    
    dumbdisplay.logToSerial(String("! written offline snap to [") + offlineSnapFileName + "] ... size=" + String(size) + "; ts=" + String(ts));
#else
    dumbdisplay.logToSerial(String("! written offline snap to [") + offlineSnapFileName + "] ... size=" + String(size / 1024.0) + "KB");
#endif
    f.close();
    offlineSnapCount = offlineSnapCount + 1;
    writeOfflineSnapPosition();
  } else {
    dumbdisplay.logToSerial(String("unable to open file [") + offlineSnapFileName + "] for offline snap");
  }
}
bool retrieveOfflineSnap(const String& offlineSnapFileName, uint8_t*& bytes, int& byteCount, long& ts) {
  if (!offlineStorageInitialized) {
    dumbdisplay.logToSerial("! storage for offline snap not ready !");
    return false;
  }
#if defined(OFFLINE_USE_SD)
  File f = STORAGE.open(offlineSnapFileName, FILE_READ);
#else
  File f = LittleFS.open(offlineSnapFileName, "r");
#endif
  if (f) {
    byteCount = f.size();
    ts = f.getLastWrite();
    uint8_t* buf = new uint8_t[byteCount];
    f.readBytes((char*) buf, byteCount);
    bytes = buf;
    f.close();
#if defined(OFFLINE_USE_SD)
    STORAGE.remove(offlineSnapFileName);
#else
    LittleFS.remove(offlineSnapFileName);
#endif
    startOfflineSnapIdx = startOfflineSnapIdx + 1;
    offlineSnapCount = offlineSnapCount - 1;
    writeOfflineSnapPosition();
    return true;
  } else {
    return false;
  }
}
bool transferOneOfflineSnap() {
  if (offlineSnapCount == 0) {
    return true;
  }
  String offlineSnapFileName;
  getOfflineSnapFileName(offlineSnapFileName, startOfflineSnapIdx);
  String composedImageFileName = transferImageFileNamePrefixForOfflineSnaps + offlineSnapFileName;
  uint8_t* bytes = NULL;
  int byteCount = 0;
  long ts;
  if (!retrieveOfflineSnap(offlineSnapFileName, bytes, byteCount, ts)) {
    dumbdisplay.logToSerial("Failed to read offline snap [" + offlineSnapFileName + "] to transfer to [" + composedImageFileName + "]");
    return false;
  }
  int64_t imageTimestamp = 1000 * (int64_t) (ts - 60 * tzMins);  // imageTimestamp should be in ms
#if defined(MORE_LOGGING)
  dumbdisplay.logToSerial("* transfer " + String(byteCount / 1024.0) + "KB (ts=" + String(ts) + ";imageTS=" + String(imageTimestamp) + ") offline snap [" + offlineSnapFileName + "] to transfer to [" + composedImageFileName + "]");
#endif
  imageLayer->cacheImage(offlineSnapFileName, bytes, byteCount);
  delete bytes;
  imageLayer->drawImageFileFit(offlineSnapFileName);
  imageLayer->saveCachedImageFileWithTS(offlineSnapFileName, composedImageFileName, imageTimestamp);
  return true;
}

#endif