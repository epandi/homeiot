#include <Arduino.h>
#include <PubSubClient.h>
#include <iostream>
#include "ArduinoJson.h"
#include "Nextion.h"
#include <CircularBuffer.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

//#include "BluetoothSerial.h"
#define dbSerial Serial

// Change the credentials below, so your ESP8266 connects to your router and MQTT Server
const char* ssid = "";
const char* password = "";
const char* mqttServer = "192.168.0.222";
const int mqttPort = 1883;
const char* mqttDeviceName = "Nextion-01";
const char* mqttUser = "";
const char* mqttPassword = "";
//Static IP address configuration
IPAddress ip_address(192, 168, 1, 51);  //ESP static ip
IPAddress ip_gateway(192, 168, 0, 1);   //IP Address of your WiFi Router (Gateway)
IPAddress ip_subnet(255, 255, 252, 0);  //Subnet mask
IPAddress ip_dns(192, 168, 0, 222);     //DNS

const short int BUILTIN_LED = 2; //GPIO2, MQTT Connection State

int CurrentPage = 0;  // Create a variable to store which page is currently loaded
CircularBuffer<uint8_t,150> WasmachineBuffer; 
char buff[10];

// Initialize the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient;
PubSubClient MQTTClient(espClient);

// Declare a button object [page id:0,component id:1, component name: "b0"]
NexPage page0 = NexPage(0, 0, "page0");               // Page0 Blinds
NexPage page1 = NexPage(1, 0, "page1");               // Page1 Switches
NexPage page2 = NexPage(2, 0, "page2");               // Page2 Energy
NexPage page3 = NexPage(3, 0, "page3");               // Page3
NexPage page4 = NexPage(4, 0, "page4");               // Page4
NexPage page5 = NexPage(5, 0, "page5");               // Page5 About
NexPage page6 = NexPage(6, 0, "page6");               // Page5 Debugging

NexButton p0b0 = NexButton(0, 1, "b0", &page0);       // Fleurblinds open
NexButton p0b1 = NexButton(0, 2, "b1", &page0);       // Fleurblinds close
NexNumber p0n0 = NexNumber(0, 7, "n0", &page0);       // Fleurblinds percentage
NexSlider p0h0 = NexSlider(0, 5, "h0", &page0);       // Fleurblinds slider
NexButton p0b2 = NexButton(0, 3, "b2", &page0);       // Hobbyblinds open
NexButton p0b3 = NexButton(0, 4, "b3", &page0);       // Hobbyblinds close
NexNumber p0n1 = NexNumber(0, 8, "n1", &page0);       // Hobbyblinds percentage
NexSlider p0h1 = NexSlider(0, 6, "h1", &page0);       // Hobbyblinds slider

NexDSButton p1bt0 = NexDSButton(1, 7, "bt0", &page1); // Buffetkast button
NexDSButton p1bt1 = NexDSButton(1, 8, "bt1", &page1); // Whiskeykast button
NexDSButton p1bt2 = NexDSButton(1, 9, "bt2", &page1); // Vensterbankvoor button
NexDSButton p1bt3 = NexDSButton(1, 10, "bt3", &page1);// Barkeuken button

NexNumber p2n0 = NexNumber(2, 7, "n0", &page2);       // Wasmachine voltage
NexNumber p2n1 = NexNumber(2, 8, "n1", &page2);       // Wasmachine wattage
NexText p2t0 = NexText(2, 10, "t0", &page2);          // Wasmachine kwh today
NexWaveform p2s0 = NexWaveform(2, 9, "s0", &page2);   // Wasmachine waveform

NexText p5t5 = NexText(5, 13, "t5", &page5);          // IPAddress

NexText p6t0 = NexText(6, 4, "t0", &page6);           // Debugging
NexText p6t1 = NexText(6, 2, "t1", &page6);           // MQTT Connected textbox
NexText p6t2 = NexText(6, 3, "t2", &page6);           // fleurblinds
NexText p6t3 = NexText(6, 5, "t3", &page6);           // whiskeykast
NexText p6t4 = NexText(6, 6, "t4", &page6);           // wasmachine
NexText p6t5 = NexText(6, 7, "t5", &page6);           // vensterbankvoor
NexText p6t6 = NexText(6, 8, "t6", &page6);           // Hobbyblinds
NexText p6t7 = NexText(6, 9, "t7", &page6);           // fleurblinds
NexText p6t8 = NexText(6, 10, "t8", &page6);          // fleurcam
NexText p6t9 = NexText(6, 11, "t9", &page6);          // buffetkast
NexText p6t10 = NexText(6, 12, "t10", &page6);        // barkeuken

