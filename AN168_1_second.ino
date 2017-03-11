/*
 * Chad Baily
 * CO2 Meter, Inc.
 * AN168_1_second
 * 3/10/2017
 * 
 */
 
//Some code for switching between modes
// was found on http://forum.43oh.com/topic/9707-switching-the-cc3200-from-ap-to-station-mode/
#include <SPI.h>
#include <WiFi.h>
#include <SLFS.h>
#define AP_MODE  0
#define STA_MODE 1
#include <udma_if.h>

uint8_t mode = 0;
const char ssid[] = "CC3200";
const char wifipw[] = "password";
const char s[2] = "-";
char *token;
String y = "";
char Rssid[100], Rpsk[100];
unsigned int Port = 2390;
char packetBuffer[255];
IPAddress Ip(255, 255, 255, 255);
char b[1024];
int j = 0;

WiFiUDP Udp;

// Code for s8 Server

byte readCO2[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25}; //Command 8 byte command to read RAM CRC = 0x259F
/* To read EEPROM the following command works:
  byte readCO2[] = {0xFE, 0X46, 0X00, 0X08, 0X02, 0X9e, 0X9d}; //Command 8 byte command to read EEPROM
  The CRC bytes 0x9D9E were calculated using the link above.
*/
// 0xFE  initiate a MODBUS command
// 0x44 = S8 Read internal RAM command
// 0x00 = RAM address H, 0x08 =  RAM address L
// 0x02 = Number of bytes to read
// 0x9F =  CRC LS byte,  0x25= CRC MS byte

// Additonal S8 commands:  0x41 =  Write S8 RAM,  0x43 = Write S8 EEPROM,  0x46 = Read S8 EEPROM

byte response[] = {0, 0, 0, 0, 0, 0, 0}; //create an array to store the 7 byte response
//multiplier for value. default is 1. set to 3 for K-30 3% and 10 for K-33 ICB
int valMultiplier = 1; //For 1% sensor P/N SE-119 marked 004-0-0013
//int valMultiplier = 5;   //For 5% sensor P/N SE-037 marked 004-0-0017

WiFiServer server(80);

void setup()
{
  Serial.begin(115200);
  SerFlash.begin();
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(PUSH2, INPUT_PULLUP);
  if (WiFi.begin() != WL_CONNECT_FAILED)
  {
    Serial.println("Connecting to stored profile");
    Serial.print("Connected to SSID: ");
    Serial.println(WiFi.SSID());

    int var = 0; // variable to timeout the connection process if it is not working. Then alerts the user to start AP MODE
    while (WiFi.localIP() == INADDR_NONE)
    {
      delay(500);
      digitalWrite(RED_LED, HIGH);
      digitalWrite(YELLOW_LED, HIGH);
      digitalWrite(GREEN_LED, HIGH);
      var++;
      if (var >= 30)
      {
        mode = AP_MODE;
        var = 0;
        digitalWrite(RED_LED, LOW);
        digitalWrite(YELLOW_LED, LOW);
        digitalWrite(GREEN_LED, LOW);
        break;
      }
      // print dots while we wait for an ip addresss
      Serial.print(".");
      delay(500);
      digitalWrite(RED_LED, LOW);
      digitalWrite(YELLOW_LED, LOW);
      digitalWrite(GREEN_LED, LOW);
    }

    //letting the use know that the device is not connected
    if (WiFi.localIP() == INADDR_NONE)
    {
      digitalWrite(RED_LED, LOW);
      digitalWrite(YELLOW_LED, LOW);
      Serial.println("\n\nDEVICE NOT CONNECTED TO THE INTERNET. PLEASE LAUNCH AP MODE\n\n");
    }

    else
    {
      Serial.println("\nIP Address obtained");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
    }

    Serial1.begin(9600); //Opens the virtual serial port with a baud of 9600
    server.begin();
    mode = STA_MODE;
  }
}


void loop()
{
  if ( digitalRead(PUSH2))
  {
    Serial.println("Starting AP Mode");
    mode = AP_MODE;
    Serial.print("Setting up Access Point named: ");
    Serial.println(ssid);
    Serial.print("AP uses WPA and password is: ");
    Serial.println(wifipw);

    WiFi.beginNetwork((char *)ssid, (char *)wifipw);

    while (WiFi.localIP() == INADDR_NONE)
    {
      // print dots while we wait for the AP config to complete
      Serial.print('.');
      delay(300);
    }
    Serial.println();
    Serial.println("AP active.");
    printWifiStatus();
    digitalWrite(RED_LED, HIGH);
  }
  if (mode == AP_MODE)
  {
    ap_loop();
  }
  if (mode == STA_MODE)
  {
    sta_loop();
  }
}

