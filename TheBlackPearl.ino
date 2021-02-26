#define FASTLED_INTERNAL
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <FS.h>
#include <FastLED.h>

#define DEBUG 1

#define COLOR_ORDER GRB //灯的排列顺序，灯带 GRB，灯珠 RGB
#define LED_COUNT 10 //灯泡数量
#define LED_DT 4    //D2 PIN 接线位置，Arduino 位置4对应D2 

CRGBArray<LED_COUNT> leds;

const char* ssid = "Robin+"; 
const char* password = "94xiaoshu"; 

IPAddress Ip(192,168,5,10); 
IPAddress Gateway(192,168,5,1); 
IPAddress Subnet(255,255,255,0); 

uint8_t bright = 50; // 灯泡亮度 (0 - 255) 255 最大
uint8_t ledMode = 0; // эффект (0 - 29)

uint8_t flag = 1; // флаг отмены эффекта

uint8_t delayValue = 20; // задержка
uint8_t stepValue = 10; // шаг по пикселям
uint8_t hueValue = 0; // тон цвета

uint8_t idex = 0; //индекс текущего пикселя
uint8_t ihue = 0; // тон цвета
uint8_t saturationVal = 255; // насыщенность
uint8_t ibright = 0; //значение яркости
uint16_t TOP_INDEX = uint8_t(LED_COUNT / 2); // получаем середину ленты
uint8_t EVENODD = LED_COUNT % 2; //флаг проверки четности
uint8_t bouncedirection = 0; //флаг для color_bounce()
int ledsX[LED_COUNT][3]; //массив для сохранения случайных значений пикселя

WebSocketsServer webSocket(81);
ESP8266WebServer server(80);

void setup() {
  Serial.begin(9600); 
  pinMode(2, OUTPUT); //设置Blink灯

  LEDS.setBrightness(bright); //设置亮度
  LEDS.addLeds<WS2812B, LED_DT, COLOR_ORDER>(leds, LED_COUNT);  // настройки для вашей ленты (ленты на WS2811, WS2812, WS2812B)
  updateColor(0, 0, 0);
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

  server.on("/room", handleRooms);
  
  server.begin();

  SPIFFS.begin();
  
  //webSocket.begin();
  //webSocket.onEvent(webSocketEvent);
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
  //leds[room1].setRGB(r++, g++, b++);
  //leds[room2].setRGB(r++, 255, 255);
  delay(20);
  //leds[2].setRGB(255,255,255);
  LEDS.show();
}

void handleRooms() {
  int roomId = server.arg("roomId").toInt();
  int light = server.arg("light").toInt();
  String hex = server.arg("color");
  long rgb = (long) strtol( &hex[1], NULL, 16 );
  if(light == 1){
    //leds[roomId].setRGB(255,255,255);
    leds[roomId].setColorCode(rgb);

    DynamicJsonDocument doc(200); // <- 200 bytes in the heap
    doc["success"] = true;
    doc["light"] = 1;
    String json;
    serializeJson(doc, json);
    
    
  }else{
    leds[roomId].setRGB(0,0,0);
    
    DynamicJsonDocument doc(200); // <- 200 bytes in the heap
    doc["success"] = true;
    doc["light"] = 0;
    String json;
    serializeJson(doc, json);
    
    server.send(200, "application/json", json);
  }
 
}

class Light{
  private:
  int index;    //当前灯的ID
  String name; //当前灯灯名字
  
  public:
  Light(int index, String name);
  void update(); //更新方法，每个定时周期都会执行的方法
  
};

Light::Light(int index_, String name_){
  index = index_;
  name = name_; 
}

void Light::update(){
  
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

void updateColor(uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = 0 ; i < LED_COUNT; i++ ) {
    leds[i].setRGB(r, g, b);
  }
}

//плавная смена цветов
void rainbow_fade() {
  ihue++;
  if (ihue > 255) {
    ihue = 0;
  }
  for (int idex = 0 ; idex < LED_COUNT; idex++ ) {
    leds[idex] = CHSV(ihue, saturationVal, 255);
  }
  LEDS.show();
  delay(delayValue);
}

