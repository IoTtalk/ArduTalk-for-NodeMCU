#include <ESP8266WiFi.h>
 
void setup(){
    Serial.begin(115200);
 }
 
void loop(){
   delay(2000);
   Serial.print("MAC: ");
   Serial.println(WiFi.macAddress());
  }