NexRtc  rtc;

//Register a button object to the touch event list  
NexTouch *nex_listen_list[] = {
  &p0b0,
  &p0b1,
  &p0h0,
  &p0b2,
  &p0b3,
  &p0h1,
  &p1bt0,
  &p1bt1,
  &p1bt2,
  &p1bt3,
  &p6t0,
  &page0,
  &page1,
  &page2,
  &page3,
  &page4,
  &page5,
  &page6,
  NULL
};

// FLEUR BLINDS
void p0b0PopCallback(void *ptr) {  
  // Fleurblinds open
  MQTTClient.publish("cmnd/fleurblinds/SHUTTEROPEN", "");
}
void p0b1PopCallback(void *ptr) {
  // Fleurblinds close
  MQTTClient.publish("cmnd/fleurblinds/SHUTTERCLOSE", ""); 
}
void p0h0PopCallback(void *ptr) {
  // Fleurblinds slider
  uint32_t number = 0;
  p0h0.getValue(&number);
  memset(buff, 0, sizeof(buff));
  itoa(number, buff, 10);
  
  MQTTClient.publish("cmnd/fleurblinds/SHUTTERPOSITION", buff);
}

// HOBBY BLINDS
void p0b2PopCallback(void *ptr) {  
  // Hobbyblinds open
  MQTTClient.publish("cmnd/hobbyblinds/SHUTTEROPEN", "");
}
void p0b3PopCallback(void *ptr) {
  // Hobbyblinds close
  MQTTClient.publish("cmnd/hobbyblinds/SHUTTERCLOSE", ""); 
}
void p0h1PopCallback(void *ptr) {
  // Hobbyblinds slider
  uint32_t number = 0;
  p0h1.getValue(&number);
  memset(buff, 0, sizeof(buff));
  itoa(number, buff, 10);
  
  MQTTClient.publish("cmnd/hobbyblinds/SHUTTERPOSITION", buff);
}

// TOGGLE BUTTONS
void p1bt0PushCallback(void *ptr)
{
  // Buffetkast
  uint32_t dual_state;
  p1bt0.getValue(&dual_state);
  if(dual_state) { MQTTClient.publish("cmnd/buffetkast/POWER1", "ON"); } else { MQTTClient.publish("cmnd/buffetkast/POWER1", "OFF"); }
}
void p1bt1PushCallback(void *ptr)
{
  // Whiskeykast
  uint32_t dual_state;
  p1bt1.getValue(&dual_state);
  if(dual_state) { MQTTClient.publish("cmnd/whiskeykast/POWER1", "ON"); } else { MQTTClient.publish("cmnd/whiskeykast/POWER1", "OFF"); }
}
void p1bt2PushCallback(void *ptr)
{
  // Vensterbankvoor
  uint32_t dual_state;
  p1bt2.getValue(&dual_state);
  if(dual_state) { MQTTClient.publish("cmnd/vensterbankvoor/POWER1", "ON"); } else { MQTTClient.publish("cmnd/vensterbankvoor/POWER1", "OFF"); }
}
void p1bt3PushCallback(void *ptr)
{
  // Barkeuken
  uint32_t dual_state;
  p1bt3.getValue(&dual_state);
  if(dual_state) { MQTTClient.publish("cmnd/barkeuken/POWER1", "ON"); } else { MQTTClient.publish("cmnd/barkeuken/POWER1", "OFF"); }
}
void p6t0PushCallback(void *ptr)
{
  uint32_t bgc;
  p6t0.Get_background_color_bco(&bgc);
  if (bgc == 65535) { // White background color
    // Enable Debugging
    dbSerial.begin(115200);
    ArduinoOTA.begin();
    p6t0.Set_background_color_bco (28651);
  } else if (bgc == 28651) { // Green background color
    // Disable Debugging
    dbSerial.end();
    ArduinoOTA.end();
    p6t0.Set_background_color_bco (65535);
  }
}
void page0PushCallback(void *ptr) { CurrentPage = 0; }
void page1PushCallback(void *ptr) { CurrentPage = 1; }
void page2PushCallback(void *ptr) { CurrentPage = 2; for (byte i = 0; i < WasmachineBuffer.size() - 1; i++) { p2s0.addValue(0, WasmachineBuffer[i]); } }
void page3PushCallback(void *ptr) { CurrentPage = 3; }
void page4PushCallback(void *ptr) { CurrentPage = 4; }
void page5PushCallback(void *ptr) { CurrentPage = 5; } 
void page6PushCallback(void *ptr) { CurrentPage = 6; } 