// крутящаяся радуга
void rainbow_loop() {
  idex++;
  ihue = ihue + stepValue;
  if (idex >= LED_COUNT) {
    idex = 0;
  }
  if (ihue > 255) {
    ihue = 0;
  }
  leds[idex] = CHSV(ihue, saturationVal, 255);
  LEDS.show();
  delay(delayValue);
}

// случайная смена цветов
void random_burst() {
  idex = random(0, LED_COUNT);
  ihue = random(0, 255);
  leds[idex] = CHSV(ihue, saturationVal, 255);
  LEDS.show();
  delay(delayValue);
}

// бегающий пиксель
void color_bounce() {
  if (bouncedirection == 0) {
    idex = idex + 1;
    if (idex == LED_COUNT) {
      bouncedirection = 1;
      idex = idex - 1;
    }
  }
  if (bouncedirection == 1) {
    idex = idex - 1;
    if (idex == 0) {
      bouncedirection = 0;
    }
  }
  for (int i = 0; i < LED_COUNT; i++ ) {
    if (i == idex) {
      leds[i] = CHSV(hueValue, saturationVal, 255);
    }
    else {
      leds[i] = CHSV(0, 0, 0);
    }
  }
  LEDS.show();
  delay(delayValue);
}

//бегающий паровозик пикселей
void color_bounceFADE() {
  if (bouncedirection == 0) {
    idex = idex + 1;
    if (idex == LED_COUNT) {
      bouncedirection = 1;
      idex = idex - 1;
    }
  }
  if (bouncedirection == 1) {
    idex = idex - 1;
    if (idex == 0) {
      bouncedirection = 0;
    }
  }
  int iL1 = adjacent_cw(idex);
  int iL2 = adjacent_cw(iL1);
  int iL3 = adjacent_cw(iL2);
  int iR1 = adjacent_ccw(idex);
  int iR2 = adjacent_ccw(iR1);
  int iR3 = adjacent_ccw(iR2);

  for (int i = 0; i < LED_COUNT; i++ ) {
    if (i == idex) {
      leds[i] = CHSV(hueValue, saturationVal, 255);
    }
    else if (i == iL1) {
      leds[i] = CHSV(hueValue, saturationVal, 150);
    }
    else if (i == iL2) {
      leds[i] = CHSV(hueValue, saturationVal, 80);
    }
    else if (i == iL3) {
      leds[i] = CHSV(hueValue, saturationVal, 20);
    }
    else if (i == iR1) {
      leds[i] = CHSV(hueValue, saturationVal, 150);
    }
    else if (i == iR2) {
      leds[i] = CHSV(hueValue, saturationVal, 80);
    }
    else if (i == iR3) {
      leds[i] = CHSV(hueValue, saturationVal, 20);
    }
    else {
      leds[i] = CHSV(0, 0, 0);
    }
  }
  LEDS.show();
  delay(delayValue);
}

// вращение красного и синего
void red_blue_bounce() {
  idex++;
  if (idex >= LED_COUNT) {
    idex = 0;
  }
  int idexR = idex;
  int idexB = antipodal_index(idexR);
  int thathue = (hueValue + 160) % 255;

  for (int i = 0; i < LED_COUNT; i++ ) {
    if (i == idexR) {
      leds[i] = CHSV(hueValue, saturationVal, 255);
    }
    else if (i == idexB) {
      leds[i] = CHSV(thathue, saturationVal, 255);
    }
    else {
      leds[i] = CHSV(0, 0, 0);
    }
  }
  LEDS.show();
  delay(delayValue);
}

int antipodal_index(int i) {
  int iN = i + TOP_INDEX;
  if (i >= TOP_INDEX) {
    iN = ( i + TOP_INDEX ) % LED_COUNT;
  }
  return iN;
}

// вращение красного/синего
void rotatingRedBlue() {
  idex++;
  if (idex >= LED_COUNT) {
    idex = 0;
  }
  int idexR = idex;
  int idexB = antipodal_index(idexR);
  int thathue = (hueValue + 160) % 255;
  leds[idexR] = CHSV(hueValue, saturationVal, 255);
  leds[idexB] = CHSV(thathue, saturationVal, 255);
  LEDS.show();
  delay(delayValue);
}

