#include "controller.h"

#include <MySignals.h>
#include <MySignals_BLE.h>

Controller::Controller(char* _MAC_SPO2, char* _MAC_BP, char* user) 
: SPO2_HANDLE(15)
, BP_HANDLE(18)
, MAC_SPO2(_MAC_SPO2)
, MAC_BP(_MAC_BP)
, m_user(user)
, available_spo2(0)
, connected_spo2(0)
, connection_handle_spo2(0)
, pulse_spo2(0)
, spo2(0)
, available_bp(0)
, connected_bp(0)
, connection_handle_bp(0)
, bloodPressureBLEData()
, buffer_tft()
, tft(Adafruit_ILI9341_AS(TFT_CS, TFT_DC))
{
}

Controller::~Controller() {}

bool Controller::sendData(const char* command, const int timeout, boolean debug)
{
  String response = "";
  bool answer = false;

  while ( Serial.available() > 0) Serial.read();
  
  Serial.println(command); // send the read character to the esp8266
  
  long int time = millis();
  
  while( (time+timeout) > millis())
  {
    while(Serial.available())
    {
      // The esp has data so display its output to the serial window
      char c = Serial.read(); // read the next character.
      response+=c;
    }
  }
  
  if(debug)
  {
    MySignals.println(response.c_str());
  }
  
  return strstr(response.c_str(), "OK");
}

void Controller::initTFT() {
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

  //TFT message: Welcome to MySignals
  strcpy_P((char*)buffer_tft, (char*)pgm_read_word(&(table_MISC[0])));
  tft.drawString(buffer_tft, 0, 0, 2); 
}

void Controller::setupWifi(String ssid, String password) {
  bitSet(MySignals.expanderState, EXP_ESP8266_POWER);
  MySignals.expanderWrite(MySignals.expanderState);

  //MySignals.initSensorUART();
  MySignals.enableSensorUART(WIFI_ESP8266);
  
  delay(500);

  String cwjap = "AT+CWJAP=\"" + ssid + "\",\"" + password + "\"";

  sendData("AT", 1000, true);
  sendData("AT+CWMODE=1", 1000, true);
  if (sendData(cwjap.c_str(), 10000, true)) {
    tft.drawString("WiFi ok", 0, 30, 2);
  }
}

void Controller::setupBluetooth() {
  //Enable BLE module power -> bit6: 1
  bitSet(MySignals.expanderState, EXP_BLE_POWER);
  MySignals.expanderWrite(MySignals.expanderState);

  //Enable BLE UART flow control -> bit5: 0
  bitClear(MySignals.expanderState, EXP_BLE_FLOW_CONTROL);
  MySignals.expanderWrite(MySignals.expanderState);

  //Disable BLE module power -> bit6: 0
  bitClear(MySignals.expanderState, EXP_BLE_POWER);
  MySignals.expanderWrite(MySignals.expanderState);
  
  delay(500);

  //Enable BLE module power -> bit6: 1
  bitSet(MySignals.expanderState, EXP_BLE_POWER);
  MySignals.expanderWrite(MySignals.expanderState);

  delay(500);

  MySignals.initSensorUART();
  MySignals.enableSensorUART(BLE);
  MySignals_BLE.initialize_BLE_values();

  if (MySignals_BLE.initModule() == 1 && MySignals_BLE.sayHello() == 1) {
      MySignals.disableMuxUART();
      Serial.println(F("BLE init ok"));
      MySignals.enableMuxUART();
      strcpy_P((char*)buffer_tft, (char*)pgm_read_word(&(table_MISC[1])));
      tft.drawString(buffer_tft, 0, 15, 2);
  } else {
      MySignals.disableMuxUART();
      Serial.println(F("BLE init fail"));
      MySignals.enableMuxUART();
      strcpy_P((char*)buffer_tft, (char*)pgm_read_word(&(table_MISC[2])));
      tft.drawString(buffer_tft, 0, 15, 2);
      while (1);
  }
}

void Controller::httppost(String& api, String& requestBody) {
  
  String request = "POST ";
  request += api;
  request += " HTTP/1.1\r\nHost: arduino-health-app.herokuapp.com\r\nContent-Type: application/json\r\nContent-Length: ";
  request += requestBody.length();
  request += "\r\n\r\n";
  request += requestBody;

  String sendPostRequest = "AT+CIPSEND=";
  sendPostRequest += request.length();

  const char* cipStart = "AT+CIPSTART=\"TCP\",\"arduino-health-app.herokuapp.com\",80";

  sendData(cipStart, 2000, true);
  sendData(sendPostRequest.c_str(), 6000, true);
  sendData(request.c_str(), 4000, true);
  sendData("AT+CIPCLOSE", 1000, true);
}