// This function is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  dbSerial.print("Message arrived on topic: ");
  dbSerial.print(topic);
  dbSerial.print(". Message: ");
  String messageTemp;
  
  for (unsigned int i = 0; i < length; i++) {
    dbSerial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  dbSerial.println();

  StaticJsonBuffer<1024> jsonBuffer;
  JsonObject& mqttjson = jsonBuffer.parseObject(messageTemp);
  
  if(((topic=="tele/fleurblinds/SENSOR") && (mqttjson.containsKey("SHUTTER-1"))) || ((topic=="tele/fleurblinds/RESULT") && (mqttjson.containsKey("SHUTTER-1")))) { 
    // Fleurblinds percentage/slider (text: p0n0; slider: p0h0) || {"Time":"2019-07-01T19:27:10","SHUTTER-1":{"position":100, "direction":0}}
    uint32_t tempval = mqttjson["SHUTTER-1"]["position"];
    p0h0.setValue(tempval);
    p0n0.setValue(tempval);
    p0n0.Set_background_color_bco(28651);

  } 
  else if(((topic=="tele/hobbyblinds/SENSOR") && (mqttjson.containsKey("SHUTTER-1"))) || ((topic=="tele/hobbyblinds/RESULT") && (mqttjson.containsKey("SHUTTER-1")))) { 
    // Hobbyblinds percentage/slider (text: p0n1; slider: p0h1) || {"Time":"2019-07-01T19:27:10","SHUTTER-1":{"position":100, "direction":0}}
    uint32_t tempval = mqttjson["SHUTTER-1"]["position"];
    p0h1.setValue(tempval);
    p0n1.setValue(tempval);
    p0n1.Set_background_color_bco(28651);
  }
  else if((topic=="stat/buffetkast/RESULT") || (topic=="tele/buffetkast/STATE")){
    // Buffetkast button (button: p1bt0)
    const char* tempval = mqttjson["POWER"];
    if(strcmp(tempval, "ON") == 0) { p1bt0.setValue(1); } else if(strcmp(tempval, "OFF") == 0) { p1bt0.setValue(0); }
  }
  else if((topic=="stat/whiskeykast/RESULT") || (topic=="tele/whiskeykast/STATE")){
    // Whiskeykast button (button: p1bt1)
    const char* tempval = mqttjson["POWER"];
    if(strcmp(tempval, "ON") == 0){ p1bt1.setValue(1); } else if(strcmp(tempval, "OFF") == 0){ p1bt1.setValue(0); }
    // Sync RTC Clock when available
    if (mqttjson.containsKey("Time"))
    {
      uint32_t d_y, d_m, d_d, t_h, t_m, t_s;
      sscanf(mqttjson["Time"], "%4d-%2d-%2dT%2d:%2d:%2d", &d_y, &d_m, &d_d, &t_h, &t_m, &t_s);
      uint32_t  time[7] = {d_y,d_m,d_d,t_h,t_m,t_s};
      rtc.write_rtc_time(time);
    }
  }
  else if((topic=="stat/vensterbankvoor/RESULT") || (topic=="tele/vensterbankvoor/STATE")){
    // Vensterbankvoor button (button: p1bt2)
    const char* tempval = mqttjson["POWER"];
    if(strcmp(tempval, "ON") == 0){ p1bt2.setValue(1); } else if(strcmp(tempval, "OFF") == 0){ p1bt2.setValue(0); }
  }
  else if((topic=="stat/barkeuken/RESULT") || (topic=="tele/barkeuken/STATE")){
    // Vensterbankvoor button (button: p1bt3)
    const char* tempval = mqttjson["POWER"];
    if(strcmp(tempval, "ON") == 0){ p1bt3.setValue(1); } else if(strcmp(tempval, "OFF") == 0){ p1bt3.setValue(0); }
  }
  else if(topic=="tele/wasmachine/SENSOR"){
    // Wasmachine stats (text voltage: p2n0; text wattage: p2n1; text kwh today: p2t0; p2s0 waveform) || {"Time":"2019-07-01T19:30:53","ENERGY":{"TotalStartTime":"2018-11-24T22:05:23","Total":132.210,"Yesterday":0.000,"Today":0.000,"Period":0,"Power":0,"ApparentPower":0,"ReactivePower":0,"Factor":0.00,"Voltage":230,"Current":0.000}}
    p2n0.setValue(mqttjson["ENERGY"]["Voltage"]);
    p2n1.setValue(mqttjson["ENERGY"]["Power"]);
      memset(buff, 0, sizeof(buff)); dtostrf(mqttjson["ENERGY"]["Today"], 4, 3, buff); 
    p2t0.setText(buff);
    WasmachineBuffer.push(map(mqttjson["ENERGY"]["Power"],0,2500,0,73));
    if(CurrentPage == 2)
    { 
      p2s0.addValue(0, WasmachineBuffer.last()); 
    }
  }
  else if (topic=="tele/fleurblinds/LWT") { if(messageTemp=="Online") { p6t3.Set_background_color_bco(28651); } else { p6t3.Set_background_color_bco(64073); } }
  else if (topic=="tele/whiskeykast/LWT") { if(messageTemp=="Online") { p6t4.Set_background_color_bco(28651); } else { p6t4.Set_background_color_bco(64073); } }
  else if (topic=="tele/wasmachine/LWT") { if(messageTemp=="Online") { p6t5.Set_background_color_bco(28651); } else { p6t5.Set_background_color_bco(64073); } }
  else if (topic=="tele/vensterbankvoor/LWT") { if(messageTemp=="Online") { p6t6.Set_background_color_bco(28651); } else { p6t6.Set_background_color_bco(64073); } }
  else if (topic=="tele/hobbyblinds/LWT") { if(messageTemp=="Online") { p6t7.Set_background_color_bco(28651); } else { p6t7.Set_background_color_bco(64073); } }
  else if (topic=="tele/fleurcam/LWT") { if(messageTemp=="Online") { p6t8.Set_background_color_bco(28651); } else { p6t8.Set_background_color_bco(64073); } }
  else if (topic=="tele/buffetkast/LWT") { if(messageTemp=="Online") { p6t9.Set_background_color_bco(28651); } else { p6t9.Set_background_color_bco(64073); } }
  else if (topic=="tele/barkeuken/LWT") { if(messageTemp=="Online") { p6t10.Set_background_color_bco(28651); } else { p6t10.Set_background_color_bco(64073); } }

  dbSerial.println();
}

