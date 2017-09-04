//   IoTtalk V1 - ESP12F Version 8.0  
#define DM_name  "ESP12F" 
#define DF_list  {"esp12f_LED", "GPIO16", "Sensor"}
#define nODF     10  // The max number of ODFs which the DA can pull.

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>
#include "ESP8266HTTPClient2.h"
#include <EEPROM.h>
//#include <ESP8266mDNS.h>   //enable Multicast DNS to provide Bonjour service.
char IoTtalkServerIP[100] = "";

void clr_eeprom(int sw=0){
    if (!sw){
        Serial.println("Count down 3 seconds to clear EEPROM.");
        delay(3000);
    }
    if( (digitalRead(5) == LOW) || (sw == 1) ){
        for(int addr=0; addr<50; addr++) EEPROM.write(addr,0);   // clear eeprom
        EEPROM.commit();
        Serial.println("Clear EEPROM.");
        digitalWrite(2,HIGH);
    }
}

void save_netInfo(char *wifiSSID, char *wifiPASS, char *ServerIP){  //stoage format: [SSID,PASS,ServerIP]
    char *netInfo[3] = {wifiSSID, wifiPASS, ServerIP};
    int addr=0,i=0,j=0;
    EEPROM.write (addr++,'[');  // the code is equal to (EEPROM.write (addr,'[');  addr=addr+1;)
    for (j=0;j<3;j++){
        i=0;
        while(netInfo[j][i] != '\0') EEPROM.write(addr++,netInfo[j][i++]);
        if(j<2) EEPROM.write(addr++,',');
    }
    EEPROM.write (addr++,']');
    EEPROM.commit();
}

int read_netInfo(char *wifiSSID, char *wifiPASS, char *ServerIP){   // storage format: [SSID,PASS,ServerIP]
    char *netInfo[3] = {wifiSSID, wifiPASS, ServerIP};
    String readdata="";
    int addr=0;
  
    char temp = EEPROM.read(addr++);
    if(temp == '['){
        for (int i=0; i<3; i++){
            readdata ="";
            while(1){
                temp = EEPROM.read(addr++);
                if (temp == ',' || temp == ']') break;
                readdata += temp;
            }
            readdata.toCharArray(netInfo[i],100);
        }
 
        if (String(ServerIP).length () < 7){
            Serial.println("ServerIP loading failed.");
            return 2;
        }
        else{ 
            Serial.println("Load setting successfully.");
            return 0;
        }
    }
    else{
        Serial.println("no data in eeprom");
        return 1;
    }
}