void Controller::run() {
  available_spo2 = MySignals_BLE.scanDevice(MAC_SPO2, 1000, TX_POWER_MAX);
  available_bp = MySignals_BLE.scanDevice(MAC_BP, 1000, TX_POWER_MAX);

  if (available_spo2 == 1) runSPO2();
  if (available_bp == 1) runBP();

  delay(500);
}

void Controller::runSPO2() {

  MySignals.disableMuxUART();
  Serial.print(F("SPO2 available:"));
  Serial.println(available_spo2);
  MySignals.enableMuxUART();

  if (available_spo2 == 1)
  {
    MySignals.disableMuxUART();
    Serial.println(F("SPO2 found.Connecting"));
    MySignals.enableMuxUART();

    if (MySignals_BLE.connectDirect(MAC_SPO2) == 1)
    {
      connected_spo2 = 1;
      connection_handle_spo2 = MySignals_BLE.connection_handle;

      MySignals.disableMuxUART();
      Serial.println(F("Connected"));
      MySignals.enableMuxUART();

      delay(6000);

      //To subscribe the spo2 measure write "1" in SPO2_HANDLE
      char attributeData[1] =
      {
        0x01
      };

      if (MySignals_BLE.attributeWrite(connection_handle_spo2, SPO2_HANDLE, attributeData, 1) == 0)
      {
        unsigned long previous = millis();
        do
        {
          if (MySignals_BLE.waitEvent(1000) == BLE_EVENT_ATTCLIENT_ATTRIBUTE_VALUE)
          {


            char attributeData[1] = {  0x00 };

            MySignals_BLE.attributeWrite(connection_handle_spo2, SPO2_HANDLE, attributeData , 1);

            uint8_t pulse_low = MySignals_BLE.event[12];
            pulse_low &= 0b01111111;

            uint8_t pulse_high = MySignals_BLE.event[11];
            pulse_high &= 0b01000000;

            if (pulse_high == 0)
            {
              pulse_spo2 = pulse_low;
            }

            if (pulse_high == 0b01000000)
            {
              pulse_spo2 = pulse_low + 0b10000000;
            }

            spo2 = MySignals_BLE.event[13];
            spo2 &= 0b01111111;

            if ((pulse_spo2 >= 25) && (pulse_spo2 <= 250)
                && (pulse_spo2 >= 35) && (pulse_spo2 <= 100))
            {
              MySignals.disableMuxUART();

              Serial.println();
              Serial.print(F("SpO2: "));
              Serial.print(spo2);
              Serial.print(F("%  "));
              Serial.print(F("Pulse: "));
              Serial.print(pulse_spo2);
              Serial.println(F("ppm  "));

              sprintf(buffer_tft, "Pulse: %d ppm / SPO2: %d", pulse_spo2, spo2);
              tft.drawString(buffer_tft, 0, 60, 2);

              uint16_t errorCode = MySignals_BLE.disconnect(connection_handle_spo2);

              Serial.print(F("Disconnecting error code: "));
              Serial.println(errorCode, HEX);

              MySignals.enableMuxUART();
              connected_spo2 = 0;
              
              MySignals.enableSensorUART(WIFI_ESP8266);
              String api = "/api/spo2/result";
              String requestBody = "{\"spo2\": ";
              requestBody += String(spo2);
              requestBody += ",\"pulse\": "; 
              requestBody += String(pulse_spo2);
              requestBody += ",\"user\": ";
              requestBody += "\"" + String(m_user) + "\"";
              requestBody += "}\r\n";
              httppost(api, requestBody);
              MySignals.enableSensorUART(BLE);
            }
          }
        }
        while ((connected_spo2 == 1) && ((millis() - previous) < 10000));

        connected_spo2 = 0;

      }
      else
      {
         MySignals.disableMuxUART();
         Serial.println(F("Subscribed"));
         MySignals.enableMuxUART();
      }
    }
    else
    {
       connected_spo2 = 0;
       MySignals.disableMuxUART();
       Serial.println(F("Not Connected"));
       MySignals.enableMuxUART();
    }
  }
  else if (available_spo2 == 0)
  {
    //Do nothing
  }
  else
  {
    
    MySignals_BLE.hardwareReset();
    MySignals_BLE.initialize_BLE_values();
    delay(100);
    
  }
}

