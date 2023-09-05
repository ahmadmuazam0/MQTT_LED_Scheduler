#include <PubSubClient.h>   // To handle the MQTT Client
#include <WiFiManager.h>    // To Manage the WiFi 
#include <ArduinoJson.h>    // To handle JSON Data

#define DEBUG Serial
#define LED1 22
#define LED2 5

WiFiClient espClient;
PubSubClient mqttclient(espClient);
unsigned long previousMillis1 = millis();
unsigned long previousMillis2 = millis();
uint8_t l1rate =0,l2rate=0;
const char* ssid = "GUEST";
const char* password = "12341234";
const char*     MQTT_broker = "broker.hivemq.com";
const uint16_t  MQTTport    = 1883;
const char*     clientID    = "ESP32";
const char*     Topic       = "ESP32/ledScheduler";

bool led1State, led2State;

enum ScheduleID_t{
  SCHEDULE_1 = 1,
  SCHEDULE_2,
  SCHEDULE_3,
  SCHEDULE_4,
  SCHEDULE_5
};

struct Scheduler {

  ScheduleID_t id;
  uint8_t DayOfWeek;
  uint8_t startHour;
  uint8_t startMinute;
  uint8_t endHour;
  uint8_t endMinute;
  uint8_t led1Rate;
  uint8_t led2Rate;

};


Scheduler S1,S2,S3,S4,S5;

/**
 * To setup the MQTT client and Broker Handshake
 * @returns none
 */
void mqttSetup()   
{
  mqttclient.setServer(MQTT_broker,MQTTport);
  mqttclient.setCallback(callback);
}



/**
 * Incase of connection lost this will connect the client with broker
 * @returns none
 */
void mqttReconnect(){
  while (!mqttclient.connected())
  {
    if(mqttclient.connect(clientID)){
      DEBUG.printf("Connected to ID : %ld",clientID);
      mqttclient.subscribe(Topic);
    } else DEBUG.print("Failed to connect, Retrying . . .");
  }
  
}

/**
 * Callback to handle the message request received from the Broker
 * @param Topic: On which the message will be received
 * @param Message: The Message that received from the publisher
 * @param Length: Length of the message the is recieved
 * @returns none
 */
void callback(char* topic, byte* message, unsigned int length) {
  
  char json[length+1];
  memcpy(json,message,length);
  json[length]='\0';
  DEBUG.println(json);
  // Parse the JSON payload
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    DEBUG.println("Failed to parse JSON");
    return;
  }

  Scheduler receivedSchedule;
  receivedSchedule.id           = doc["id"];
  receivedSchedule.DayOfWeek    = doc["dayOfWeek"];
  receivedSchedule.startHour    = doc["startHour"];
  receivedSchedule.startMinute  = doc["startMinute"];
  receivedSchedule.endHour      = doc["endHour"];
  receivedSchedule.endMinute    = doc["endMinute"];
  l1rate    = doc["led1Rate"];
  l2rate    = doc["led2Rate"];
  DEBUG.print("The ID of the Schedular is: ");
  DEBUG.println(receivedSchedule.id);
 // DEBUG.println("Start Time is %d : %d",receivedSchedule.startHour ,receivedSchedule.startMinute );

}

/**
 * To connect the device with WIFI network
 * 
 */
void wifiConnect() {
// wm.autoConnect("KytherTek_MQTT"); 
// wm.setConnectTimeout(10);     // It'll try reconnecting for 10s

WiFi.mode(WIFI_STA);
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
 }
}


void UpdateStatus(){
  
}

/**
 * To set the Blinking rate per second of LED 1
 * @param rate: At which the led should blink per second
 * @returns none
 */
void BlinkLED1(uint8_t rate){
  //unsigned long previousMillis1 = millis();
  if(rate > 0) {
    if(millis() - previousMillis1 >= (1000/rate)) {
      previousMillis1 = millis();
      led1State = !led1State;
      digitalWrite(LED1, led1State);
    }
  } else {
    digitalWrite(LED1,LOW);
    led1State = false;}
}

/**
 * To set the Blinking Rate of LED 2 Per Second
 * @param rate: At which the LED 2 should Blink
 * @returns None
 */
void BlinkLED2(uint8_t rate){
  //unsigned long previousMillis2 = millis();
  if(rate > 0) {
    if(millis() - previousMillis2 >= (1000/rate)) {
      previousMillis2 = millis();
      led2State = !led2State;
      digitalWrite(LED2, led2State);
    }
  } else {
    digitalWrite(LED2,LOW);
    led2State = false;}
}

uint16_t mils=millis();

void setup() {
  DEBUG.begin(115200);
  pinMode(LED1,OUTPUT);
  pinMode(LED2,OUTPUT);
  digitalWrite(LED1,HIGH);
  delay(1000);
  digitalWrite(LED1,LOW);
  digitalWrite(LED2,HIGH);
  delay(1000);
  digitalWrite(LED2,LOW);
  wifiConnect();
  mqttSetup();
}

void loop() {
  // put your main code here, to run repeatedly:
  BlinkLED1(l1rate);
  BlinkLED2(l2rate);
  if (!mqttclient.connected())
  {
    mqttReconnect();
  }


  mqttclient.loop();
  
}