String scan_network(void){
    int AP_N,i;  //AP_N: AP number 
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

ESP8266WebServer server ( 80 );
void handleRoot(){
  String temp = "<html><title>Wi-Fi Setting</title>";
  temp += "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>";
  temp += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head>";
  temp += "<form action=\"setup\"><div>";
  temp += "SSID:<br>";
  temp += scan_network();
  temp += "Password:<br>";
  temp += "<input type=\"password\" name=\"Password\" vplaceholder=\"輸入AP密碼\" style=\"width: 150px;\">";
  temp += "<br><br>IoTtalk Server IP<br>";  
  temp += "<input type=\"serverIP\" name=\"serverIP\" value=\"140.113.199.199\" style=\"width: 150px;\">";
  temp += "<br><br><input type=\"submit\" value=\"Submit\" on_click=\"javascript:alert('TEST');\">";
  temp += "</div></form><br>";
  temp += "<div><input type=\"button\" value=\"開啟\" onclick=\"location.href=\'turn_on_pin\'\"><br>";
  temp += "<input type=\"button\" value=\"關閉\" onclick=\"location.href=\'turn_off_pin\'\"><br></div>";
  temp += "</html>";
  server.send ( 200, "text/html", temp );
}

void handleNotFound() {
  Serial.println("Page Not Found ");
  server.send( 404, "text/html", "Page not found.");
}

void saveInfoAndConnectToWiFi() {
    Serial.println("Get network information.");
    char _SSID_[100]="";
    char _PASS_[100]="";
    
    if (server.arg(0) != ""){//arg[0]-> SSID, arg[1]-> password (both string)
      server.arg(0).toCharArray(_SSID_,100);
      server.arg(1).toCharArray(_PASS_,100);
      server.arg(2).toCharArray(IoTtalkServerIP,100);
      server.send(200, "text/html", "ok");
      server.stop();
      save_netInfo(_SSID_, _PASS_, IoTtalkServerIP);
      connect_to_wifi(_SSID_, _PASS_);      
    }
}

void turn_off_pin(void){
   digitalWrite(2,LOW);
   server.send (204);
}

void turn_on_pin(void){
    digitalWrite(2,HIGH);  
    server.send (204);
}

void start_web_server(void){
    server.on ( "/", handleRoot );
    server.on ( "/setup", saveInfoAndConnectToWiFi);
    server.on ( "/turn_off_pin", turn_off_pin);
    server.on ( "/turn_on_pin", turn_on_pin);
    server.onNotFound ( handleNotFound );
    server.begin();  
}

void wifi_setting(void){
    String softapname = "UV_Light";
    IPAddress ip(192,168,0,1);
    IPAddress gateway(192,168,0,1);
    IPAddress subnet(255,255,255,0);  
    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect();
    WiFi.softAPConfig(ip,gateway,subnet);
    WiFi.softAP(&softapname[0]);
    //if ( MDNS.begin ( "esp8266" ) ) Serial.println ( "MDNS responder started" ); //enable Multicast DNS to provide Bonjour service.
    start_web_server();
    Serial.println ( "Switch to AP mode and start web server." );
}

uint8_t wifimode = 1; //1:AP , 0: STA 
void connect_to_wifi(char *wifiSSID, char *wifiPASS){
  long connecttimeout = millis();
  
  WiFi.softAPdisconnect(true);
  Serial.println("-----Connect to Wi-Fi-----");
  WiFi.begin(wifiSSID, wifiPASS);
  
  while (WiFi.status() != WL_CONNECTED && (millis() - connecttimeout < 10000) ) {
      delay(1000);
      Serial.print(".");
  }
  
  if(WiFi.status() == WL_CONNECTED){
    Serial.println ( "Connected!\n");
    digitalWrite(2,LOW);
    wifimode = 0;
  }
  else if (millis() - connecttimeout > 10000){
    Serial.println("Connect fail");
    wifi_setting();
  }
}


HTTPClient http;
String url = "";
int iottalk_register(void){
    url = "http://" + String(IoTtalkServerIP) + ":9999/";  
    
    String df_list[] = DF_list;
    int n_of_DF = sizeof(df_list)/sizeof(df_list[0]); // the number of DFs in the DF_list
    String DFlist = ""; 
    for (int i=0; i<n_of_DF; i++){
        DFlist += "\"" + df_list[i] + "\"";  
        if (i<n_of_DF-1) DFlist += ",";
    }
  
    uint8_t MAC_array[6];
    WiFi.macAddress(MAC_array);//get esp12f mac address
    for (int i=0;i<6;i++){
        if( MAC_array[i]<0x10 ) url+="0";
        url+= String(MAC_array[i],HEX);;      //Append the mac address to url string
    }
 
    //send the register packet
    Serial.println("[HTTP] POST..." + url);
    String profile="{\"profile\": {\"d_name\": \"";
    profile += DM_name;
    profile += "." + String(MAC_array[4],HEX) + String(MAC_array[5],HEX);
    profile += "\", \"dm_name\": \"";
    profile += DM_name;
    profile += "\", \"is_sim\": false, \"df_list\": [";
    profile +=  DFlist;
    profile += "]}}";

    http.begin(url);
    http.addHeader("Content-Type","application/json");
    int httpCode = http.POST(profile);

    Serial.println("[HTTP] Register... code: " + (String)httpCode );
    Serial.println(http.getString());
    //http.end();
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

int push(char *df_name, String value){
    http.begin( url + String(df_name));
    http.addHeader("Content-Type","application/json");
    String data = "{\"data\":[" + value + "]}";
    int httpCode = http.PUT(data);
    if (httpCode != 200) Serial.println("[HTTP] PUSH \"" + String(df_name) + "\"... code: " + (String)httpCode + ", retry to register.");
    while (httpCode != 200){
        digitalWrite(2, HIGH);
        httpCode = iottalk_register();
        if (httpCode == 200)  http.PUT(data);
        else delay(3000);
    }    
    return httpCode;
}

String pull(char *df_name){
    http.begin( url + String(df_name) );
    http.addHeader("Content-Type","application/json");
    int httpCode = http.GET(); //http state code
    if (httpCode != 200) Serial.println("[HTTP] PULL \"" + String(df_name) + "\"... code: " + (String)httpCode + ", retry to register.");
    while (httpCode != 200){
        digitalWrite(2, HIGH);
        httpCode = iottalk_register();
        if (httpCode == 200) http.GET();
        else delay(3000);
    }
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
    pinMode(2, OUTPUT);//GPIO2 : on board led
    digitalWrite(2,HIGH);
    pinMode(5, INPUT_PULLUP);//GPIO5: clear eeprom button
    pinMode(16, OUTPUT);//GPIO16 : relay signal
    digitalWrite(16,LOW);

    EEPROM.begin(512);
    Serial.begin(115200);

    char wifissid[100]="";
    char wifipass[100]="";
    int statesCode = read_netInfo(wifissid, wifipass, IoTtalkServerIP);
    //for (int k=0; k<50; k++) Serial.printf("%c", EEPROM.read(k) );  //inspect EEPROM data for the debug purpose.
  
    if (!statesCode)  connect_to_wifi(wifissid, wifipass);
    else{
        Serial.println("Laod setting failed! statesCode: " + String(statesCode)); // StatesCode 1=No data, 2=ServerIP with wrong format
        wifi_setting();
    }
    while(wifimode) server.handleClient(); //waitting for connecting to AP ;

    statesCode = 0;
    while (statesCode != 200) {
        statesCode = iottalk_register();
        if (statesCode != 200){
            Serial.println("Retry to register to the IoTtalk server. Suspend 3 seconds.");
            if (digitalRead(5) == LOW) clr_eeprom();
            delay(3000);
        }
    }
    init_ODFtimestamp();
}

long cycleTimestamp = millis();
int LED_flag=0;
String result;
void loop() {
    if (digitalRead(5) == LOW){
        clr_eeprom();
        LED_flag = 3;
    }
 
    if (millis() - cycleTimestamp > 500) {
        result = pull("esp12f_LED");
        if (result != "___NULL_DATA___"){
          Serial.println ("esp12f_LED Data: "+result);
          if (result.toInt() > 0) digitalWrite(2,HIGH);
          else if(result.toInt() == 0) digitalWrite(2,LOW);
        }
        
        result = pull("GPIO16");
        if (result != "___NULL_DATA___"){
          Serial.println ("GOIP16 Data: "+result);
          if (result.toInt() > 0) digitalWrite(16,HIGH);
          else if(result.toInt() == 0) digitalWrite(16,LOW);          
        }

        push("Sensor", String(millis()));        

        LED_flag = LED_flag^1;
        digitalWrite(2, LED_flag);    
        cycleTimestamp = millis();
    }

}
