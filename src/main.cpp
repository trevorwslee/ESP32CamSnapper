// ***
// the below _secret.h just define macros like:
// #define WIFI_SSID           "your-wifi-ssid"
// #define WIFI_PASSWORD       "your-wifi-password"
// ***
#include "_secret.h"

#if !defined(FOR_S3EYE) && !defined(FOR_XIAO_S3SENSE)
  // *** use Bluetooth with the following device name
  #define BLUETOOTH "ESP32CamSnapper"
#endif

#if defined(FOR_ESP32CAM)
  // *** ESP32CAM has SD
  //#define OFFLINE_USE_SD
#elif defined(FOR_TCAMERAPLUS)
  // *** LILYGO TCameraPlus has SD
  //#define OFFLINE_USE_SD
#elif defined(FOR_XIAO_S3SENSE)
  // *** Xiao S3Sense has SD
  //#define OFFLINE_USE_SD
#endif

//#define MORE_LOGGING
//#define DISABLE_OFFLINE_UPON_RESET

#include "esp32camsnapper/esp32camsnapper.ino"