// случайный стробоскоп
void flicker() {
  int random_bright = random(0, 255);
  int random_delay = random(10, 100);
  int random_bool = random(0, random_bright);
  if (random_bool < 10) {
    for (int i = 0 ; i < LED_COUNT; i++ ) {
      leds[i] = CHSV(160, 50, random_bright);
    }
    LEDS.show();
    delay(random_delay);
  }
}

// плавная смена яркости
void fade_vertical() {
  idex++;
  if (idex > TOP_INDEX) {
    idex = 0;
  }
  int idexA = idex;
  int idexB = horizontal_index(idexA);
  ibright = ibright + 10;
  if (ibright > 255) {
    ibright = 0;
  }
  leds[idexA] = CHSV(hueValue, saturationVal, ibright);
  leds[idexB] = CHSV(hueValue, saturationVal, ibright);
  LEDS.show();
  delay(delayValue);
}

//служебная функция
int horizontal_index(int i) {
  if (i == 0) {
    return 0;
  }
  if (i == TOP_INDEX && EVENODD == 1) {
    return TOP_INDEX + 1;
  }
  if (i == TOP_INDEX && EVENODD == 0) {
    return TOP_INDEX;
  }
  return LED_COUNT - i;
}

//служебная функция
void random_red() {
  int temprand;
  for (int i = 0; i < LED_COUNT; i++ ) {
    temprand = random(0, 100);
    if (temprand > 50) {
      leds[i].r = 255;
    }
    if (temprand <= 50) {
      leds[i].r = 0;
    }
    leds[i].b = 0; leds[i].g = 0;
  }
  LEDS.show();
}

//безумие красных светодиодов
void rule30() {
  if (bouncedirection == 0) {
    random_red();
    bouncedirection = 1;
  }
  copy_led_array();
  int iCW;
  int iCCW;
  int y = 100;
  for (int i = 0; i < LED_COUNT; i++ ) {
    iCW = adjacent_cw(i);
    iCCW = adjacent_ccw(i);
    if (ledsX[iCCW][0] > y && ledsX[i][0] > y && ledsX[iCW][0] > y) {
      leds[i].r = 0;
    }
    if (ledsX[iCCW][0] > y && ledsX[i][0] > y && ledsX[iCW][0] <= y) {
      leds[i].r = 0;
    }
    if (ledsX[iCCW][0] > y && ledsX[i][0] <= y && ledsX[iCW][0] > y) {
      leds[i].r = 0;
    }
    if (ledsX[iCCW][0] > y && ledsX[i][0] <= y && ledsX[iCW][0] <= y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] > y && ledsX[iCW][0] > y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] > y && ledsX[iCW][0] <= y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] <= y && ledsX[iCW][0] > y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] <= y && ledsX[iCW][0] <= y) {
      leds[i].r = 0;
    }
  }
  LEDS.show();
  delay(delayValue);
}
int adjacent_cw(int i) {
  int r;
  if (i < LED_COUNT - 1) {
    r = i + 1;
  } else {
    r = 0;
  }
  return r;
}
int adjacent_ccw(int i) {
  int r;
  if (i > 0) {
    r = i - 1;
  }
  else {
    r = LED_COUNT - 1;
  }
  return r;
}

// безумие случайных цветов
void random_march() {
  copy_led_array();
  int iCCW;
  leds[0] = CHSV(random(0, 255), 255, 255);
  for (int idex = 1; idex < LED_COUNT ; idex++ ) {
    iCCW = adjacent_ccw(idex);
    leds[idex].r = ledsX[iCCW][0];
    leds[idex].g = ledsX[iCCW][1];
    leds[idex].b = ledsX[iCCW][2];
  }
  LEDS.show();
  delay(delayValue);
}
void copy_led_array() {
  for (int i = 0; i < LED_COUNT; i++ ) {
    ledsX[i][0] = leds[i].r;
    ledsX[i][1] = leds[i].g;
    ledsX[i][2] = leds[i].b;
  }
}

