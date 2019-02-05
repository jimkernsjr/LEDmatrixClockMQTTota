//***********************************************************ESP WIFI AND OTA LIBRARY******************************************
//////////////////////////////////////////////////
// An include file for all the ESP Wifi goodies //
//  Created 12/21/2017     Jim                  //
//                                              //
//  In Loving Memory:                           //
//  BBOSS  Baby Boy Oreo Sylvester Shore        //
//  05/2016 -> 12/02/2017                       //
//////////////////////////////////////////////////
//
//Usage:
//Put this command in the main loop:
//      wifiLoop(1);  //you can almost always call it with a overload of 1, the OTAenable not being defined will compile it out anyway
//                    you can overload it with 0 to turn off the OTA feature, i.e. after a certain time to maybe disable it to turn the
//                    port off to stop the Arduino and Atom IDE from being full of ports and to stop accidental overwrites.  
//                    A good example would be if millis>(30*60*1000){wifiLoop(0)}else{wifiLoop(1)}.  To make it easy to reenable with 
//                    a MQTT command, for example tag it onto the setack like so:
//                    if millis()>(30*60*1000){if(!setAck){wifiLoop(0)}else{wifiLoop(1)}}else{wifiLoop(1)}
//                    WARNING - if you dont have OTAenable defined, it does not matter what you overload it with.
//*****************************************************************************************************************************
#include <ESP8266WiFi.h>
WiFiClient  espClient;

  
#ifdef OTAenable
    #include <ESP8266mDNS.h>
    #include <WiFiUdp.h>
    #include <ArduinoOTA.h>
#endif


String  IP;                   // IPaddress of ESP
long  otaUpTime = 0;               // uptime in minutes
long  otaLastMinute = -1;            // timestamp last minute
long  WifiTryAgainMin=0;
char  wifi_ssid[20];                // Wifi SSI name
char  wifi_password[20];              // Wifi password
bool  fallBackSsi = false;          // toggle access point



//***********************************************************OTA CHECKS FUNCTION******************************************
void wifiOTAcheck(){
#ifdef OTAenable
  Serial.println("WIFI OTA Checks beginning.");

  ArduinoOTA.setHostname(OTAname);
  
  ArduinoOTA.onStart([]() {
    Serial.print("\n\nOTA Starting Update [.onStart]    - WiFi RSSI: ");
    Serial.println(WiFi.RSSI());
    //digitalWrite(PUMPPIN,HIGH); //turn on the redlight
    delay(100);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Complete.");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
    int pp=(progress / (total / 100));
    int ledSwitch = 1;
    //if (pp<80){digitalWrite(LEDSENSE,LOW);}else{digitalWrite(LEDSENSE,HIGH);}
    if ((pp%10)==0){ //on the 10's
      //things to do at 10% marks 
        if ((pp%20)==0) {ledSwitch=0;}  //flip flop it on the 20's to make an alternating light  
        digitalWrite(MQCON,ledSwitch); //turn on the redlight
    }
    if (pp%10==0){P.print("OTA%"+String(pp));}
    if (pp==100){P.print("OTA OK!");}
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("OTA Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("OTA Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("OTA Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("OTA Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("OTA End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready - OTA.begin");
  Serial.print("OTA IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("OTA Port Name: ");
  Serial.println(OTAname);
#endif
}   //end of wifiOTAcheck
//**********************************************************END OTA CHECKS FUNCTION**************************************


//*********************************************************WIFI CONNECTION FUNCTION**************************************    
void connectWifi() {                // reconnect to Wifi
  while (WiFi.status() != WL_CONNECTED and otaUpTime>=WifiTryAgainMin) {
      int i=0;
      int o=0;
      if (fallBackSsi)                  // select main or fallback access point
        {strcpy(wifi_ssid, wifi_ssid_B);
        strcpy(wifi_password, wifi_password_B);}
      else
        {strcpy(wifi_ssid, wifi_ssid_A);
        strcpy(wifi_password, wifi_password_A);}
    #ifdef DEBUG
        Serial.println();
        Serial.print("Connecting to ");
        Serial.println(wifi_ssid);
    #endif
      WiFi.begin(wifi_ssid, wifi_password);
      while ((WiFi.status() != WL_CONNECTED) && (i<20)) {
          delay(1000);
          i++;
          if ((i%2)==0){ //on the 2's
                digitalWrite(MQCON,HIGH); //turn on the redlight
          }else{
                digitalWrite(MQCON,LOW); //turn on the redlight-
          }//end mod flash LED 
    #ifdef DEBUG
          Serial.print(".");
    #endif
      } //end 20 tries loop
      fallBackSsi =!fallBackSsi;              // toggle access point
      digitalWrite(MQCON,LOW); 
      o++;
      if (o>2){WifiTryAgainMin==otaUpTime+10;}//end checks to see that it cycled the toggle AP loop 2x,if so it puts a retry min in the variable and turns on the AP, if defined
  } //end of the flip flop access point loop
  if (WiFi.status() == WL_CONNECTED){
    delay(1000);
    IP = WiFi.localIP().toString();
#ifdef STAONLY
    WiFi.enableAP(0);  //Jim Added to stop all the devices showing up on the Wifi network 3/2017
#endif
#ifdef DEBUG
    Serial.println();
    Serial.print("\nWiFi successfully connected to AP:  ");
    Serial.println(wifi_ssid);
    Serial.print("IP address: ");
    Serial.println(IP);
    Serial.print("WiFi RSSI: ");
    Serial.println(WiFi.RSSI());
#endif
    wifiOTAcheck();
    digitalWrite(MQCON,LOW); 
  }else{ //it failed to connect
#ifdef STAIFFAIL
      WiFi.enableAP(1);  //turn on the AP if a connect fail
#endif
  }//end of the connected/not connected decision
}//end of connectWifi()

void OTAdelay(int dtime){
  for(int i=0; i<dtime;i++){
    delay (100);
#ifdef OTAenable
    ArduinoOTA.handle();    
#endif
    yield();
  }
}
//*****************************************************END WIFI CONNECTION FUNCTION**************************************

//***********************************************************MAIN LOOP FUNCTION******************************************
//THIS CALL TO WIFI LOOP IS ALL THAT IS NEEDED *
//IN THE MAIN LOOP! CALL IT AND ALL IS WELL!   *
//**********************************************
void wifiLoop(int checkOTA){
  if (WiFi.status() != WL_CONNECTED) {            // Wifi connected ?
    connectWifi();
  }
#ifdef OTAenable
   if ((checkOTA) && (WiFi.status() == WL_CONNECTED)) {ArduinoOTA.handle();}
#endif
  if (otaLastMinute != (millis()/60000)) {           // another minute passed ?
    otaLastMinute = millis()/60000;
    otaUpTime++;
  }
}
//*******************************************************END MAIN LOOP FUNCTION******************************************    



