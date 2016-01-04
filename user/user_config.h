#include "debug.h"
#define USE_DNS false // if true, device will try to resolve REMOTE_HOST, otherwise REMOTE_IP will be used.
#define REMOTE_PORT 80 // server port
#define REMOTE_HOST "wisense" // only if USE_DNS true
#define REMOTE_IP "192.168.0.11" // only used if USE_DNS false
#define HOST_NAME "svecomp17" // name of this device
#define DATA_URL "/data" // relative PATH of POST request.
#define SSID "svesoftware" // wifi name
#define PASSWORD "" // wifi password
#define SLEEP_SEC 600 // 10 minutes
