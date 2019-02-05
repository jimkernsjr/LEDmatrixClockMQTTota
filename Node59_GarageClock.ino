
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!PICK ONE TO CHOOSE THE NODE !!!!!!!!!!!!!!!!!!!!!!!!
//*ESP8266
#include "Node59_GarageClock_Settings.h"                //OTA deployed date:
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

///////////////PIN ASSIGNMENTS://///////////////////
//DIGITAL                                         //
//------------------------------------------------//
// 8                                              //
// 7                                              //
// 6                                              //
// 5                                              //
// 4  DHT  (Built in LED)                         //
// 3                                              //
// 2                                              //
// 1                                              //
// 0                                              //
////////////////////////////////////////////////////

////////////////////DEVICES:////////////////////////
//IN=SB=to node, OUT=NB=toward MQTT               //
//------------------------------------------------//
//48  (OUT) TEMP                                  //
//49  (OUT) HUMIDITY                              //
//50  (OUT) NTP TIME                              //
////////////////////////////////////////////////////

//////////////Jim's Customizations Comments/////////////////////////////////////////////////////////////////////////////
//2017/04   jim integrated enableable/disableable OTA system
//****ALERT***  KEEP SPIFFS AT 128, NO NEED FOR HIGHER AND IT MUST HAVE ENOUGH ROOM IN THE 
//              **PROGRAM AREA** NOT SPIFFS AREA FOR THE NEW FILE.
//              **For Sonoff and ECOPLUGS, the proper setting for Flash Size is 1M (128K SPIFFS) 
//seperated the header files from the code
//added on at startup option
//added send version at start
//customizable OTA name
//made the option for persistant messages or not.
//  Reserved ranges for node devices, as implemented in the gateway are:
//  0  - 16       Node system devices
//  16 - 32       Binary output (LED, relay)
//  32 - 40       Integer output (pwm, dimmer)
//  40 - 48       Binary input (button, switch, PIR-sensor)
//  48 - 64       Real input (temperature, humidity)
//  64 - 72       Integer input (light intensity)

//////////////End Jim's Customizations Comments/////////////////////////////////////////////////////////////////////////


//////////////Begin Computourist Comments///////////////////////////////////////////////////////////////////////////////

//	ESP_SONOFF_V1.2
//
//	This MQTT client will is designed to run on a SONOFF device.
//	https://www.itead.cc/smart-home/sonoff-wifi-wireless-switch.html
//
//	It will connect over Wifi to the MQTT broker and controls a digital output (LED, relay):
//	- toggle output and send status message on local button press
//	- receive messages from the MQTT broker to control output, change settings and query state
//	- periodically send status messages
//
//	Several nodes can operate within the same network; each node has a unique node ID.
//	On startup the node operates with default values, as set during compilation.
//	Hardware used is a ESP8266 WiFi module that connects directly to the MQTT broker.
//
//	Message structure is equal to the RFM69-based gateway/node sytem by the same author.
//	This means both type of gateway/nodes can be used in a single Openhab system.
//
//	The MQTT topic is /home/esp_gw/direction/nodeid/devid
//	where direction is "sb" towards the node and "nb" towards the MQTT broker.
//
//	Defined devices are:
//	0	uptime:		read uptime in minutes
//	1	interval:	read/set transmission interval for push messages
//	3	version:	read software version
//	2	RSSI:		read radio signal strength
//	5	ACK:		read/set acknowledge message after a 'set' request
//	6	toggle:		read/set select toggle / timer function
//	7	timer:		read/set timer interval in seconds
//	10	IP:			Read IP address
//	16	actuator:	read/set relay output
//	40	button		tx only: button pressed
//  41  Flash Switch: RX Only -  This does an ON - wait - OFF/ON to keep motion type floodlights on
//  48  DHT Temp
//  49  DHT Humid
//  50  Fan Speed based on 48 TEMP
//	92	error:		tx only: device not supported
//	91	error:		tx only: syntax error
//	99	wakeup:		tx only: first message sent on node startup
//
//	Hardware connections as in Sonoff device:
//	-	pin 0 is connected to a button that switches to GND, pullup to VCC, also used to enter memory flash mode.
//	-	pin 13 is connected to LED and current limiting resistor to VCC, used to indicate MQTT link status
//	-	pin 12 is connected to the relais output
//
//	version 1.0 by computourist@gmail.com June 2016
//	version 1.1 by computourist@gmail.com July 2016
//		specific client name to avoid conflicts when using multiple devices.
//		added mqtt will to indicate disconnection of link
//	version 1.2 by computourist@gmail.com July 2016
//		fallback SSI and autorecovery for wifi connection. 
//		a connection attempt to each SSI is made during 20 seconds .
//


