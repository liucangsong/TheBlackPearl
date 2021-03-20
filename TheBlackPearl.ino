#define FASTLED_INTERNAL
#include <ESP8266WebServer.h>
#include <FS.h>
#include <FastLED.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <WiFiManager.h>  
#include <DNSServer.h>
#include <ESP8266WiFi.h>

#define DEBUG 1

#define COLOR_ORDER GRB //灯的排列顺序，灯带 GRB，灯珠 RGB
#define LED_COUNT 30 //灯泡数量
#define LED_DT D2    //D2 PIN 接线位置，Arduino 位置4对应D2 

CRGBArray<LED_COUNT> leds;

const char* ssid = "Robin+"; 
const char* password = "94xiaoshu"; 

IPAddress Ip(192,168,5,10); 
IPAddress Gateway(192,168,5,1); 
IPAddress Subnet(255,255,255,0); 

uint8_t bright = 100; // 灯泡亮度 (0 - 255) 255 最大

ESP8266WebServer server(80);

SoftwareSerial mySoftwareSerial(D7, D5); // RX-D7-13, TX-D5-14
DFRobotDFPlayerMini soundPlayer;
void printDetail(uint8_t type, int value);

uint8_t rainbowhue = 200;
boolean rainbow = true;

uint8_t seaSound = 5; //soundPlayer.loopFolder(seaSound);  大海音效再05文件中， 可以循环播放
uint8_t canonSound = 5; //ADVERT文件夹中 0005 文件，播放方式：  soundPlayer.advertise(canonSound); 
uint8_t cickSound = 3;  //ADVERT文件夹中 0003 文件，播放方式：  soundPlayer.advertise(cickSound); 

uint8_t resetTimeout = 6000;
uint8_t resetCount = 0;
#define WIFI_RESET D6 

void setup() {

  pinMode(D4, OUTPUT); //设置Blink灯
  pinMode(D1, OUTPUT); //设置Blink灯

  pinMode(WIFI_RESET, INPUT);

  initDfPlayer();
  initLights();
  //setupWifi();
  initWifiServer();

  SPIFFS.begin();
}

void loop() {
  // Blik 蓝灯，表示主循环在工作
  digitalWrite(D4, ! digitalRead(D4)); // 主板 Blink灯

  handleResetWifiButton();
   
  if (soundPlayer.available()) {
    printDetail(soundPlayer.readType(), soundPlayer.read());
  }

  server.handleClient();

  if(rainbow){
    rainbowhue++;
    if(rainbowhue==255){
      rainbowhue=0;
    }
    leds.fill_rainbow(rainbowhue);
  }
  LEDS.show();
  delay(20);
}

void handleResetWifiButton(){
  if (digitalRead(WIFI_RESET) == HIGH) {
    digitalWrite(D1, ! digitalRead(D1)); // 自己添加的 LED 灯
    resetCount+=20;
    if(resetCount > resetTimeout){
      resetCount = 0;
      setupWifi();
    }
  } else {
    resetCount = 0;
  }
}

void setupWifi(){
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  wifiManager.autoConnect("TheBlackPearlAP");
}

void debugPin(){
  Serial.println();
  Serial.print(F("D1:"));
  Serial.println(D1);
  Serial.print(F("D2:"));
  Serial.println(D2);
  Serial.print(F("D3:"));
  Serial.println(D3);
  Serial.print(F("D4:"));
  Serial.println(D4);
  Serial.print(F("D5:"));
  Serial.println(D5);
  Serial.print(F("D6:"));
  Serial.println(D6);
  Serial.print(F("D7:"));
  Serial.println(D7);
  Serial.print(F("D8:"));
  Serial.println(D8);
}

void initDfPlayer(){
  Serial.begin(9600); 
  mySoftwareSerial.begin(9600);

  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  while (!soundPlayer.begin(mySoftwareSerial)) {
    Serial.print(F("."));
    digitalWrite(D4, ! digitalRead(D4));
    soundPlayer.reset();
    delay(1000); // Code to compatible with ESP8266 watch dog.
  } 
  Serial.println(F("DFPlayer Mini online."));
   
  soundPlayer.volume(10);  //Set volume value. From 0 to 30
  soundPlayer.enableLoop();
  soundPlayer.loopFolder(seaSound);
}

void initLights(){
  LEDS.setBrightness(bright); //设置亮度
  LEDS.addLeds<WS2812B, LED_DT, COLOR_ORDER>(leds, LED_COUNT);  // настройки для вашей ленты (ленты на WS2811, WS2812, WS2812B)
  leds.fill_solid(CRGB::White);
  LEDS.show();
  delay(200);
  leds.fill_solid(CRGB::Black);
  LEDS.show();
}

void initWifiServer(){
  WiFi.config(Ip, Gateway, Subnet);
  WiFi.begin(ssid, password);
  Serial.print("WIFI");

  while (WiFi.status() != WL_CONNECTED){ 
    digitalWrite(D1, ! digitalRead(D1)); 
    delay(500);
    Serial.print(".");
  }
  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  //获取当前全部灯的状态
  server.on("/lights", handleLights);
  
  //处理一盏灯
  server.on("/light", handleLight);
  
  //处理一个房间的全部灯
  server.on("/room", handleRoom);
  
  //设置亮度
  server.on("/bright", handleBright);
  
  //设置彩虹效果
  server.on("/rainbow", handleRainbow);
  
  //设置色温
  server.on("/temperature", handleTemperature);
  
  //整体关闭
  server.on("/off", handleOff);

  //海浪声音开关
  server.on("/sound/play", handlePlay);

  //海浪声音开关
  server.on("/sound/pause", handlePause);
  
  //音量增加
  server.on("/volume/up", handleVolumeUp);

  //音量减少
  server.on("/volume/down", handleVolumeDown);

  //开炮
  server.on("/sound/open-fire", handleOpenFire);
  
  server.begin();

}