//белый синий красный бегут по кругу
void rwb_march() {
  copy_led_array();
  int iCCW;
  idex++;
  if (idex > 2) {
    idex = 0;
  }
  switch (idex) {
    case 0:
      leds[0].r = 255;
      leds[0].g = 0;
      leds[0].b = 0;
      break;
    case 1:
      leds[0].r = 255;
      leds[0].g = 255;
      leds[0].b = 255;
      break;
    case 2:
      leds[0].r = 0;
      leds[0].g = 0;
      leds[0].b = 255;
      break;
  }
  for (int i = 1; i < LED_COUNT; i++ ) {
    iCCW = adjacent_ccw(i);
    leds[i].r = ledsX[iCCW][0];
    leds[i].g = ledsX[iCCW][1];
    leds[i].b = ledsX[iCCW][2];
  }
  LEDS.show();
  delay(delayValue);
}

// эффект пламени
void flame() {
  int idelay = random(0, 35);
  float hmin = 0.1; float hmax = 45.0;
  float hdif = hmax - hmin;
  int randtemp = random(0, 3);
  float hinc = (hdif / float(TOP_INDEX)) + randtemp;
  int ihue = hmin;
  for (int i = 0; i <= TOP_INDEX; i++ ) {
    ihue = ihue + hinc;
    leds[i] = CHSV(ihue, saturationVal, 255);
    int ih = horizontal_index(i);
    leds[ih] = CHSV(ihue, saturationVal, 255);
    leds[TOP_INDEX].r = 255; leds[TOP_INDEX].g = 255; leds[TOP_INDEX].b = 255;
    LEDS.show();
    delay(idelay);
  }
}

// радуга
void rainbow_vertical() {
  idex++;
  if (idex > TOP_INDEX) {
    idex = 0;
  }
  ihue = ihue + stepValue;
  if (ihue > 255) {
    ihue = 0;
  }
  int idexA = idex;
  int idexB = horizontal_index(idexA);
  leds[idexA] = CHSV(ihue, saturationVal, 255);
  leds[idexB] = CHSV(ihue, saturationVal, 255);
  LEDS.show();
  delay(delayValue);
}

// безумие случайных вспышек
void random_color_pop() {
  idex = random(0, LED_COUNT);
  ihue = random(0, 255);
  updateColor(0, 0, 0);
  leds[idex] = CHSV(ihue, saturationVal, 255);
  LEDS.show();
  delay(delayValue);
}

// полицейская мигалка
void policeBlinker() {
  int hueValue = 0;
  int thathue = (hueValue + 160) % 255;
  for (int x = 0 ; x < 5; x++ ) {
    for (int i = 0 ; i < TOP_INDEX; i++ ) {
      leds[i] = CHSV(hueValue, saturationVal, 255);
    }
    LEDS.show(); delay(delayValue);
    updateColor(0, 0, 0);
    LEDS.show(); delay(delayValue);
  }
  for (int x = 0 ; x < 5; x++ ) {
    for (int i = TOP_INDEX ; i < LED_COUNT; i++ ) {
      leds[i] = CHSV(thathue, saturationVal, 255);
    }
    LEDS.show(); delay(delayValue);
    updateColor(0, 0, 0);
    LEDS.show(); delay(delayValue);
  }
}

void rgb_propeller() { // пропеллер
  idex++;
  int ghue = (hueValue + 80) % 255;
  int bhue = (hueValue + 160) % 255;
  int N3  = int(LED_COUNT / 3);
  int N6  = int(LED_COUNT / 6);
  int N12 = int(LED_COUNT / 12);

  for (int i = 0; i < N3; i++ ) {
    int j0 = (idex + i + LED_COUNT - N12) % LED_COUNT;
    int j1 = (j0 + N3) % LED_COUNT;
    int j2 = (j1 + N3) % LED_COUNT;
    leds[j0] = CHSV(hueValue, saturationVal, 255);
    leds[j1] = CHSV(ghue, saturationVal, 255);
    leds[j2] = CHSV(bhue, saturationVal, 255);
  }
  LEDS.show();
  delay(delayValue);
}