///////////////////////////////LEDMatrix///////////////////////////////////////////////////
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
//#define SENSORSHT
#define SENSORBME

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted

#define MAX_DEVICES 4
#define CLK_PIN   D5 // or SCK
#define DATA_PIN  D7 // or MOSI
#define CS_PIN    D8 // or SS

// Hardware SPI connection
MD_Parola P = MD_Parola(CS_PIN, MAX_DEVICES);
// Arbitrary output pins
// MD_Parola P = MD_Parola(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////SHT30 TEMP HUM SHIELD///////////////////////////////
#ifdef SENSORSHT
  #include <WEMOS_SHT3X.h>
  SHT3X sht30(0x45);
#endif
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////BME280 TEMP HUM SHIELD///////////////////////////////
#ifdef SENSORBME
#include "SparkFunBME280.h"
BME280 BMEsensor;

#endif
////////////////////////////////////////////////////////////////////////////////////////



long showPage, showNextPage;
String qPubErrorMessage="";
bool needTsync=true;

//#include <ESP8266WiFi.h>
#include "espWifiOTA.h"
#include <PubSubClient.h>
#include "NTP.h"
#define RESYNCMINS 30

#ifdef OTAenable
  #include <ESP8266mDNS.h>
  #include <WiFiUdp.h>
  #include <ArduinoOTA.h>
#endif


	//	sensor setting

#define SERIAL_BAUD 115200
#define HOLDOFF 1000							// blocking period between button messages

	//	STARTUP DEFAULTS

 
	long 	TXinterval = 90;						// periodic transmission interval in seconds
	long	TIMinterval = 20;						// timer interval in seconds
	bool	ackButton = false;						// flag for message on button press
	bool	toggleOnButton = true;					// toggle output on button press

	//	VARIABLES

	int		DID;									// Device ID
	int		error;									// Syntax error code
	long	lastPeriod = -1;						// timestamp last transmission
	long 	lastBtnPress = -1;						// timestamp last buttonpress
	long	lastMinute = -1;						// timestamp last minute
	long	upTime = 0;								// uptime in minutes
	int		ACT1State;								// status ACT1 output
	bool	mqttNotCon = true;						// MQTT broker not connected flag
	int		signalStrength;							// radio signal strength
	bool	wakeUp = true;							// wakeup indicator
	bool	setAck = false; 						// acknowledge receipt of actions
	bool	curState = true;						// current button state 
	bool	lastState = true;						// last button state
	bool	timerOnButton = false;					// timer output on button press
	bool	msgBlock = false;						// flag to hold button message
	bool	readAction;								// indicates read / set a value
	bool	send0, send1, send2, send3, send5, send6, send7;
	bool	send10, send16, send40, send41, send48, send49, send50, send51, send52, send99;			// message triggers
	char	buff_topic[30];							// mqtt topic
	char	buff_msg[32];							// mqtt message
	char	clientName[10];							// Mqtt client name

//These are moves outside local scopes because they are now used globally for the dispaly.
float temp;
float tempc;
float humi;
float ahpa;
float ainhg;
float aalt;
int minSinceSync=0;
String timeLast;

