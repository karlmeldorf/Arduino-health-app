#include "controller.h"

#include <MySignals.h>
#include <MySignals_BLE.h>

const char* ssid      = "TRAPHOUSE";
const char* password  = "roosipoisid";
const char* user      = "testUser";

char _MAC_SPO2[14] = "00A050042F13";
char _BP_MAC[14] = "508CB1664C02";

Controller controller(_MAC_SPO2, _BP_MAC, user);

void setup() {
  Serial.begin(115200);
  MySignals.begin();
  controller.initTFT();  
  controller.setupBluetooth();
  controller.setupWifi(ssid, password);
  MySignals.enableSensorUART(BLE);
}

void loop() {
  controller.run();
}
