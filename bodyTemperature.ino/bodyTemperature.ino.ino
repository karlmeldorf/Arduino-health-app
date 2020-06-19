//Include libraries
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 15
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
// Sensor mac: 0x28, 0x69, 0x41, 0x35, 0x05, 0x00, 0x00, 0x93

// punane ja roheline vilguvad kui mootmine kaib, lõpuks kui palavik (tmp < 36 || tmp > 37), siis põleb punane ja muidu roheline
const int ledRed = 18;
const int ledGreen = 19;

float previousTemp = 0.0;
float temp = 0.0;
int tempHasntChangedSecs = 0;

//WiFi
const char* ssid      = "TRAPHOUSE";
const char* password  = "pussymoneyweed";

void setup(void)
{
  Serial.begin(9600); //Begin serial communication
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  Serial.println("Arduino Digital Temperature // Serial Monitor Version"); //Print a message
  sensors.begin();

  setupWIFI(ssid, password);
}

void loop(void)
{ 
  digitalWrite(ledRed, HIGH);
  delay(500);
  digitalWrite(ledRed, LOW);

  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0);

  bool isFinished = tempReached();

  if (isFinished) {
    Serial.print("Finished. Temperature is: ");
    Serial.println(temp);

    if (temp > 36.0 || temp < 37.0) {
      digitalWrite(ledGreen, HIGH);
    } else {
      digitalWrite(ledRed, HIGH);
    }

    httppost(temp + 0.3);
    delay(60000);
    
    digitalWrite(ledGreen, LOW);
    digitalWrite(ledRed, LOW);
  } else {
    Serial.print("Temperature is: ");
    Serial.println(temp);
    Serial.print("Counter: ");
    Serial.println(tempHasntChangedSecs);
  }
  
  digitalWrite(ledGreen, HIGH);
  delay(500);
  digitalWrite(ledGreen, LOW);
}

bool tempReached() {

  if (temp >= 32.0 && temp <= 45.0) {
    if (temp == previousTemp) {
      tempHasntChangedSecs += 1;
    } else {
      tempHasntChangedSecs = 0;
    }
    
    if (tempHasntChangedSecs == 15) {
        return true;
    } 
  }

  previousTemp = temp;
  return false;
}

void httppost(float temperature) {
if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status
  
   HTTPClient http;   
  
   http.begin("https://arduino-health-app.herokuapp.com/api/temperature/result");  //Specify destination for HTTP request
   http.addHeader("Content-Type", "application/json");             //Specify content-type header
  
   int httpResponseCode = http.POST("{\"temperature\": " + String(temperature) + "," + "user: testUser" + "}");   //Send the actual POST request
  
   if(httpResponseCode>0){
  
    String response = http.getString();                       //Get the response to the request
  
    Serial.println(httpResponseCode);   //Print return code
    Serial.println(response);           //Print request answer
  
   }else{
  
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  
   }
  
   http.end();  //Free resources
  
 }else{
  
    Serial.println("Error in WiFi connection");   
  
 }
}

void setupWIFI(const char* ssid, const char* password)
{
  Serial.println("");
  
  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println("");
  Serial.print("WiFi connected with IP address: ");
  Serial.println(WiFi.localIP());
}