int pageFrequency[5]={10,10,5,5,5}; //SECONDS DURATION TO DISPLAY THE EXTRA "PAGES", base0=informational messages
#define PAGEDURATION 3

int showPageNo=1;

unsigned long previousMillis = 0;        // will store last temp humi fan update
#define stInterval  30           // interval at which to recheck (milliseconds)
#define rtInterval   5                //adjustment to subtract from interval in case of fail
int retries=1;                      //set to 1 initially to get a quicker first read

////////////////////////////////////////////////////Jim Start/////////////////////////////////////////////////////////

void errorblink (int blinks){
  errorblink(blinks,200);
}

void errorblink(int blinks, int blinkdelay){
  int prevstat = digitalRead(MQCON);
  int ebdelay=blinkdelay*1;
  digitalWrite(MQCON,!prevstat);//it may be on...turn it off and wait a sec.  remember the led is reversed of the state, 1=off
  for (int i= 0; i <  blinks; i++){  
    delay(ebdelay);
    digitalWrite(MQCON,!prevstat);
    delay(ebdelay);
    digitalWrite(MQCON,prevstat); 
  }
}

#ifdef SENSORSHT
void setSHTvars() {

  if(sht30.get()==0){
    humi=sht30.humidity;
    temp=sht30.fTemp;
    tempc=sht30.cTemp;
    Serial.print("Temperature in Celsius : ");
    Serial.println(tempc);
    Serial.print("Temperature in Fahrenheit : ");
    Serial.println(temp);
    Serial.print("Relative Humidity : ");
    Serial.println(humi);
    Serial.println();
    
  }
  else
  {
    Serial.println("SHT30 READ Error!");
  }
  delay(1000);
}
#endif

#ifdef SENSORBME
void setBMEvars() {
  
    humi=(BMEsensor.readFloatHumidity());
    tempc=(BMEsensor.readTempC());
    temp=(BMEsensor.readTempF());
    ahpa=(BMEsensor.readFloatPressure()/100); //hPa
    ainhg=ahpa/33.86;  //reference  https://www.convertunits.com/from/hpa/to/inhg
    aalt=(BMEsensor.readFloatAltitudeFeet());
    //aalt(BMEsensor.readFloatAltitudeMeters(), 2);
#ifdef DEBUG
    Serial.print("Temperature in Celsius : ");
    Serial.println(tempc);
    Serial.print("Temperature in Fahrenheit : ");
    Serial.println(temp);
    Serial.print("Relative Humidity : ");
    Serial.println(humi);
    Serial.print("Atmospheric Pressure : ");
    Serial.print(ahpa);
    Serial.println(" hPa");
    Serial.print("Atmospheric Pressure : ");
    Serial.print(ainhg);
    Serial.println(" inhg");
    Serial.print("Approx. Altitude = ");
    Serial.print(aalt);
    Serial.println(" ft");
#endif
 
   delay(500);
}
#endif

////////////////////////////////////////////////////Jim End/////////////////////////////////////////////////////////


void mqttSubs(char* topic, byte* payload, unsigned int length);

//	WiFiClient espClient;
	PubSubClient client(mqtt_server, 1883, mqttSubs, espClient); // instantiate MQTT client
	
	//	FUNCTIONS

//===============================================================================================

void pubMQTT(String topic, String topic_val){ // publish MQTT message to broker
#ifdef DEBUG
	Serial.print("topic " + topic + " value:");
	Serial.println(String(topic_val).c_str());
#endif

	client.publish(topic.c_str(), String(topic_val).c_str(), RETAINMSG);  //use this line for retained messages
	}

void pubErrorMessage(String immediateMessage) {              // send error message
        sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev91", nodeId);
        //sprintf(buff_msg, "MSG: %s", immediateMessage);
        pubMQTT(buff_topic, immediateMessage);
}


