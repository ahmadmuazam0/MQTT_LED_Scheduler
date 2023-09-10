#include <RTClib.h>
#include <PubSubClient.h>   // To handle the MQTT Client
#include <WiFi.h>    // To Manage the WiFi 
#include <ArduinoJson.h>    // To handle JSON Data
#include <EEPROM.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define DEBUG Serial
#define LED1 4
#define LED2 5
#define MAX_SCHEDULES 5

RTC_DS3231 rtc;
WiFiClient espClient;
PubSubClient mqttclient(espClient);

QueueHandle_t xQueue;
TaskHandle_t mqttTask;
TaskHandle_t schedularTask;

DateTime nowClock = rtc.now();

const char*     ssid        = "Epazz2FOffice4-2G";
const char*     password    = "epazzlahore";
const char*     MQTT_broker = "broker.hivemq.com";
const uint16_t  MQTTport    =  1883;
const char*     clientID    = "KYTHERTEK123";
const char*     Topic       = "KytherTek/ledScheduler";
const char*     txTopic     = "KytherTek/sendScheduler";
uint64_t startAlarm, stopAlam;

bool startBlinking = false, getSchedule = false, led1State = false, led2State = false;

struct Scheduler_t {

  uint8_t id;
  uint8_t DayOfWeek;
  uint8_t startHour;
  uint8_t startMinute;
  uint8_t endHour;
  uint8_t endMinute;
  uint8_t led1Rate;
  uint8_t led2Rate;
  bool    flag;

};

Scheduler_t Schedule[5];
Scheduler_t curretnSchedule;

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

  Scheduler_t receivedSchedule;
  receivedSchedule.id           = doc["id"];
  receivedSchedule.DayOfWeek    = doc["dayOfWeek"];
  receivedSchedule.startHour    = doc["startHour"];
  receivedSchedule.startMinute  = doc["startMinute"];
  receivedSchedule.endHour      = doc["endHour"];
  receivedSchedule.endMinute    = doc["endMinute"];
  receivedSchedule.led1Rate     = doc["led1Rate"];
  receivedSchedule.led2Rate     = doc["led2Rate"];
  receivedSchedule.flag         = false;
  DEBUG.print("The ID of the Schedular is: ");
  DEBUG.println(receivedSchedule.id);

  updateEEPROM(receivedSchedule);
  // readScheduleFromEEPROM();
}

/**
 * To connect the device with WIFI network
 * 
 */
void wifiConnect() {

WiFi.mode(WIFI_STA);
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
 }
}

/**
 * To initialize the EEPROM 
 * @returns None
 */
void EEPROMInit(){
  if(!EEPROM.begin(sizeof(Scheduler_t) * 5)){
    DEBUG.printf("EEPROM Initialization Failed. Reboot system");
  }
}

/**
 * To Store and Update the Schedule in EEPROM
 * @param sched: Taking Value of scheduler to store in EEPROM
 * @returns None
 */
void updateEEPROM(Scheduler_t &sched){
  if (sched.id > 0 && sched.id <= 5 )
  {  
  uint8_t address = (sched.id-1) * sizeof(Scheduler_t);
  EEPROM.put(address,sched);
  EEPROM.commit();
  }else DEBUG.printf("Invalid ID");
}

/**
 * To set the Blinking rate per second of LED 1
 * @param rate: At which the led should blink per second
 * @returns none
 */
void BlinkLED1(uint8_t rate){
  unsigned long previousMillis1 = millis();
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

void readScheduleFromEEPROM() {
  int addr = 0;
  for (uint8_t i = 0; i < MAX_SCHEDULES; i++)
  {
    addr = i * sizeof(Scheduler_t);;
    EEPROM.get(addr, Schedule[i]);
  }
 
}

/**
 * To set the Blinking Rate of LED 2 Per Second
 * @param rate: At which the LED 2 should Blink
 * @returns None
 */
void BlinkLED2(uint8_t rate){
  unsigned long previousMillis2 = millis();
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
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable   detector
  DEBUG.begin(115200);
  pinMode(LED1,OUTPUT);
  pinMode(LED2,OUTPUT);
  if(rtc.lostPower())rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Adjust the Date and time at Compiling
  xQueue = xQueueCreate(1,sizeof(String));
  while (xQueue == NULL);
  
  xTaskCreatePinnedToCore(
              mqttHandeling,
              "MQTT",
              2048,
              NULL,
              1,
              &mqttTask,
              0
  );

  xTaskCreatePinnedToCore(
              blinkScheduling,
              "Blink",
              2048,
              NULL,
              1,
              &schedularTask,
              1
  );

}

void sendCurrentStatus(Scheduler_t &Sched)
{
  String data;
  data += "Current Schedule ID: ";
  data += Sched.id;
  data += " Led 1 rate : ";
  data += Sched.led1Rate;
  data += " Led 2 rate : ";
  data += Sched.led2Rate;
  xQueueSend(xQueue,&data,portMAX_DELAY);
}

void mqttHandeling(void* pvParameters){
  EEPROMInit();
  wifiConnect();
  mqttSetup();
  String rxData;
  for(;;){
    mqttclient.loop();
    if (!mqttclient.connected())
    {
      mqttReconnect();
    }
    mqttclient.subscribe(Topic);
  }

  xQueueReceive(xQueue,&rxData,portMAX_DELAY);
  mqttclient.publish(txTopic,rxData.c_str(),sizeof(rxData));
  vTaskDelay(pdMS_TO_TICKS(10));
}

void blinkScheduling(void* pvParameter){
  
  for (;;)
  {
    while(getSchedule)
    {
      readScheduleFromEEPROM();
      uint8_t i=0;
      nowClock = rtc.now();
      if (Schedule[i].DayOfWeek == nowClock.day())
      {
        if (Schedule[i].startHour == nowClock.hour() && Schedule[i].startMinute == nowClock.minute())
        {
          startBlinking = true;
          Schedule[i].flag = true;
          curretnSchedule = Schedule[i];
          getSchedule = false;
          sendCurrentStatus(curretnSchedule);
          break;
        }               
      }
      nowClock = rtc.now();
      i++ > 5 ? i=0:i;
      
    }
    while (startBlinking)
    {
      nowClock = rtc.now();
      if (curretnSchedule.endHour == nowClock.hour() && curretnSchedule.endMinute == nowClock.minute())
        {
          startBlinking = false;
          getSchedule = true;
          curretnSchedule.flag = false;
        }
      BlinkLED1(curretnSchedule.led1Rate);
      BlinkLED2(curretnSchedule.led2Rate);
    }
    
  }
  
}
void loop() {
  // put your main code here, to run repeatedly:
}
