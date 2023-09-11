# MQTT_LED_Scheduler
MQTT based OUTPUT control schedular to Turn ON/OFF or set rate of Blinking an led at specific time and Day of Week

Connect a DS3231 with esp32, 
	SCL --> scl of ESP32
	SCK --> sck of ESP32
	Vcc --> 3.3V of ESP32
	GND --> gnd of ESP32
	
Connect two LEDs:
	Led 1 --> D4 of ESP32
	Led 2 --> D5 of ESP32
	you can change the pins of LEDs by updating the defined macro
	
				
Server Set-Up:

Go to Hivemq Broker Site or https://www.hivemq.com/demos/websocket-client/
Set the MQTT port to 1883
In the Publish Tab set the Topic on which you want to pUblish the message in our case it is "KytherTek/ledScheduler"
In the Subscribe Tab set the Topic from which you want to receive the status, in our case it is "KytherTek/sendScheduler"
Hit the Connect button and wait for connected message.
In Publish section enter the message you want to send. In our case we will send JSON string and the format is follows:

{
"id"          : 4,   // The Scheduler Number
"dayOfWeek"   : 3,   // The day of week on which you want to start the process
"startHour"   : 10,  // The Hour at which it start
"startMinute" : 45,  // The Minute at which it start
"endHour"     : 11,  // The Hour at which it stops
"endMinute"   : 20,  // The Hour at which it stops
"led1Rate"    : 7,   // The Rate of LED 1 to blink per second
"led2Rate"    :3     // The Rate of LED 2 to blink per second
}


Hit publish by setting the values and See the ESP32 DO its JOB :)