void mqttSubs(char* topic, byte* payload, unsigned int length) {	// receive and handle MQTT messages
	int i;
	error = 4; 										// assume invalid device until proven otherwise
#ifdef DEBUG
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
		}
	Serial.println();
#endif
	if (strlen(topic) == 27) {						// correct topic length ?
		DID = (topic[25]-'0')*10 + topic[26]-'0';	// extract device ID from MQTT topic
		payload[length] = '\0';						// terminate string with '0'
		String strPayload = String((char*)payload);	// convert to string
	readAction = (strPayload == "READ");			// 'READ' or 'SET' value
	if (length == 0) {error = 2;}					// no payload sent
	else {
		if (DID ==0) {								// uptime 
			if (readAction) {
				send0 = true;
				error = 0;
			} else error = 3;						// invalid payload; do not process
		}
//------------------
		if (DID==1) {								// transmission interval
			error = 0;
			if (readAction) {
				send1 = true;
			} else {								// set transmission interval
				TXinterval = strPayload.toInt();
				if (TXinterval <10 && TXinterval !=0) TXinterval = 10;	// minimum interval is 10 seconds
			}
		}
//------------------
		if (DID ==2) {								// RSSI
			if (readAction) {
				send2 = true;
				error = 0;
			} else error = 3;						// invalid payload; do not process
		}
//------------------
		if (DID==3) {								// version
			if (readAction) {
				send3 = true;
				error = 0;
			} else error = 3;						// invalid payload; do not process
		}
//------------------
		if (DID==5) {								// ACK
			if (readAction) {
				send5 = true;
				error = 0;
			} else if (strPayload == "ON") {
					setAck = true;
					if (setAck) send5 = true;
					error = 0;
			}	else if (strPayload == "OFF") {
					setAck = false;
					if (setAck) send5 = true;
					error = 0;
			}	else error = 3;
		}
//------------------		
		if (DID==6) {								// toggle / timer mode selection
			if (readAction) {
				send6 = true;
				error = 0;
			} else if (strPayload == "ON") {		// select toggle mode
					toggleOnButton = true;
					if (setAck) send6 = true;
					error = 0;
			}	else if (strPayload == "OFF") {		// select timer mode
					toggleOnButton = false;
					if (setAck) send6 = true;
					error = 0;
			}	else error = 3;
		}
//------------------		
		if (DID==7) {								// Timer interval
			error = 0;
			if (readAction) {
				send7 = true;
			} else {								// set timer interval
				TIMinterval = strPayload.toInt();
				if (TIMinterval <5 && TIMinterval !=0) TIMinterval = 5;	// minimum interval is 5 seconds
			}
		}
		if (DID ==10) {								// IP address 
			if (readAction) {
				send10 = true;
				error = 0;
			} else error = 3;						// invalid payload; do not process
		}
//------------------
		if (DID==16) {								// state of actuator
			if (readAction) {
				send16 = true;
				error = 0;
			} else if (strPayload == "ON") {
					ACT1State = 1;
					digitalWrite(ACT1,ACT1State);
					if (setAck) {
					  send16 = true;
            send3 = true;
					}
					error = 0;
			}	else if (strPayload == "OFF") {
					ACT1State = 0;
					digitalWrite(ACT1,ACT1State);
					if (setAck) send16 = true;
					error = 0;
			}	else error = 3;
		}
//------------------
//------------------    
	}
		} else error =1;
		if (error !=0) {							// send error message
				sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev91", nodeId);
				sprintf(buff_msg, "syntax error %d", error);
				pubMQTT(buff_topic, buff_msg);
			}
		}
  
	void connectMqtt() {								// reconnect to mqtt broker
		sprintf(buff_topic, "home/esp_gw/sb/node%02d/#", nodeId);
		sprintf(clientName, "ESP_%02d", nodeId);
		if (!client.loop()) {
			mqttNotCon = true;							// LED off means high voltage
#ifdef DEBUG
			Serial.print("Connect to MQTT broker...");
#endif
			if (client.connect(clientName,"home/esp_gw/disconnected",0,true,clientName)) { 
				mqttNotCon = false;						// LED on means low voltage
				client.subscribe(buff_topic);
	
#ifdef DEBUG
				Serial.println("connected");
#endif
			} else {
#ifdef DEBUG
				Serial.println("Failed, try again in 5 seconds");
#endif
				delay(5000);
				}
			digitalWrite(MQCON, mqttNotCon);			// adjust MQTT link indicator
			}
		}


