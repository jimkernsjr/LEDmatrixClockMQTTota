//////////Include file for Node 55////////////////////////////
////Jim 12/2017

//Commands for this node:


//Limit 28 Chrs:|----------------------------| //measure
#define VERSION "ESP8266 V1.3.1-N59-GarClock" // this value can be queried as device 3
#define VERSIONno "1.3.1"

#define LOCATIONOP

#ifdef LOCATIONOP
  #define wifi_ssid_B "OP_2"				     	       // wifi station name
  #define wifi_password_B "*******"			       // wifi password
  #define mqtt_server "10.1.1.15"                // mqtt server IP
  
  #define wifi_ssid_A "OP_G"            				 // fallback wifi station name; if not present use same as A
  #define wifi_password_A "*******"        		 // fallback wifi password; if not present use same as A
#else
  #define wifi_ssid_A "location_01"          // fallback wifi station name; if not present use same as A
  #define wifi_password_A "********"           // fallback wifi password; if not present use same as A
  #define mqtt_server "192.168.2.15"           // mqtt server IP

  #define wifi_ssid_B "location_01"                     // fallback wifi station name; if not present use same as A
  #define wifi_password_B "*******"             // fallback wifi password; if not present use same as A
#endif

#define nodeId 59							               // node ID
#define DEBUG							                	 // uncomment for debugging
#define OnAtPowerup 1                        //jim - 0=off, 1=on
#define RETAINMSG 0                          //jim  comment to make non retained.  1=retained, 0=no
#define STAONLY                              //to turn off the built in AP

#define MQCON 3                             // MQTT link indicator
#define ACT1 1                              // Actuator pin (LED or relay to ground)   D1 on Wemos Mini
#define BTN 0                                // Button pin (also used for Flash setting)

#define OTAenable                            //enable OTA code upgrades
//#define OTAnotimeout
#define OTAname "WEMOSMINI-N59-GarClk"          //enable OTA code upgrades  --Max Chars about 20 & no _ or the port wont show!
//Limit 20 Chrs:|--------------------|       //measure