//случайные вспышки красного
void kitt() {
  int rand = random(0, TOP_INDEX);
  for (int i = 0; i < rand; i++ ) {
    leds[TOP_INDEX + i] = CHSV(hueValue, saturationVal, 255);
    leds[TOP_INDEX - i] = CHSV(hueValue, saturationVal, 255);
    LEDS.show();
    delay(delayValue / rand);
  }
  for (int i = rand; i > 0; i-- ) {
    leds[TOP_INDEX + i] = CHSV(hueValue, saturationVal, 0);
    leds[TOP_INDEX - i] = CHSV(hueValue, saturationVal, 0);
    LEDS.show();
    delay(delayValue / rand);
  }
}

// зеленые бегают по кругу случайно
void matrix() {
  int rand = random(0, 100);
  if (rand > 90) {
    leds[0] = CHSV(hueValue, saturationVal, 255);
  }
  else {
    leds[0] = CHSV(hueValue, saturationVal, 0);
  }
  copy_led_array();
  for (int i = 1; i < LED_COUNT; i++ ) {
    leds[i].r = ledsX[i - 1][0];
    leds[i].g = ledsX[i - 1][1];
    leds[i].b = ledsX[i - 1][2];
  }
  LEDS.show();
  delay(delayValue);
}

// плавная вращающаяся радуга
void new_rainbow_loop() {
  ihue -= 1;
  fill_rainbow( leds, LED_COUNT, ihue );
  LEDS.show();
  delay(delayValue);
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
  leds[Pixel].r = red;
  leds[Pixel].g = green;
  leds[Pixel].b = blue;
}
//служебная функция
void setAll(byte red, byte green, byte blue) {
  for (int i = 0; i < LED_COUNT; i++ ) {
    setPixel(i, red, green, blue);
  }
  FastLED.show();
}