void sendMsg() {								// send any outstanding messages
	int i;
	if (wakeUp) {									// send wakeup message
		wakeUp = false;
		sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev99", nodeId);
		sprintf(buff_msg, "NODE %d WAKEUP: %s",  nodeId, clientName);
		send99 = false;
		pubMQTT(buff_topic, buff_msg);
		}

	if (send0) {									// send uptime
		sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev00", nodeId);
		sprintf(buff_msg, "%d", upTime);
		send0 = false;
		pubMQTT(buff_topic, buff_msg);
		}

	if (send1) {									// send transmission interval
		sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev01", nodeId);
		sprintf(buff_msg, "%d", TXinterval);
		send1 = false;
		pubMQTT(buff_topic, buff_msg);
		}

	if (send2) {									// send transmission interval
		sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev02", nodeId);
		signalStrength = WiFi.RSSI();
		sprintf(buff_msg, "%d", signalStrength);
		send2 = false;
		pubMQTT(buff_topic, buff_msg);
		}

	if (send3) {									// send software version
		sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev03", nodeId);
		for (i=0; i<sizeof(VERSION); i++) {
			buff_msg[i] = VERSION[i];}
		buff_msg[i] = '\0';
		send3 = false;
		pubMQTT(buff_topic, buff_msg);
		}

	if (send5) {									// send ACK state
		sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev05", nodeId);
		if (!setAck) sprintf(buff_msg, "OFF");
		else sprintf(buff_msg, "ON");
		pubMQTT(buff_topic, buff_msg);
		send5 = false;
		}

	if (send6) {									// send toggleOnButton state
		sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev06", nodeId);
		if (!toggleOnButton) sprintf(buff_msg, "OFF");
		else sprintf(buff_msg, "ON");
		pubMQTT(buff_topic, buff_msg);
		send6 = false;
		}

	if (send7) {									// send timer value
		sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev07", nodeId);
		sprintf(buff_msg, "%d", TIMinterval);
		pubMQTT(buff_topic, buff_msg);
		send7 = false;
		}

	if (send10) {								// send IP address
		sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev10", nodeId);
		for (i=0; i<16; i++) {
			buff_msg[i] = IP[i];}
		buff_msg[i] = '\0';
		pubMQTT(buff_topic, buff_msg);
		send10 = false;
		}

	if (send16) {									// send actuator state
		sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev16", nodeId);
		if (ACT1State ==0) sprintf(buff_msg, "OFF");
		if (ACT1State ==1) sprintf(buff_msg, "ON");
		pubMQTT(buff_topic, buff_msg);
		send16 = false;
		}

	if (send40) {									// send button pressed message
		sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev40", nodeId);
		if (ACT1State ==0) sprintf(buff_msg, "OFF");
		if (ACT1State ==1) sprintf(buff_msg, "ON");
		pubMQTT(buff_topic, buff_msg);
		send40 = false;
		}

  if (send48) {            // Temperature
    sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev48", nodeId);
    sprintf(buff_msg, "%0.1f", temp);
    pubMQTT(buff_topic, buff_msg);
    send48 = false;
  }
  if (send49) {           // Humidity
      sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev49", nodeId);
      sprintf(buff_msg, "%0.f", humi);
      pubMQTT(buff_topic, buff_msg);
      send49 = false;
  }
  if (send50) {           // ntp time
      sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev50", nodeId);
            setTimeStrings();
      for (i=0; i<12; i++) {buff_msg[i] = NTPfulltime[i];}
      buff_msg[i] = '\0';
      pubMQTT(buff_topic, buff_msg);
      send50 = false;
  }
  if (send51) {           // ntp date
      sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev51", nodeId);
      for (i=0; i<12; i++) {buff_msg[i] = NTPm_d_yr[i];}
      buff_msg[i] = '\0';
      pubMQTT(buff_topic, buff_msg);
      send51 = false;
  }
  if (send52) {           //  Baro
      sprintf(buff_topic, "home/esp_gw/nb/node%02d/dev52", nodeId);
      sprintf(buff_msg, "%0.f", ainhg);
      pubMQTT(buff_topic, buff_msg);
      send52 = false;  }

}
  
