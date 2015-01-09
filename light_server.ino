/*
CHANGELOG:

Dec 31, 2014 @ 4:47p:
Updates: Now simplified! Roundtrip command now takes only about 5 seconds to be completed.
All of the original borrowed code has been rewritten to... um... work. Going to test now.
Happy New Years!

Jan 5, 2015 @ 9:15p:
Seems to work just fine.

Jan 9, 2015 @ 4:49p:
Implemented improvements by Dir (http://www.esp8266.com/memberlist.php?mode=viewprofile&u=4235),
and modified the setup routine to allow for a slower WiFi handshake when joining an AP. Should
result in a much less buggy setup.

------

Wiring Setup (Arduino UNO):
- Pins 11 and 12 to ESP8266 (running at baudrate of 9600) connected via SoftwareSerial
- 3.3v and GND to ESP8266 (DO NOT USE 5v!!!!!)
- Pins 2-10 are setup to be toggled high/low (e.g. relay switches, LEDs, etc.)
- Pins 0 and 1 are left untouched for USB/Serial debugging (will print out its IP address here)
- Pin 13 is being used for the onboard LED for basic status information (to be updated later)

How To Use:
- Fill in the Strings below to set your WiFi network details for connection
- Load onto Arduino
- Remove all power from Arduino to properly reset ESP8266
- Return power to Arduino, wait for the LED on pin 13 to illuminate in a solid manner
- Send HTTP requests to the IP address of your Arduino as follows:
  - http://x.x.x.x/?command=0000E
  - Translation: set the first four pins (2-5) to LOW
  - Maximum command length: 9 (e.g. 001100110)
- If using a browser or cURL request, the data returned will be a JSON object reporting
  the current HIGH/LOW status of all 9 controllable pins
*/


#include <SoftwareSerial.h>
#define BUFFER_SIZE 512
#define GET_SIZE 64
#define dbg Serial  // USB local debug
SoftwareSerial esp(11,12);
String ssid = "";
String pass = "";
String serverPort = "80";

char buffer[BUFFER_SIZE]; // Don't touch
char get_s[GET_SIZE];
char OKrn[] = "OK\r\n"; // Don't touch

String currentCommand = "000000000";


byte waitForEsp(int timeout, char* term=OKrn) {
  unsigned long t=millis();
  bool found=false;
  int i=0;
  int len=strlen(term);
  while(millis()<t+timeout) {
    if(esp.available()) {
      buffer[i++]=esp.read();
      if(i>=len) {
        if(strncmp(buffer+i-len, term, len)==0) {
          found=true;
          break;
        }
      }
    }
  }
  buffer[i]=0;
  dbg.print(buffer);
  return found;
}


// ##################
// ## Setup & Loop ##
// ##################
void setup() {
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10,OUTPUT);
  pinMode(13, OUTPUT); // For debugging purposes w/o USB
  // Set baud rates
  digitalWrite(13,HIGH);
  esp.begin(9600);
  dbg.begin(9600);
  dbg.println("DEBUG: Running Setup");
  // Reset ESP, Test, Configure, Connect, Start Server
  esp.println("AT+RST"); // Reset
  waitForEsp(4000);
  esp.println("AT"); // Test
  waitForEsp(2000);
  esp.println("AT+CWMODE=1"); // Set to client mode
  waitForEsp(2000);
  esp.println("AT+CWJAP=\""+ssid+"\",\""+pass+"\""); // Join AP
  waitForEsp(6000);
  esp.println("AT+CIPMUX=1"); // Allow multiple connections
  waitForEsp(2000);
  esp.println("AT+CIPSERVER=1,"+serverPort); // Start server on port
  waitForEsp(2000);
  esp.println("AT+CIFSR");
  waitForEsp(1000);
  dbg.println("DEBUG: Setup complete\n\n");
  digitalWrite(13,LOW);
}
void loop() {
  updateLights(currentCommand);
  // BEGIN - Borrowed Code
  int ch_id, packet_len, lssid = 0, lpass = 0;
  char *pb;  
  if(readTillEnd()) {
    if(strncmp(buffer, "+IPD,", 5)==0) {
      sscanf(buffer+5, "%d,%d", &ch_id, &packet_len);
      if (packet_len > 0) {
        pb = buffer+5;
        while(*pb!=':') pb++;
        pb++;
        incomingRequest(pb, ch_id);
      }
    }
  }
  // END - Borrowed Code
}
// ##################
// ## Setup & Loop ##
// ##################


void incomingRequest(char *pb, int ch_id){
  char *pget;
  if (strncmp(pb, "GET /favicon.ico", 16) == 0) {
    pb = pb+6;
    waitForEsp(1000);
  }
  if (strncmp(pb, "GET /?", 6) == 0) {
    pb = pb+6;
    pget = get_s;
    while(*pb!=' '){
      *pget = *pb;
      pb++; 
      pget++;
    }
    extractCommand();
    waitForEsp(1000);
    dbg.println("-> serve homepage (Would've!)\n");
    dbg.println("Command received: "+currentCommand);
    serveReply(ch_id,currentCommand);
  }
}

void serveReply(int ch_id,String received){
  String reply = "{current_command:"+received+"}";
  esp.println("AT+CIPSEND="+String(ch_id)+","+String(reply.length())); // change 18 to reply length
  waitForEsp(2000,">");
  dbg.println("Sending back a response");
  esp.print(reply);
  waitForEsp(2000);
  dbg.println("Closing connection");
  esp.println("AT+CIPCLOSE="+String(ch_id));
  waitForEsp(2000);
}

void extractCommand(){
  String url_command ="";
  char *pget;
  pget = get_s;
  pget = pget+8; //len("?command")+1
  while(*pget!='E'){ // Terminating thing on end of string (ex. 010011E)
    url_command += *pget;
    pget++; 
  }
  waitForEsp(2000);
  currentCommand = url_command;
}
bool readTillEnd() {
  static int i=0;
  if(esp.available()) {
    buffer[i++]=esp.read();
    if(i==BUFFER_SIZE)  i=0;
    if(i>1 && buffer[i-2]==13 && buffer[i-1]==10) {
      buffer[i]=0;
      i=0;
      dbg.print(buffer);
      return true;
    }
  }
  return false;
}

void updateLights(String inbound){
  for(int x = 0; x < inbound.length(); x++){
    int powerValue = inbound[x] - '0';
    if (powerValue == 1){
      ledOn(x);
    }
    if (powerValue == 0){
      ledOff(x);
    }
  }
}


void ledOn(int number){
  digitalWrite(number+2, HIGH);
}

void ledOff(int number){
  digitalWrite(number+2, LOW);
}