void wakeup() {
  dbSerial.println("Wakeup: retrieve all device states");
  MQTTClient.publish("cmnd/sonoffs/STATE", "");     // retrieve all device states
  MQTTClient.loop();
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  dbSerial.println();
  dbSerial.print("Connecting to ");
  dbSerial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip_address, ip_gateway, ip_subnet, ip_dns);
  WiFi.begin(ssid, password);
  WiFi.setHostname(mqttDeviceName);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    dbSerial.print(".");
  }
  dbSerial.println("");
  dbSerial.print("WiFi connected - ");
  dbSerial.println("IP: " + WiFi.localIP().toString() + " | SUBNET: " + WiFi.subnetMask().toString() + " | GATEWAY: " + WiFi.gatewayIP().toString());
  p6t2.Set_background_color_bco(28651); // Green background color
  p5t5.setText(WiFi.localIP().toString().c_str());
}

// This function reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
void setup_mqtt() {
  MQTTClient.setServer(mqttServer, mqttPort);
  MQTTClient.setCallback(callback); 

  // Loop until we're reconnected
  while (!MQTTClient.connected()) {
    dbSerial.println("Connecting to MQTT...");
 
    if (MQTTClient.connect(mqttDeviceName, mqttUser, mqttPassword )) {
      digitalWrite (BUILTIN_LED, LOW);
      dbSerial.println("connected");
      p6t1.Set_background_color_bco(28651);   // Green background color
      MQTTClient.subscribe("stat/+/RESULT");  // stat/+/RESULT: Power on/off change data
      MQTTClient.subscribe("tele/+/RESULT");  // tele/+/+: tele/+/STATE: Power on/off periodic state data  |  tele/+/SENSOR: Sensor periodic data  |  tele/+/LWT: device online/offline status  |  tele/+/RESULT: blinds update
      MQTTClient.subscribe("tele/+/STATE");   // tele/+/+: tele/+/STATE: Power on/off periodic state data  |  tele/+/SENSOR: Sensor periodic data  |  tele/+/LWT: device online/offline status  |  tele/+/RESULT: blinds update
      MQTTClient.subscribe("tele/+/SENSOR");  // tele/+/+: tele/+/STATE: Power on/off periodic state data  |  tele/+/SENSOR: Sensor periodic data  |  tele/+/LWT: device online/offline status  |  tele/+/RESULT: blinds update
      MQTTClient.subscribe("tele/+/LWT");     // tele/+/+: tele/+/STATE: Power on/off periodic state data  |  tele/+/SENSOR: Sensor periodic data  |  tele/+/LWT: device online/offline status  |  tele/+/RESULT: blinds update
      wakeup();
    } else {
      dbSerial.print("failed with state ");
      dbSerial.print(MQTTClient.state());
      dbSerial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000); 
    }
  }
}