//	SETUP

//===============================================================================================
	void setup() {									// set up serial, output and wifi connection

#ifdef DEBUG
  Serial.begin(SERIAL_BAUD);
  Serial.print("Starting Node ");
  Serial.println(nodeId);
#endif

  P.begin();
  P.print("Start...");
//  errorblink(5,200);
  
	pinMode(MQCON,OUTPUT);							// configure MQTT connection indicator

#ifdef SENSORBME
  BMEsensor.settings.commInterface = I2C_MODE;
  BMEsensor.settings.I2CAddress = 0x76;

  BMEsensor.settings.runMode = 3; //Normal mode
  BMEsensor.settings.tStandby = 0;
  BMEsensor.settings.filter = 0;
  BMEsensor.settings.tempOverSample = 1;
  BMEsensor.settings.pressOverSample = 1;
  BMEsensor.settings.humidOverSample = 1;

  
  delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
#ifdef DEBUG  
  Serial.print("Starting BME280... result of .begin(): 0x");  
  //Calling .begin() causes the settings to be loaded
  Serial.println(BMEsensor.begin(), HEX);

  Serial.print("Displaying ID, reset and ctrl regs\n");
  
  Serial.print("ID(0xD0): 0x");
  Serial.println(BMEsensor.readRegister(BME280_CHIP_ID_REG), HEX);
  Serial.print("Reset register(0xE0): 0x");
  Serial.println(BMEsensor.readRegister(BME280_RST_REG), HEX);
  Serial.print("ctrl_meas(0xF4): 0x");
  Serial.println(BMEsensor.readRegister(BME280_CTRL_MEAS_REG), HEX);
  Serial.print("ctrl_hum(0xF2): 0x");
  Serial.println(BMEsensor.readRegister(BME280_CTRL_HUMIDITY_REG), HEX);

  Serial.print("\n\n");
#else
  String resultsensor=String(BMEsensor.begin());  //start it if debug not on
#endif
#endif

//	digitalWrite(MQCON,HIGH);  						// switch off MQTT connection indicator
#ifdef DEBUG
   Serial.println("DIO Init Done");
#endif

	send0 = false;
	send1 = false;
	send3 = true;                     //send version on startup
	send5 = false;
	send7 = false;
	send10 = true;									// send IP on startup
	send16 = false;
	send40 = false;
  send48 = true;
  send49 = true;
  send50 = true;
  send51 = true;
  send52 = true;
  delay(1000);                  //wait for DHT to settle  


#ifdef DEBUG
	  Serial.println("Beginning Main Loop.");
#endif
  P.print("Conn...");
  yield();
	}

//	LOOP

