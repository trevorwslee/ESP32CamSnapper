
// ***
// the below _secret.h just define macros like:
// #define WIFI_SSID           "your-wifi-ssid"
// #define WIFI_PASSWORD       "your-wifi-password"
// ***
#include "_secret.h"

#define BLUETOOTH "ESP32CamSnapper"

#if defined(FOR_ESP32CAM)
  #define OFFLINE_USE_SD
#elif defined(FOR_TCAMERAPLUS)
  #define OFFLINE_USE_SD
#endif

//#define MORE_LOGGING


#include "esp32camsnapper/esp32camsnapper.ino"