//линейный огонь
void Fire(int Cooling, int Sparking, int SpeedDelay) {
  static byte heat[LED_COUNT];
  int cooldown;

  for (int i = 0; i < LED_COUNT; i++) {
    cooldown = random(0, ((Cooling * 10) / LED_COUNT) + 2);

    if (cooldown > heat[i]) {
      heat[i] = 0;
    } else {
      heat[i] = heat[i] - cooldown;
    }
  }

  for ( int k = LED_COUNT - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  if ( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160, 255);
  }

  for ( int j = 0; j < LED_COUNT; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  FastLED.show();
  delay(SpeedDelay);
}

void setPixelHeatColor(int Pixel, byte temperature) {
  byte t192 = round((temperature / 255.0) * 191);
  byte heatramp = t192 & 0x3F;
  heatramp <<= 2;

  if ( t192 > 0x80) {
    setPixel(Pixel, 255, 255, heatramp);
  } else if ( t192 > 0x40 ) {
    setPixel(Pixel, 255, heatramp, 0);
  } else {
    setPixel(Pixel, heatramp, 0, 0);
  }
}

void CenterToOutside(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for (int i = ((LED_COUNT - EyeSize) / 2); i >= 0; i--) {
    setAll(0, 0, 0);

    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);

    setPixel(LED_COUNT - i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(LED_COUNT - i - j, red, green, blue);
    }
    setPixel(LED_COUNT - i - EyeSize - 1, red / 10, green / 10, blue / 10);

    FastLED.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void OutsideToCenter(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for (int i = 0; i <= ((LED_COUNT - EyeSize) / 2); i++) {
    setAll(0, 0, 0);

    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);

    setPixel(LED_COUNT - i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(LED_COUNT - i - j, red, green, blue);
    }
    setPixel(LED_COUNT - i - EyeSize - 1, red / 10, green / 10, blue / 10);

    FastLED.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void LeftToRight(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for (int i = 0; i < LED_COUNT - EyeSize - 2; i++) {
    setAll(0, 0, 0);
    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);
    FastLED.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

// случайные вспышки белого цвета
void Sparkle(byte red, byte green, byte blue, int SpeedDelay) {
  int Pixel = random(LED_COUNT);
  setPixel(Pixel, red, green, blue);
  FastLED.show();
  delay(SpeedDelay);
  setPixel(Pixel, 0, 0, 0);
}

// бегущие каждые 3 пикселя
void theaterChase(byte red, byte green, byte blue, int SpeedDelay) {
  for (int j = 0; j < 10; j++) {
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < LED_COUNT; i = i + 3) {
        setPixel(i + q, red, green, blue);
      }
      FastLED.show();
      delay(SpeedDelay);
      for (int i = 0; i < LED_COUNT; i = i + 3) {
        setPixel(i + q, 0, 0, 0);
      }
    }
  }
}

// стробоскопический эффект
void Strobe(byte red, byte green, byte blue, int StrobeCount, int FlashDelay, int EndPause) {
  for (int j = 0; j < StrobeCount; j++) {
    setAll(red, green, blue);
    FastLED.show();
    delay(FlashDelay);
    setAll(0, 0, 0);
    FastLED.show();
    delay(FlashDelay);
  }

  delay(EndPause);
}

//голубой линейный огонь
void blueFire(int Cooling, int Sparking, int SpeedDelay) {
  static byte heat[LED_COUNT];
  int cooldown;

  for (int i = 0; i < LED_COUNT; i++) {
    cooldown = random(0, ((Cooling * 10) / LED_COUNT) + 2);

    if (cooldown > heat[i]) {
      heat[i] = 0;
    } else {
      heat[i] = heat[i] - cooldown;
    }
  }
  for ( int k = LED_COUNT - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
  if ( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160, 255);
  }
  for ( int j = 0; j < LED_COUNT; j++) {
    setPixelHeatColorBlue(j, heat[j] );
  }
  FastLED.show();
  delay(SpeedDelay);
}

void setPixelHeatColorBlue(int Pixel, byte temperature) {
  byte t192 = round((temperature / 255.0) * 191);
  byte heatramp = t192 & 0x03;
  heatramp <<= 2;

  if ( t192 > 0x03) {
    setPixel(Pixel, heatramp, 255, 255);
  } else if ( t192 > 0x40 ) {
    setPixel(Pixel, 255, heatramp, 0);
  } else {
    setPixel(Pixel, 0, 0, heatramp);
  }
}

//расплывающие цвета
void fadeToCenter() {
  static uint8_t hue;
  for (int i = 0; i < LED_COUNT / 2; i++) {
    leds.fadeToBlackBy(40);
    leds[i] = CHSV(hue++, 255, 255);
    leds(LED_COUNT / 2, LED_COUNT - 1) = leds(LED_COUNT / 2 - 1 , 0);
    FastLED.delay(33);
  }
}

//бегающий по кругу паровозик
void runnerChameleon() {
  static uint8_t hue = 0;
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i] = CHSV(hue++, 255, 255);
    FastLED.show();
    leds[i] = CRGB::Black;
    //    fadeall();
    delay(10);
  }
}


void blende() {
  static uint8_t hue = 0;
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i] = CHSV(hue++, 255, 255);
    FastLED.show();
    //     leds[i] = CRGB::Black;
    fadeall();
    delay(10);
  }

  for (int i = (LED_COUNT) - 1; i >= 0; i--) {
    leds[i] = CHSV(hue++, 255, 255);
    FastLED.show();
    //     leds[i] = CRGB::Black;
    fadeall();
    delay(10);
  }
}

void blende_2() {
  static uint8_t hue = 0;
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i] = CHSV(hue++, 255, 255);
    FastLED.show();
    leds[i] = CRGB::Black;
    fadeall();
    delay(10);
  }

  for (int i = (LED_COUNT) - 1; i >= 0; i--) {
    leds[i] = CHSV(hue++, 255, 255);
    FastLED.show();
    //     leds[i] = CRGB::Black;
    fadeall();
    delay(10);
  }
}
//служебная функция
void fadeall() {
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i].nscale8(250);
  }
}