void setup(void) {
  delay(1000);

  pinMode(BUILTIN_LED, OUTPUT); digitalWrite (BUILTIN_LED, HIGH);
  // Start debuggin serial port
  // dbSerial.begin(115200);
  
  //Set the baudrate which is for debug and communicate with Nextion screen
  nexInit();

  //Register the pop/push event callback function of the current button component
  p0b0.attachPop(p0b0PopCallback, &p0b0);       // Fleurblinds open
  p0b1.attachPop(p0b1PopCallback, &p0b1);       // Fleurblinds close
  p0h0.attachPop(p0h0PopCallback, &p0h0);       // Fleurblinds slider
  p0b2.attachPop(p0b2PopCallback, &p0b2);       // Hobbyblinds open
  p0b3.attachPop(p0b3PopCallback, &p0b3);       // Hobbyblinds close
  p0h1.attachPop(p0h1PopCallback, &p0h1);       // Hobbyblinds slider
  p1bt0.attachPush(p1bt0PushCallback, &p1bt0);  // Buffetkast
  p1bt1.attachPush(p1bt1PushCallback, &p1bt1);  // Whiskeykast
  p1bt2.attachPush(p1bt2PushCallback, &p1bt2);  // Vensterbankvoor
  p1bt3.attachPush(p1bt3PushCallback, &p1bt3);  // Barkeuken
  p6t0.attachPush(p6t0PushCallback, &p6t0);     // enable/disable Debugging
  page0.attachPush(page0PushCallback, &page0);  // Page0 Blinds
  page1.attachPush(page1PushCallback, &page1);  // Page1 Switches
  page2.attachPush(page2PushCallback, &page2);  // Page2 Energy
  page3.attachPush(page3PushCallback, &page3);  // Page3 
  page4.attachPush(page4PushCallback, &page4);  // Page4 
  page5.attachPush(page5PushCallback, &page5);  // Page5 About
  page6.attachPush(page6PushCallback, &page6);  // Page6 Debug
    
  setup_wifi();

  setup_mqtt();

  // OTA
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(mqttDeviceName);
  ArduinoOTA.setPassword(mqttPassword);
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      dbSerial.println("Start updating " + type);
    })
    .onEnd([]() {
      dbSerial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      dbSerial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      dbSerial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) dbSerial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) dbSerial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) dbSerial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) dbSerial.println("Receive Failed");
      else if (error == OTA_END_ERROR) dbSerial.println("End Failed");
    });
}

void loop(void){
  // When a push event occured every time,
  // the corresponding component[right page id and component id] in touch event list will be asked
  nexLoop(nex_listen_list);

  // Reconnect Wifi if not connected
  if (WiFi.status() != WL_CONNECTED) {  
    p6t2.Set_background_color_bco(64073); // Red background color
    setup_wifi();
  }  

  // Reconnect MQTT if not connected
  if (!MQTTClient.connected()) {
    digitalWrite (BUILTIN_LED, HIGH);
    p6t1.Set_background_color_bco(64073); // Red background color
    setup_mqtt();
  }

  // MQTT Loop
  MQTTClient.loop();

  // OTA
  ArduinoOTA.handle();

}