void sta_loop()
{
  delay(1000);
  sendRequest(readCO2);
  unsigned long valCO2 = getValue(response);
  Serial.print("Co2 ppm = ");
  Serial.println(valCO2);
  //Wifi Server Side
  int i = 0;
  WiFiClient client = server.available();   // listen for incoming clients
  if (client)
  {
    char buffer[150] = {0};
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (strlen(buffer) == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            // the content of the HTTP response follows the header:
            client.println("<html><head><title>Energia CC3200 WiFi Web Server</title>");
            client.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/1.3.0/jquery.min.js\"></script>");
            client.println("</head><body align=center>");
            client.println("<h1 align=center><font color=\"red\">S8 CO2 Sensor</font></h1>");
            client.print("<h2 id = \"co2\" align=center><font color=\"red\">");
            client.print(valCO2);
            client.println("</font></h2>");
            client.println("<span></span>");
            client.println("<div id = \"content\"></div>");
            client.println("<script> setInterval(function(){ $('#co2').load(document.URL +  ' #co2');}, 300); </script>");
            // The HTTP response ends with another blank line:
            client.println();
            break;
          }
          else {      // if you got a newline, then clear the buffer:
            memset(buffer, 0, 150);
            i = 0;
          }
        }
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          buffer[i++] = c;      // add it to the end of the currentLine
        }
      }
    }
    // close the connection:
    client.stop();
  }
}

void sendRequest(byte packet[])
{
  while (!Serial1.available()) //keep sending request until we start to get a response
  {
    Serial1.write(readCO2, 7);
    delay(50);
  }

  int timeout = 0; //set a timeoute counter
  while (Serial1.available() < 7 ) //Wait to get a 7 byte response
  {
    timeout++;
    if (timeout > 10) //if it takes to long there was probably an error
    {
      while (Serial1.available()) //flush whatever we have
        Serial1.read();
      break; //exit and try again
    }
    delay(50);
  }

  for (int i = 0; i < 7; i++)
  {
    response[i] = Serial1.read();
  }
}
unsigned long getValue(byte packet[])
{
  int high = packet[3]; //high byte for value is 4th byte in packet in the packet
  int low = packet[4]; //low byte for value is 5th byte in the packet
  unsigned long val = high * 256 + low; //Combine high byte and low byte with this formula to get value
  return val * valMultiplier;
}

void ap_loop()
{ Udp.begin(Port);
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    int len = Udp.read(packetBuffer, 255);
    if (len > 0)packetBuffer[len] = 0;
    //unit=atoi(packetBuffer);
    Serial.println(packetBuffer);

    /* get the first token */
    token = strtok(packetBuffer, s);
    int w = 0;

    while ( token != NULL )
    {
      if (w == 0)
      {
        strcpy(Rssid, token);
        Serial.println(Rssid);
        w++;
      }
      else if (w == 1)
      {
        strcpy(Rpsk, token);
        Serial.println(Rpsk);
      }
      token = strtok(NULL, s);
    }

    /*int retVal;
      retVal = sl_WlanSetMode(ROLE_STA);
      retVal = sl_Stop(0);
      retVal= sl_Start(NULL, NULL, NULL);*/

    UDMAInit();
    sl_Start(NULL, NULL, NULL);

    sl_WlanDisconnect();

    sl_NetAppMDNSUnRegisterService(0, 0);

    sl_WlanRxStatStart();

    sl_WlanSetMode(ROLE_STA);

    // Restart Network processor
    sl_Stop(30);

    sl_Start(NULL, NULL, NULL);

    WiFi._initialized = false;
    WiFi._connecting = false;
    // attempt to connect to Wifi network:
    Serial.print("Attempting to connect to Network named: ");
    delay(1000);
    // print the network name (SSID);
    Serial.println(Rssid);
    //WiFi._initialized = false;
    //WiFi._connecting = false;
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    WiFi.begin(Rssid, Rpsk);
    while ( WiFi.status() != WL_CONNECTED)
    {
      // print dots while we wait to connect
      Serial.print(".");
      delay(300);
    }

    Serial.println("\nYou're connected to the network");
    Serial.println("Waiting for an ip address");

    while (WiFi.localIP() == INADDR_NONE)
    {
      // print dots while we wait for an ip addresss
      Serial.print(".");
      delay(300);
    }

    Serial.println("\nIP Address obtained");

    printWifiStatus();
    mode = STA_MODE;
  }
}

void printWifiStatus()
{
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  sprintf(b, "%d.%d.%d.%d,", ip[0], ip[1], ip[2], ip[3] );
  Serial.print("IP Address: ");
  Serial.println(ip);
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
