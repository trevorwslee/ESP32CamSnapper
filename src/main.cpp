
// // ***
// // the below _secret.h just define macros like:
// // #define WIFI_SSID           "your-wifi-ssid"
// // #define WIFI_PASSWORD       "your-wifi-password"
// // ***
// #include "_secret.h"

#ifdef FOR_ESP32CAM
  #define OFFLINE_USE_SD_MMC 
#endif

#define MORE_LOGGING


#include "esp32camsnapper/esp32camsnapper.ino"
