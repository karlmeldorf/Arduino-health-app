#include "Arduino.h"
#include <Adafruit_GFX_AS.h>
#include <Adafruit_ILI9341_AS.h>

class Controller
{
  public:
    Controller(char* spo2MAC, char* spo2BP, char* user);
    ~Controller();

    void initTFT();
    void setupWifi(String ssid, String password);
    void setupBluetooth();
    void run();
    void runSPO2();
    void runBP();

  private:
    bool sendData(const char* command, const int timeout, boolean debug);
    void httppost(String& api, String& requestBody);

  private:
    int SPO2_HANDLE;
    int BP_HANDLE;
    char* MAC_SPO2;
    char* MAC_BP;
    char* m_user;
    uint8_t available_spo2;
    uint8_t connected_spo2;
    uint8_t connection_handle_spo2;
    uint8_t pulse_spo2;
    uint8_t spo2;
    uint8_t available_bp;
    uint8_t connected_bp;
    uint8_t connection_handle_bp;
    struct bloodPressureBLEDataVector
    {
      uint16_t systolic;
      uint16_t diastolic;
      uint16_t pulse;
    };
    bloodPressureBLEDataVector bloodPressureBLEData;
    char buffer_tft[26];
    Adafruit_ILI9341_AS tft;
};
