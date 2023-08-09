#include "updateControllers.h"
#include "LittleFS.h"
AsyncWebServer server(80);
const char *ssid = "controller_device";
const char *password = "strongpassword";

bool serverStarted;
bool cabinVersionAvailable;