void handleOpenFire(){
  soundPlayer.advertise(canonSound); 
}

void handleVolumeUp(){
  soundPlayer.volumeUp();
}

void handleVolumeDown(){
  soundPlayer.volumeDown();
}

/**
 * 播放/关闭大海的音效
 */
void handlePlay(){
  soundPlayer.play();
}

void handlePause(){
  soundPlayer.pause();
}

void handleOff(){
  CRGB color = CRGB::Black;
  //填充为黑色
  leds.fill_solid(color);
  //关闭彩虹效果
  rainbow = false;

  soundPlayer.advertise(cickSound); 
  
  String json = "{\"success\":true}";
  server.send(200, "application/json", json);
}

void handleTemperature(){
  int k = server.arg("temperature").toInt();
  CRGB color;
  if(k==2600){
    color = CRGB(ColorTemperature::Candle);
  }
  if(k==6000){
    color = CRGB(ColorTemperature::DirectSunlight);
  }
  leds.fill_solid(color);
  rainbow = false;
  soundPlayer.advertise(cickSound); 
  String json = "{\"success\":true}";
  server.send(200, "application/json", json);
}

void handleBright(){
  bright = server.arg("bright").toInt();
  LEDS.setBrightness(bright);
  String json = "{\"success\":true}";
  soundPlayer.advertise(cickSound); 
  server.send(200, "application/json", json);
}

void handleRainbow(){
  uint8_t initialhue = (uint8_t)(server.arg("initialhue").toInt());
  rainbowhue = initialhue;
  rainbow = true;
  String json = "{\"success\":true}";
  soundPlayer.advertise(cickSound); 
  server.send(200, "application/json", json);
}

void handleLights(){
  //在栈中，创建JSON对象
  StaticJsonDocument<2048> doc;
  //转换为JSON数组
  JsonArray list = doc.to<JsonArray>();
  //将每个灯的状态添加到JSON中
  for(int i=0; i<leds.size(); i++){
    JsonObject light = list.createNestedObject();
    light["id"]=i;
    long r = leds[i].r;
    long g = leds[i].g;
    long b = leds[i].b;
    long rgb = (r<<16)|(g<<8)|b;
    String hex = String(rgb, HEX); 
    light["power"] = rgb==0 ? false : true;
    light["color"] = hex;
  }

  //soundPlayer.reset();
  //soundPlayer.advertise(cickSound); 
  
  //将JSON对象序列化为字符串
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleRoom() {
  boolean power = server.arg("power") == "true" ? true : false;
  String hex = server.arg("color");
  long rgb = (long) strtol( &hex[1], NULL, 16 );
  rainbow = false;

  String param=server.arg("lights");
  int index = 0;
  int startIndex = 0;
  int id;
  while((index = param.indexOf(',', startIndex)) !=-1){
    id = param.substring(startIndex, index).toInt();
    if(power){
      leds[id].setColorCode(rgb);
    }else{
      leds[id].setRGB(0,0,0);
    }
    startIndex = index+1;
  }
  id = param.substring(startIndex).toInt();
  if(power){
    leds[id].setColorCode(rgb);
  }else{
    leds[id].setRGB(0,0,0);
  }

  soundPlayer.advertise(cickSound); 

  StaticJsonDocument<200> doc; 
  doc["power"] = power;
  doc["success"] = true;
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}


void handleLight() {
  boolean power = server.arg("power") == "true" ? true : false;
  String hex = server.arg("color");
  long rgb = (long) strtol( &hex[1], NULL, 16 );
  rainbow = false;

  int id=server.arg("id").toInt();
  
  if(power){
    leds[id].setColorCode(rgb);
  }else{
    leds[id].setRGB(0,0,0);
  }

  soundPlayer.advertise(cickSound); 

  StaticJsonDocument<200> doc; // <- 200 bytes in the heap
  doc["power"] = power;
  doc["success"] = true;
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

bool handleFileRead(String path){
  #ifdef DEBUG
    Serial.println("handleFileRead: " + path);
  #endif
  if(path.endsWith("/")) path += "index.html";
  if(SPIFFS.exists(path)){
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, getContentType(path));
    file.close();
    return true;
  }
  return false;
  
}

String getContentType(String filename){
    if(server.hasArg("download")) return "application/octet-stream";
    else if(filename.endsWith(".htm")) return "text/html";
    else if(filename.endsWith(".html")) return "text/html";
    else if(filename.endsWith(".css")) return "text/css";
    else if(filename.endsWith(".js")) return "application/javascript";
    else if(filename.endsWith(".png")) return "image/png";
    else if(filename.endsWith(".gif")) return "image/gif";
    else if(filename.endsWith(".jpg")) return "image/jpeg";
    else if(filename.endsWith(".ico")) return "image/x-icon";
    else if(filename.endsWith(".xml")) return "text/xml";
    else if(filename.endsWith(".pdf")) return "application/x-pdf";
    else if(filename.endsWith(".zip")) return "application/x-zip";
    else if(filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";

}

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
 
}
