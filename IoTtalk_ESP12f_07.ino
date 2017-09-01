/*
 * IoTtalk V1 - ESP12F Version 7.0 
 */
 
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>
#include "ESP8266HTTPClient2.h"
#include <EEPROM.h>

char wifissid[100]="";
char wifipass[100]="";
String url = "http://140.113.199.199:9999/";
HTTPClient http;
uint8_t wifimode = 1; //1:AP , 0: STA 
unsigned long five_min;

String softapname = "IoTtalk-ESP12F";
IPAddress ip(192,168,0,1);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server ( 80 );

void store_ap_ssid_pass(void){
  // store form: [SSID,PASS,server IP]
  int addr=0,i,j;


  EEPROM.write(addr++,'[');
  
  for (i=0;i<3;i++){
    while(server.arg(i)[j] != '\0')
      EEPROM.write(addr++,server.arg(i)[j]);
    if(i<2)
      EEPROM.write(addr++,',');
  }
  
  EEPROM.write(addr,']');
  EEPROM.commit();
  
  /*
  String S_ssid_pass = "[" + server.arg(0) + "," + server.arg(1) +"]";
  char c_ssid_pass[50];
  
  S_ssid_pass.toCharArray(c_ssid_pass,50);
  Serial.println(c_ssid_pass);

  //store data in eeprom
  while (c_ssid_pass[addr] != ']'){
    EEPROM.write(addr,c_ssid_pass[addr]);
    addr++;
  }
  EEPROM.write(addr,']');// c_ssid_pass[addr++] == ']'
  */
}

boolean read_ap_ssid_pass(void){
  // store form: [SSID,PASS]
  String readdata="";
  char temp;
  int addr=0;
  
  temp = EEPROM.read(addr++);
  if(temp == '['){
    while(1){
      temp = EEPROM.read(addr++);
      if(temp == ',')
        break;
      readdata += temp;
    }
    
    readdata.toCharArray(wifissid,100);
    readdata ="";
    
    while(1){
      temp = EEPROM.read(addr++);
      if(temp == ']')
        break;
      readdata += temp;
    }
    readdata.toCharArray(wifipass,100);
    
    return true;
  }
  else{
    Serial.println(temp);
    Serial.println("no data in eeprom");
    return false;
  }
}

void clr_eeprom(void){
  int addr;
  delay(3000);
  if(digitalRead(13) == LOW){// clear eeprom
    for(addr=0;addr<50;addr++)
      EEPROM.write(addr,0);
    EEPROM.commit();
  }
}

String scan_network(void){
  /*output string example
   * 
   * <select name="SSID">
   *  <option value="">請選擇</option>
   *  <option value="Bryan">Bryan</option>
   *  <option value="Stanley">Stanley</option>
   *  <option value="Yaue">Yaue</option>
   * </select><br>
   */
  int AP_N,i;       //AP_N: AP number 
  String AP_List="<select name=\"SSID\" style=\"width: 150px;\">" ;// make ap_name in a string
  AP_List += "<option value=\"\">請選擇</option>";
  
  WiFi.disconnect();
  delay(100);
  AP_N = WiFi.scanNetworks();
  
  if(AP_N>0)
    for (i=0;i<AP_N;i++)
      AP_List += "<option value=\""+WiFi.SSID(i)+"\">" + WiFi.SSID(i) + "</option>";
  else
    AP_List = "<option value="">NO AP</option>";
  
  AP_List +="</select><br><br>";
  return(AP_List); 
}

void handleRoot() {
  String temp="<html><form action=\"action_page.php\">";
  temp += "<div style=\"width:400px; height:150px;margin:0px auto;\"><div>";
  temp += "SSID:<br>";
  temp += scan_network();
  temp += "Password:<br>";
  temp += "<input type=\"password\" name=\"Password\" vplaceholder=\"輸入AP密碼\" style=\"width: 150px;\">";
  temp += "<br><br><input type=\"submit\" value=\"Submit\">";
  temp += "</div></div></form></html>";
  server.send ( 200, "text/html", temp );
}

