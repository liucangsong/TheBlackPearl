#define FASTLED_INTERNAL
#include <ESP8266WebServer.h>
#include <FS.h>
#include <FastLED.h>
#include <ArduinoJson.h>

#define DEBUG 1

#define COLOR_ORDER GRB //灯的排列顺序，灯带 GRB，灯珠 RGB
#define LED_COUNT 10 //灯泡数量
#define LED_DT 4    //D2 PIN 接线位置，Arduino 位置4对应D2 

CRGBArray<LED_COUNT> leds;

const char* ssid = "409wifi"; 
const char* password = "tarena_2019"; 

IPAddress Ip(192,168,13,200); 
IPAddress Gateway(192,168,13,1); 
IPAddress Subnet(255,255,255,0); 

uint8_t bright = 100; // 灯泡亮度 (0 - 255) 255 最大

ESP8266WebServer server(80);

uint8_t rainbowhue = 200;
boolean rainbow = true;

void setup() {
  Serial.begin(9600); 
  pinMode(2, OUTPUT); //设置Blink灯

  LEDS.setBrightness(bright); //设置亮度
  LEDS.addLeds<WS2812B, LED_DT, COLOR_ORDER>(leds, LED_COUNT);  // настройки для вашей ленты (ленты на WS2811, WS2812, WS2812B)
  leds.fill_solid(CRGB::Black);
  LEDS.show();

  WiFi.config(Ip, Gateway, Subnet);
  WiFi.begin(ssid, password);
  Serial.print("WIFI");

  while (WiFi.status() != WL_CONNECTED){ 
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
  
  server.begin();

  SPIFFS.begin();
  
}

int r = 0;
int g = 0;
int b = 0;

uint8_t room1 = 0;
uint8_t room2 = 1;

void loop() {
  // Blik 蓝灯，表示主循环在工作
  digitalWrite(2, ! digitalRead(2));
  server.handleClient();

  if(rainbow){
    rainbowhue++;
    if(rainbowhue==255){
      rainbowhue=0;
    }
    leds.fill_rainbow(rainbowhue);
  }

  delay(20);
  LEDS.show();
}

void handleOff(){
  CRGB color = CRGB::Black;
  //填充为黑色
  leds.fill_solid(color);
  //关闭彩虹效果
  rainbow = false;
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
  String json = "{\"success\":true}";
  server.send(200, "application/json", json);
}

void handleBright(){
  bright = server.arg("bright").toInt();
  LEDS.setBrightness(bright);
  String json = "{\"success\":true}";
  server.send(200, "application/json", json);
}

void handleRainbow(){
  uint8_t initialhue = (uint8_t)(server.arg("initialhue").toInt());
  rainbowhue = initialhue;
  rainbow = true;
  String json = "{\"success\":true}";
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