void Controller::runBP() {
  
  MySignals.disableMuxUART();
  Serial.print(F("BP available:"));
  Serial.println(available_bp);
  MySignals.enableMuxUART();


  if (available_bp == 1)
  {

    if (MySignals_BLE.connectDirect(MAC_BP) == 1)
    {
      MySignals.disableMuxUART();
      Serial.println(F("Connected"));
      MySignals.enableMuxUART();
      
      connected_bp = 1;
      delay(8000);
      //To subscribe the BP measure write an ASCII letter "e"
      if (MySignals_BLE.attributeWrite(MySignals_BLE.connection_handle,  BP_HANDLE, "e", 1) == 0)
      {

       MySignals.disableMuxUART();
       Serial.println(F("Subscribed"));
       MySignals.enableMuxUART();

        unsigned long previous = millis();
        do
        {

          if (MySignals_BLE.waitEvent(1000) == BLE_EVENT_ATTCLIENT_ATTRIBUTE_VALUE)
          {
            //Search letter "g" in ASCII
            if (MySignals_BLE.event[9] == 0x67)
            {

              if (MySignals_BLE.event[10] == 0x2f)
              {
                //Ojo esto esta mal en la guia de comandos

                //Primero da la alta -> sistolica
                uint8_t sh = MySignals_BLE.event[11] - 48;
                uint8_t sm = MySignals_BLE.event[12] - 48;
                uint8_t sl = MySignals_BLE.event[13] - 48;
                bloodPressureBLEData.systolic = (sh * 100) + (sm * 10) + sl;

                //Primero da la baja -> diastolica
                uint8_t dh = MySignals_BLE.event[15] - 48;
                uint8_t dm = MySignals_BLE.event[16] - 48;
                uint8_t dl = MySignals_BLE.event[17] - 48;
                bloodPressureBLEData.diastolic = (dh * 100) + (dm * 10) + dl;

                uint8_t ph = MySignals_BLE.event[19] - 48;
                uint8_t pm = MySignals_BLE.event[20] - 48;
                uint8_t pl = MySignals_BLE.event[21] - 48;
                bloodPressureBLEData.pulse = (ph * 100) + (pm * 10) + pl;


                MySignals.disableMuxUART();
                Serial.print(F("Systolic: "));
                Serial.println(bloodPressureBLEData.systolic);

                Serial.print(F("Diastolic: "));
                Serial.println(bloodPressureBLEData.diastolic);

                Serial.print(F("Pulse/min: "));
                Serial.println(bloodPressureBLEData.pulse);
                Serial.println(F("Disconnected from device"));
                MySignals.enableMuxUART();

                sprintf(buffer_tft, "S:%d D:%d P:%d      ", bloodPressureBLEData.systolic, bloodPressureBLEData.diastolic, bloodPressureBLEData.pulse);
                tft.drawString(buffer_tft, 0, 60, 2);
                
                connected_bp = 0;

              }
            }
          }
        }
        while ((connected_bp == 1) && ((millis() - previous) < 40000));


        MySignals_BLE.attributeWrite(MySignals_BLE.connection_handle,  BP_HANDLE, "i" , 1);
        delay(100);


        MySignals_BLE.disconnect(MySignals_BLE.connection_handle);

        connected_bp = 0;

      }
      else
      {
       MySignals.disableMuxUART();
       Serial.println(F("Subscribed"));
       MySignals.enableMuxUART();
      }

    }
    else
    {
       connected_bp = 0;
       MySignals.disableMuxUART();
       Serial.println(F("Not Connected"));
       MySignals.enableMuxUART();
    }
  }
  else if (available_bp == 0)
  {
    //Do nothing
  }
  else
  {
    MySignals_BLE.hardwareReset();
    MySignals_BLE.initialize_BLE_values();
    delay(100);

  }
  
  delay(1000);
  // make a POST request
  MySignals.enableSensorUART(WIFI_ESP8266);
  String api = "/api/bp/result";
  // request format example: "systolic": 115,"diastolic": 65,"pulse": 68
  String requestBody = "{\"s\": ";
  requestBody += String(bloodPressureBLEData.systolic);
  requestBody += ",\"d\": "; 
  requestBody += String(bloodPressureBLEData.diastolic);
  requestBody += ",\"p\": ";
  requestBody += String(bloodPressureBLEData.pulse); 
  requestBody += ",\"user\": ";
  requestBody += "\"" + String(m_user) + "\"";
  requestBody += "}\r\n";
  httppost(api, requestBody);
  MySignals.enableSensorUART(BLE);
  
  delay(2000);
}
