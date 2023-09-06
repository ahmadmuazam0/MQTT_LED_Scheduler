#include <RTClib.h>
#include <PubSubClient.h>   // To handle the MQTT Client
#include <WiFiManager.h>    // To Manage the WiFi 
#include <ArduinoJson.h>    // To handle JSON Data
#include <EEPROM.h>
#include <FreeRTOS.h>


#define DEBUG Serial
#define LED1 4
#define LED2 5
#define TimerIntrupt 12

WiFiClient espClient;
PubSubClient mqttclient(espClient);


TaskHandle_t mqttTask;
TaskHandle_t schedularTask;


const char*     ssid        = "Epazz2FOffice4-2G";
const char*     password    = "epazzlahore";
const char*     MQTT_broker = "broker.hivemq.com";
const uint16_t  MQTTport    =  1883;
const char*     clientID    = "KYTHERTEK123";
const char*     Topic       = "KytherTek/ledScheduler";

bool led1State, led2State;


struct Scheduler_t {

  uint8_t id;
  uint8_t DayOfWeek;
  uint8_t startHour;
  uint8_t startMinute;
  uint8_t endHour;
  uint8_t endMinute;
  uint8_t led1Rate;
  uint8_t led2Rate;

};


Scheduler_t Schedule[5];

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
  DEBUG.print("The ID of the Schedular is: ");
  DEBUG.println(receivedSchedule.id);

  updateEEPROM(receivedSchedule);
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

uint16_t mils=millis();

void setup() {
  DEBUG.begin(115200);
  pinMode(LED1,OUTPUT);
  pinMode(LED2,OUTPUT);

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
              "MQTT",
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
  uint8_t adress = 0;
  for (uint8_t i = 0; i < 5; i++)
  {
    EEPROM.get(adress,Schedule[i+1]);
    adress = i*sizeof(Scheduler_t);
  }

  for (;;)
  {
    /* code */
  }
  
  
}
void loop() {
  // put your main code here, to run repeatedly:
}
