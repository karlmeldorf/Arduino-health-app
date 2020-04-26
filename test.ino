#include "controller.h"

#include <MySignals.h>
#include <MySignals_BLE.h>

const char* ssid      = "";
const char* password  = "";

String _server = "";
char _MAC_SPO2[14] = "00A050042F13";
char _BP_MAC[14] = "508CB1664C02";


Controller controller(_MAC_SPO2, _BP_MAC, _server);

void setup() {
  Serial.begin(115200);
  MySignals.begin();
  controller.setupBluetooth();
  controller.setupWifi(ssid, password);
  MySignals.enableSensorUART(BLE);
}

void loop() {
  controller.run();
}
