#include <RTClib.h>
#include <PubSubClient.h>   // To handle the MQTT Client
#include <WiFiManager.h>    // To Manage the WiFi 
#include <ArduinoJson.h>    // To handle JSON Data
#include <EEPROM.h>
#include <FreeRTOS.h>

#define DEBUG Serial
#define LED1 4
#define LED2 5
#define MAX_SCHEDULES 5

RTC_DS3231 rtc;
WiFiClient espClient;
PubSubClient mqttclient(espClient);

hw_timer_t *BlinkTimer = NULL;
void IRAM_ATTR onTimerIntrupt(){

  if (startBlinking)
  {
    startBlinking = false;
    getSchedule = true;
    
  }else
  {
    startBlinking = true;
    getSchedule = false;
    timerAlarmWrite(BlinkTimer, stopAlam, false);
    timerAlarmEnabled(BlinkTimer);
  }
  
}

TaskHandle_t mqttTask;
TaskHandle_t schedularTask;
DateTime nowClock = rtc.now();

const char*     ssid        = "Epazz2FOffice4-2G";
const char*     password    = "epazzlahore";
const char*     MQTT_broker = "broker.hivemq.com";
const uint16_t  MQTTport    =  1883;
const char*     clientID    = "KYTHERTEK123";
const char*     Topic       = "KytherTek/ledScheduler";
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
  uint8_t address = sched.id * sizeof(Scheduler_t);
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

void readScheduleFromEEPROM(int id, Scheduler_t &sched) {
  int addr = id * sizeof(Scheduler_t);
  EEPROM.get(addr, sched);
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

void getScheduleptr(void){

  uint8_t adress = 0;
  uint8_t smallestHour;
  uint8_t smallestMinute;
  uint8_t currentHour;
  uint8_t currentMinute;


  nowClock = rtc.now();

  for (uint8_t i = 0; i < MAX_SCHEDULES; i++)
  {
    adress = i*sizeof(Scheduler_t);
    EEPROM.get(adress,Schedule[i]);
    if (Schedule[i].flag ==false && Schedule[i].startHour >= nowClock.hour() && Schedule[i].startMinute >= nowClock.minute())
    {
      currentHour    = Schedule[i].startHour - nowClock.hour();
      currentMinute  = Schedule[i].startMinute - nowClock.minute();
      if (currentHour <= smallestHour && currentMinute <= smallestMinute)
      {
        smallestHour = currentHour;
        smallestMinute = currentMinute;
        curretnSchedule = Schedule[i];
      }     
    }

    nowClock = rtc.now();
    

  }
  getSchedule = false;
  
}

/**
 * To set the Start and End Time of Schedule to trigger Timer Interrupt Accordingly
 * @param Sched : Gets the Scheduler from which the Time has to get to set Alarms
 * @returns NONE
 */

void getAlarmTime(Scheduler_t Sched){

 uint8_t hours,minutes;
 nowClock = rtc.now();
 hours =  curretnSchedule.startHour -  nowClock.hour() * 60;
 minutes = curretnSchedule.startMinute - nowClock.minute();
 startAlarm = (hours + minutes) * 60 * 1000;

 hours =  curretnSchedule.endHour -  nowClock.hour() * 60;
 minutes = curretnSchedule.endMinute - nowClock.minute();
 stopAlam = (hours + minutes) * 60 * 1000;

 timerAlarmWrite(BlinkTimer,startAlarm,false);
 timerAlarmEnable(BlinkTimer);

}

uint16_t mils=millis();

void setup() {

  DEBUG.begin(115200);
  pinMode(LED1,OUTPUT);
  pinMode(LED2,OUTPUT);
  if(rtc.lostPower())rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Adjust the Date and time at Compiling
  BlinkTimer = timerBegin(0,80000,true);   // 0: Timer 0,  80000: Prescaller,  true: Count Upwards 1000cycle = 1Second
  timerAttachInterrupt(BlinkTimer,&onTimerIntrupt,false);
  xTaskCreatePinnedToCore(
              mqttHandeling,
              "MQTT",
              1024,
              NULL,
              1,
              &mqttTask,
              0
  );

  xTaskCreatePinnedToCore(
              blinkScheduling,
              "Blink",
              1024,
              NULL,
              1,
              &schedularTask,
              1
  );

}


void mqttHandeling(void* pvParameters){
  EEPROMInit();
  wifiConnect();
  mqttSetup();
  for(;;){
    mqttclient.loop();
    if (!mqttclient.connected())
    {
      mqttReconnect();
    }
  }
}

void blinkScheduling(void* pvParameter){
  
  for (;;)
  {
    if(getSchedule)getScheduleptr();
    while (startBlinking)
    {
      BlinkLED1(curretnSchedule.led1Rate);
      BlinkLED2(curretnSchedule.led2Rate);
    }
    
  }
  
  
}
void loop() {
  // put your main code here, to run repeatedly:
}