//===============================================================================================
	void loop() {												// Main program loop

  bool connStat=false;
	if (WiFi.status() == WL_CONNECTED){connStat=true;}
  if ( millis() > (30*60*1000) ){
    if(!setAck){
#ifdef OTAnotimeout
      wifiLoop(1);   //its been running 30 mins, and ACKs are turned off, stop the OTA system - disable this function if notimeout defined
#else
      wifiLoop(0);   //its been running 30 mins, and ACKs are turned off, stop the OTA system
#endif
    }else{
      wifiLoop(1);   //its been running 30 mins, and ACKs are turned on, OTA system enabled
    }
  }else{
    wifiLoop(1);     //its been running less than 30 mins so OTA is enabled
  }
  if (connStat==false){
      P.print("AP=");
      delay(1000);
      yield;
      P.print(WiFi.SSID());
      showNextPage=millis()+(pageFrequency[0]*1000);
  }

	if (!client.connected()) {									// MQTT connected ?
		connectMqtt();
		}
	client.loop();
	
	handleTime();
  if (NTPfulltimeNS != timeLast){  //show the new time if it changed...but onlyid it's ben running for >1m
      timeLast=NTPfulltimeNS;
      if ((showPageNo=1) && (upTime>1)){P.print(NTPfulltimeNS);}
  }
  //pages:  1=time, 2=temp 3=humi 4=psi  
  //Variables used to change:
    //#define PAGEFREQUENCY 20  /SECONDS DURATION TO DISPLAY THE EXTRA "PAGES"
    //#define PAGEDURATION 3
    //long showNextPage;
    //int showPageNo=1;

  if (millis()>=showNextPage){
    showPageNo++;
    if (showPageNo>4){showPageNo=1;}

      #ifdef DEBUG
        Serial.print("Page change to ");
        Serial.println(showPageNo);
      #endif
    
    String buffer="";
    
    if (showPageNo==1){
      buffer=NTPfulltimeNS;
    }
    if (showPageNo==2){
      buffer=String(int(temp))+" F";
    }
    if (showPageNo==3){
      buffer=String(int(humi))+" %";
    }
    if (showPageNo==4){
      buffer=String(int(ainhg))+" inhg";
    }

      #ifdef DEBUG
        Serial.print("Display: ");
        Serial.println(buffer);
      #endif
      P.print(buffer);
      showNextPage=millis()+(pageFrequency[showPageNo]*1000);
  }


// DETECT INPUT CHANGE

// TIMER CHECK

	if (TIMinterval > 0 && timerOnButton) {						// =0 means no timer
		if ( millis() - lastBtnPress > TIMinterval*1000) {		// timer expired ?
			timerOnButton = false;								// then end timer interval 
			ACT1State = LOW;									// and switch off Actuator
			digitalWrite(ACT1,ACT1State);
			send16 = true;
			}
		}

// INCREASE UPTIME / MINUTE TIMER 

	if (lastMinute != (millis()/60000)) {						// another minute passed ?
		lastMinute = millis()/60000;
		upTime++;
#ifdef SENSORSHT
    setSHTvars();
#endif
#ifdef SENSORBME
    setBMEvars();
#endif
		}
   
// PERIODIC TRANSMISSION


	if (TXinterval > 0){
		int currPeriod = millis()/(TXinterval*1000);
		if (currPeriod != lastPeriod) {							// interval elapsed ?
			lastPeriod = currPeriod;

			// list of sensordata to be sent periodically..
			// remove comment to include parameter in transmission
 
//			send10 = true;									// send IP address
			send2 = true;			  							// send RSSI
//			send3 = true;										// send version
			send16 = true;										// output state
      send48 = true;                    // temp
      send49 = true;                    //humidity
      send50 = true;                    //NTP time
      send51 = true;                    //NTP date
      send52 = true;                    //pressure inhg
      #ifdef DEBUG
        Serial.println("Messages transmitted for interval.");
        Serial.println();
      #endif
		}
	}

  unsigned long currentMillis = millis();
  int interval=stInterval;
  if (retries>0){interval=rtInterval;}
  if ((currentMillis - previousMillis) >= (interval*1000)) {
    previousMillis = currentMillis;   // save the last time temp humi fan updated
  }
  if (qPubErrorMessage!=""){
      pubErrorMessage(qPubErrorMessage);
      qPubErrorMessage="";
  }
	sendMsg();													// send any mqtt messages
  yield();
}		// end loop
  

