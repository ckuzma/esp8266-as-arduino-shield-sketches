/*
  Modified: Dec. 29th @ 5:16pm by Christopher Kuzma
  Original Project Found: http://hackaday.io/project/3072/instructions
  
  Modifications:
    -Designed to run on an Arduino Uno (Tested on R3)
    -ESP is using SoftwareSerial, so output is now on native
     TX and RX pins (USB connection w/Serial Monitor works)
    -Reset command has been changed, because the reply from
     the module seems to have changed between firmware revisions
    
  Notes:
    -Despite all the changes I've made, this still doesn't work well
    -Requires an ESP8266 running v0.9.2.2 firmware @ 9600 baud
    -ESP 8266 is connected on pins 11 and 12 (RX and TX), with CH-PD
     on VCC (3.3v). GPIOs left untouched (tested model ESP-01)
     
  Usage:
    -A simple demo program that connects to your WiFi network,
     establishes a TCP connection to a server, then requests
     the HTML of a desired page. Modify the SSID and PASS parameters
     below. (Changing the DEST_HOST and DEST_IP should work, too.)


*/

#include <SoftwareSerial.h>
#define dbg Serial
#define SSID        "YOUR_SSID"
#define PASS        "YOUR_PASSWORD"
#define DEST_HOST   "retro.hackaday.com"
#define DEST_IP     "192.254.235.21"
#define TIMEOUT     5000 // mS
#define CONTINUE    false
#define HALT        true

SoftwareSerial esp(11,12);

// #define ECHO_COMMANDS // Un-comment to echo AT+ commands to serial monitor

// Print error message and loop stop.
void errorHalt(String msg)
{
  dbg.println(msg);
  dbg.println("HALT");
  while(true){};
}

// Read characters from WiFi module and echo to serial until keyword occurs or timeout.
boolean echoFind(String keyword)
{
  byte current_char   = 0;
  byte keyword_length = keyword.length();
  
  // Fail if the target string has not been sent by deadline.
  long deadline = millis() + TIMEOUT;
  while(millis() < deadline)
  {
    if (esp.available())
    {
      char ch = esp.read();
      dbg.write(ch);
      if (ch == keyword[current_char])
        if (++current_char == keyword_length)
        {
          dbg.println();
          return true;
        }
    }
  }
  return false;  // Timed out
}

// Read and echo all available module output.
// (Used when we're indifferent to "OK" vs. "no change" responses or to get around firmware bugs.)
void echoFlush()
  {while(esp.available()) dbg.write(esp.read());}
  
// Echo module output until 3 newlines encountered.
// (Used when we're indifferent to "OK" vs. "no change" responses.)
void echoSkip()
{
  echoFind("\n");        // Search for nl at end of command echo
  echoFind("\n");        // Search for 2nd nl at end of response.
  echoFind("\n");        // Search for 3rd nl at end of blank line.
}

// Send a command to the module and wait for acknowledgement string
// (or flush module output if no ack specified).
// Echoes all data received to the serial monitor.
boolean echoCommand(String cmd, String ack, boolean halt_on_fail)
{
  esp.println(cmd);
  #ifdef ECHO_COMMANDS
    dbg.print("--"); dbg.println(cmd);
  #endif
  
  // If no ack response specified, skip all available module output.
  if (ack == "")
    echoSkip();
  else
    // Otherwise wait for ack.
    if (!echoFind(ack))          // timed out waiting for ack string 
      if (halt_on_fail)
        errorHalt(cmd+" failed");// Critical failure halt.
      else
        return false;            // Let the caller handle it.
  return true;                   // ack blank or ack found
}

// Connect to the specified wireless network.
boolean connectWiFi()
{
  String cmd = "AT+CWJAP=\""; cmd += SSID; cmd += "\",\""; cmd += PASS; cmd += "\"";
  if (echoCommand(cmd, "OK", CONTINUE)) // Join Access Point
  {
    dbg.println("Connected to WiFi.");
    return true;
  }
  else
  {
    dbg.println("Connection to WiFi failed.");
    return false;
  }
}

// ******** SETUP ********
void setup()  
{
  dbg.begin(9600);         // Communication with PC monitor via USB
  esp.begin(9600);        // Communication with ESP8266 via 5V/3.3V level shifter
  
  esp.setTimeout(TIMEOUT);
  dbg.println("ESP8266 HTML GET Demo (Modified)");
  
  delay(2000);

  esp.println("AT+RST");    // Manually sent reset command to ESP
  delay(3000);  // This seems to be more than enough time for a reset
  dbg.println("Module is ready.");
  echoCommand("AT+GMR", "OK", CONTINUE);   // Retrieves the firmware ID (version number) of the module. 
  echoCommand("AT+CWMODE?","OK", CONTINUE);// Get module access mode. 
  
  // echoCommand("AT+CWLAP", "OK", CONTINUE); // List available access points - DOESN't WORK FOR ME
  
  echoCommand("AT+CWMODE=1", "", HALT);    // Station mode
  echoCommand("AT+CIPMUX=1", "", HALT);    // Allow multiple connections (we'll only use the first).

  //connect to the wifi
  boolean connection_established = false;
  for(int i=0;i<5;i++)
  {
    if(connectWiFi())
    {
      connection_established = true;
      break;
    }
  }
  if (!connection_established) errorHalt("Connection failed");
  
  delay(5000);

  //echoCommand("AT+CWSAP=?", "OK", CONTINUE); // Test connection
  echoCommand("AT+CIFSR", "", HALT);         // Echo IP address. (Firmware bug - should return "OK".)
  //echoCommand("AT+CIPMUX=0", "", HALT);      // Set single connection mode
}

// ******** LOOP ********
void loop() 
{
  // Establish TCP connection
  String cmd = "AT+CIPSTART=0,\"TCP\",\""; cmd += DEST_IP; cmd += "\",80";
  if (!echoCommand(cmd, "OK", CONTINUE)) return;
  delay(2000);
  
  // Get connection status 
  if (!echoCommand("AT+CIPSTATUS", "OK", CONTINUE)) return;

  // Build HTTP request.
  cmd = "GET / HTTP/1.1\r\nHost: "; cmd += DEST_HOST; cmd += ":80\r\n\r\n";
  
  // Ready the module to receive raw data
  if (!echoCommand("AT+CIPSEND=0,"+String(cmd.length()), ">", CONTINUE))
  {
    echoCommand("AT+CIPCLOSE", "", CONTINUE);
    dbg.println("Connection timeout.");
    return;
  }
  
  // Send the raw HTTP request
  echoCommand(cmd, "OK", CONTINUE);  // GET
  
  // Loop forever echoing data received from destination server.
  while(true)
    while (esp.available())
      dbg.write(esp.read());
      
  errorHalt("ONCE ONLY");
}

