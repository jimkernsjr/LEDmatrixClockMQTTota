////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                  NTP Time Handler                                                                                          //
//                                                                                                                            //
// Jim  01/01/2018                                                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                            //
//                                                                                                                            //
//                                                                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/*
 * TimeNTP_ESP8266WiFi.ino
 * Example showing time sync to NTP time source
 *
*/
#include <TimeLib.h>
#include <WiFiUdp.h>
#define countof(a) (sizeof(a) / sizeof(a[0]))

#ifndef RESYNCMINS
  #define RESYNCMINS 30   //default if not overloaded
#endif

bool updatePermitted=true;
String NTPfulltime="";    //With Seconds
String NTPfulltimeNS="";  //No Seconds
String NTPm_d_yr="";
String NTPmiltime="";
int timeSinceSync=0;
long  NTPlastMinute = -1;            // timestamp last minute

// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";

//const int timeZone = 1;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)

String str="";
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);


void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
 
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  if (updatePermitted){
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
  }
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
if (updatePermitted){
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
}

void printDateTime()
{
//    char datestring[20];
//
//    snprintf_P(datestring, 
//            countof(datestring),
//            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
//            dt.Month(),
//            dt.Day(),
//            dt.Year(),
//            dt.Hour(),
//            dt.Minute(),
//            dt.Second() );
//    Serial.print(datestring);
}

void getNetTime(){
  Serial.print("Getting Net Time...");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("...connection FAIL.");
    return;
  }
  Serial.println("Connection OK. ");
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);    // Set the time using NTP
  //setSyncInterval(300);
  if(timeStatus() != timeNotSet){
    digitalClockDisplay();
    Serial.println("here is another way to display");
      time_t t = now();
      char d_mon_yr[12];
      snprintf_P(d_mon_yr, countof(d_mon_yr), PSTR("%s %02u %04u"), monthShortStr(month(t)), day(t), year(t)); // get month, day and year 
      Serial.println(d_mon_yr);
      char tim_set[9];
      snprintf_P(tim_set, countof(tim_set), PSTR("%02u:%02u:%02u"), hour(t), minute(t), second(t)); // get time 
      Serial.println(tim_set);
      Serial.println("Done.");

  }
}





//////////////////////////////////////////////
void setTimeStrings(){
    time_t t = now();
    char buff1[14];
    sprintf(buff1,"%02u:%02u", hour(t), minute(t)); // get time 
    //sprintf(NTPfulltimeNS,buff1[10]);
    NTPfulltimeNS=String((char*)buff1);
    snprintf_P(buff1, countof(buff1), PSTR("%02u:%02u:%02u"), hour(t), minute(t), second(t)); // get time 
    NTPfulltime=String((char*)buff1);
    snprintf_P(buff1, countof(buff1), PSTR("%s %02u %04u"), monthShortStr(month(t)), day(t), year(t)); // get month, day and year 
    NTPm_d_yr=String((char*)buff1);

}

void handleTime(int resyncMins){
    if (NTPlastMinute != (millis()/60000)) {           // another minute passed ?
        NTPlastMinute = millis()/60000;
        timeSinceSync++;
        
        if ((timeSinceSync>=(resyncMins*1000)) || (needTsync) ){
          getNetTime();
          timeSinceSync=0;
          needTsync=false;
        }else{
          //////Do the updating here
          //no need to do anything with this new code.
          
          
        }
    } 
    setTimeStrings();
}

void handleTime(){
  handleTime(RESYNCMINS);
}








