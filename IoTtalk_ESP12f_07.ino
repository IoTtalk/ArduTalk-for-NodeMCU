/*
 * IoTtalk V1 - ESP12F Version 7.0 
 */

#define nODF 10   // The total number of ODFs which the DA will pull.
 
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>
#include "ESP8266HTTPClient2.h"
#include <EEPROM.h>

char wifissid[100]="";
char wifipass[100]="";
char ServerIP[100]="";
String url = "";
//String url = "http://140.113.199.199:9999/";  
HTTPClient http;
uint8_t wifimode = 1; //1:AP , 0: STA 
//unsigned long five_min;

String softapname = "UV_Light";
IPAddress ip(192,168,0,1);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server ( 80 );



void clr_eeprom(int sw=0){
    if (!sw){
        Serial.println("Count down 3 seconds to clear EEPROM.");
        delay(3000);
    }
    if( (digitalRead(5) == LOW) || (sw == 1) ){
        for(int addr=0;addr<50;addr++) EEPROM.write(addr,0);   // clear eeprom
        EEPROM.commit();
        Serial.println("Clear EEPROM.");
        digitalWrite(2,HIGH);
    }
}

void store_ap_ssid_pass(void){
/* stoage format: [SSID,PASS,ServerIP]
   server.arg(0)  <--- that is SSID.
   server.arg(1)  <--- that is PASSWD.
   server.arg(2)  <--- that is server IP.        */
  int addr=0,i=0,j=0;
  EEPROM.write (addr++,'[');  // the code is equal to (EEPROM.write (addr,'[');  addr=addr+1;)
  for (j=0;j<3;j++){
    i=0;
    while(server.arg(j)[i] != '\0') EEPROM.write(addr++,server.arg(j)[i++]);
    if(j<2) EEPROM.write(addr++,',');
  }
  EEPROM.write (addr++,']');
  EEPROM.commit();
}

boolean read_ap_ssid_pass(void){
  // storage format: [SSID,PASS,ServerIP]
  String readdata="";
  char temp;
  int addr=0;
  
  temp = EEPROM.read(addr++);
  if(temp == '['){
    while(1){
      temp = EEPROM.read(addr++);
      if(temp == ',')   break;
      readdata += temp;
    }
    readdata.toCharArray(wifissid,100);
    readdata ="";
    
    while(1){
      temp = EEPROM.read(addr++);
      if(temp == ',') break;
      readdata += temp;
    }
    readdata.toCharArray(wifipass,100);
    readdata ="";    

    while(1){
      temp = EEPROM.read(addr++);
      if(temp == ']') break;
      readdata += temp;
    }
    readdata.toCharArray(ServerIP,100);

    if (String(ServerIP).length () < 7){
        Serial.println("Failed to reload setting.");
        //clr_eeprom(1);
        return false;
    }
    else{ 
        Serial.println("Reload setting successfully.");
        return true;
    }
  }
  else{
    Serial.println(temp);
    Serial.println("no data in eeprom");
    return false;
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
  
  if(AP_N>0) for (i=0;i<AP_N;i++) AP_List += "<option value=\""+WiFi.SSID(i)+"\">" + WiFi.SSID(i) + "</option>";
  else AP_List = "<option value=\"\">NO AP</option>";
  
  AP_List +="</select><br><br>";
  return(AP_List); 
}

void handleRoot() {
  String temp = "<html><title>Wi-Fi Setting</title>";
  temp += "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>";
  temp += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head>";
  temp += "<form action=\"setup\">";
  temp += "<div style=\"width:400px; height:150px;margin:0px auto;\"><div>";
  temp += "SSID:<br>";
  temp += scan_network();
  temp += "Password:<br>";
  temp += "<input type=\"password\" name=\"Password\" vplaceholder=\"輸入AP密碼\" style=\"width: 150px;\">";
  temp += "<br><br>IoTtalk Server IP<br>";  
  temp += "<input type=\"serverIP\" name=\"serverIP\" value=\"140.113.199.199\" style=\"width: 150px;\">";
  temp += "<br><br><input type=\"submit\" value=\"Submit\" on_click=\"javascript:alert('TEST');\">";
  temp += "</div></div></form></html>";
  server.send ( 200, "text/html", temp );
}

void handleNotFound() {
  Serial.println("Not Found Page");
}
void setup_wifi() {
    Serial.println("setup.");
    if (server.arg(0) != ""){//arg[0]-> SSID, arg[1]-> password (both string)
      server.arg(0).toCharArray(wifissid,100);
      server.arg(1).toCharArray(wifipass,100);
      server.arg(2).toCharArray(ServerIP,100);
      server.send( 200, "text/html", "ok");
      connect_to_wifi();
    }
}

void wifi_setting(void){
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  WiFi.softAPConfig(ip,gateway,subnet);
  WiFi.softAP(&softapname[0]);
  
  if ( MDNS.begin ( "esp8266" ) )     Serial.println ( "MDNS responder started" );

  server.on ( "/", handleRoot );
  server.on ( "/setup", setup_wifi );
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "AP started" );
}