void handleNotFound() {
  if (server.arg(0) != ""){//arg[0]-> SSID, arg[1]-> password (both string)
    server.arg(0).toCharArray(wifissid,100);
    server.arg(1).toCharArray(wifipass,100);
    connect_to_wifi();
  }
}

void wifi_setting(void){
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  WiFi.softAPConfig(ip,gateway,subnet);
  WiFi.softAP(&softapname[0]);
  
  if ( MDNS.begin ( "esp8266" ) ) 
    Serial.println ( "MDNS responder started" );
    
  server.on ( "/", handleRoot );
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "AP started" );
}

void connect_to_wifi(void){
  long connecttimeout = millis();
  Serial.println("-----Connect to Wi-Fi-----");
  WiFi.begin(wifissid, wifipass);
  
  while (WiFi.status() != WL_CONNECTED && (millis() - connecttimeout < 10000) ) {
      delay(1000);
      Serial.print(".");
  }
  
  if(WiFi.status() == WL_CONNECTED){
    Serial.println ( "Connected!\n");
    store_ap_ssid_pass();
    wifimode = 0;
  }
  else if (millis() - connecttimeout > 10000){
    Serial.println("Connect fail");
    wifi_setting();
  }
  
  //Serial.println(WiFi.localIP());
}

void iottalk_register(void){
  
  uint8_t MAC_array[6];
  int i;
  if(WiFi.status() !=WL_CONNECTED)
    connect_to_wifi();
  
  //Append the mac address to url string
  WiFi.macAddress(MAC_array);//get esp12f mac address
  for (i=0;i<6;i++){
    if( MAC_array[i]<0x10 )
      url+="0";
    url+=String(MAC_array[i],HEX);
  }
  
  //send the register packet
  Serial.println("[HTTP] begin...");
  Serial.println("[HTTP] POST..." + url);
  
  http.begin(url);
  http.addHeader("Content-Type","application/json");
  int httpCode = http.POST("{\"profile\": {\"d_name\": \"01.ESP12F\", \"dm_name\": \"ESP12F\", \"is_sim\": false, \"df_list\": [\"esp12f_LED\"]}}");
  Serial.println("[HTTP] POST... code:" + (String)httpCode );
  Serial.println(http.getString());
  http.end();
  
  url +="/esp12f_LED";
  Serial.println("[HTTP] URL: "+url);
  
}


void setup() {
  EEPROM.begin(512);
  Serial.begin(115200);
  pinMode(2, OUTPUT);//GPIO2 : on board led
  pinMode(13, INPUT);//GPIO13: clear eeprom button

  wifi_setting();
  if(read_ap_ssid_pass())
    connect_to_wifi();
  
  while(wifimode)//waitting for connecting to AP ;
    server.handleClient();
  
  
  iottalk_register();
  five_min = millis();
}

void loop() {
  //Serial.printf("Stations connected = %d\n", WiFi.softAPgetStationNum());
  int httpCode;//http state code
  int i ;
  String get_ret_str;//After send GET request , store the return string
  int Brackets_index;// find the third '[' in get_ret_str 
  long clr_eeprom_timer=0;;
 
  if (digitalRead(13) == LOW)
    clr_eeprom();

  if(millis() - five_min >1000  ) {
    Serial.println("---------------------------------------------------");
    http.begin(url);
    http.addHeader("Content-Type","application/json");
    httpCode = http.GET();
    
    Serial.println("[HTTP] GET... code:" + (String)httpCode );
    get_ret_str = http.getString();
    Serial.println(get_ret_str);
    Brackets_index=0;
    for (i=0;i<3;i++)
      Brackets_index=get_ret_str.indexOf("[",Brackets_index+1);
    
    if (get_ret_str[Brackets_index+1] == '1')
      digitalWrite(2,HIGH);
    else if(get_ret_str[Brackets_index+1] == '0')
      digitalWrite(2,LOW);
    
    http.end();
    
    five_min =millis();
  }
  

}