void connect_to_wifi(void){
  long connecttimeout = millis();

  if (String(ServerIP).length() < 7){
      Serial.println("Server IP is wrong. Clean EEPROM.");
      clr_eeprom;
      wifi_setting();      
  }
  WiFi.softAPdisconnect(true);
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
    //clr_eeprom(1);
    wifi_setting();
  }
  
  //Serial.println(WiFi.localIP());
}

int iottalk_register(char *dm_name, char *df_list){
  url = "http://" + String(ServerIP) + ":9999/";  

  if(WiFi.status() !=WL_CONNECTED) connect_to_wifi();
  
  //Append the mac address to url string
  uint8_t MAC_array[6];
  WiFi.macAddress(MAC_array);//get esp12f mac address
  for (int i=0;i<6;i++){
    if( MAC_array[i]<0x10 ) url+="0";
    url+=String(MAC_array[i],HEX);
  }
  
  //send the register packet
  Serial.println("[HTTP] POST..." + url);

  String profile="{\"profile\": {\"d_name\": \"";
  profile += String(dm_name);
  profile += ".MAC";
  profile += "\", \"dm_name\": \"";
  profile += String(dm_name);
  profile += "\", \"is_sim\": false, \"df_list\": [";
  profile +=  String(df_list);
  profile += "]}}";

  http.begin(url);
  http.addHeader("Content-Type","application/json");

  int httpCode = http.POST(profile);
  Serial.println("[HTTP] Register... code: " + (String)httpCode );
  Serial.println(http.getString());
  http.end();
  url +="/";  
  return httpCode;
}

String df_name_list[nODF];
String df_timestamp[nODF];
void init_ODFtimestamp(){
  for (int i=0; i<=nODF; i++) df_timestamp[i] == "";
  for (int i=0; i<=nODF; i++) df_name_list[i] == "";  
}

int DFindex(char *df_name){
    for (int i=0; i<=nODF; i++){
        if (String(df_name) ==  df_name_list[i]) return i;
        else if (df_name_list[i] == ""){
            df_name_list[i] = String(df_name);
            return i;
        }
    }
    return nODF+1;  // df_timestamp is full
}

String pull(char *df_name){
    http.begin( url + String(df_name) );
    http.addHeader("Content-Type","application/json");
    int httpCode = http.GET(); //http state code
    if (httpCode != 200) Serial.println("[HTTP] PULL \"" + String(df_name) + "\"... code: " + (String)httpCode );
    String get_ret_str = http.getString();  //After send GET request , store the return string
    //Serial.println(get_ret_str);
    http.end();

    int string_index = 0;
    string_index = get_ret_str.indexOf("[",string_index);
    String portion = "";  //This portion is used to fetch the timestamp.
    if (get_ret_str[string_index+1] == '[' &&  get_ret_str[string_index+2] == '\"'){
        string_index += 3;
        while (get_ret_str[string_index] != '\"'){
          portion += get_ret_str[string_index];
          string_index+=1;
        }
        
        if (df_timestamp[DFindex(df_name)] != portion){
            df_timestamp[DFindex(df_name)] = portion;
            string_index = get_ret_str.indexOf("[",string_index);
            string_index += 1;
            portion = ""; //This portion is used to fetch the data.
            while (get_ret_str[string_index] != ']'){
                portion += get_ret_str[string_index];
                string_index+=1;
            }
            return portion;   // return the data.
         }
         else return "___NULL_DATA___";
    }
    else return "___NULL_DATA___";

    
}


void setup() {
  EEPROM.begin(512);
  Serial.begin(115200);
  pinMode(2, OUTPUT);//GPIO2 : on board led
  digitalWrite(2,HIGH);
  pinMode(5, INPUT_PULLUP);//GPIO5: clear eeprom button

  if(read_ap_ssid_pass())  connect_to_wifi();
  else {
    Serial.println("Relaod failed!");
    wifi_setting();
  }
  
  while(wifimode) server.handleClient(); //waitting for connecting to AP ;

  server.stop();
  init_ODFtimestamp();
  if (iottalk_register("ESP12F", "\"esp12f_LED\",\"GPIO6\"") == 200) digitalWrite(2,LOW);;

//  five_min = millis();
}

void loop() {

    String result;
    while(1){
        
        if (digitalRead(5) == LOW)  clr_eeprom();

        result = pull("esp12f_LED");
        if (result != "___NULL_DATA___"){
          Serial.println ("esp12f_LED Data: "+result);
          if (result == "1") digitalWrite(2,HIGH);
          else if(result == "0") digitalWrite(2,LOW);
        }

        result = pull("GPIO6");
        if (result != "___NULL_DATA___")  Serial.println ("GOIP6 Data: "+result);
        
        delay(3000);
    }
}
