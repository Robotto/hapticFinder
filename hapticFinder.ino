/* 
* HAPTIC FINDER 
* 
* listens on port 1337 UDP for the chars: 'U' 'D' 'L' 'R' 'F' 'B' (up, down, left, right, forward, back)
* Also implements a webserver (serving on port 80) for testing.
*
* Wifi connectivity is handled by the excellent wifiManager library from https://github.com/tzapu/WiFiManager
* Supports OTA firmware flashing in case the USB somehow becomes unavailable.
*
* Pinout for the 8 vibrators is as follows:
*                       ([1]->D0)
*
*                       ([2]->D5)
*
* ([3]->D6)     ([4]->D7)       ([5]->D2)       ([6]->D1)
*
*                       ([7]->D4)
*
*                       ([8]->D3)
*
* Hold on to your butts...
*
*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoOTA.h>

//motor pins:
#define NUM_VIBRATORS 8
static unsigned int v1=D0;
static unsigned int v2=D5;
static unsigned int v3=D6;
static unsigned int v4=D7;
static unsigned int v5=D2;
static unsigned int v6=D1;
static unsigned int v7=D4;
static unsigned int v8=D3;

static unsigned int vibrators[NUM_VIBRATORS]={v1,v2,v3,v4,v5,v6,v7,v8};

WiFiUDP Udp;
const unsigned int localPort = 1337;    // local port to listen for UDP packets
const int UDP_PACKET_SIZE = 1; //change to whatever you need.
byte packetBuffer[ UDP_PACKET_SIZE ]; //buffer to hold outgoing packets

ESP8266WebServer webserver(80);

void configModeCallback (WiFiManager *myWiFiManager) {
    Serial.println("Entered config mode");
    Serial.println("AP: " + myWiFiManager->getConfigPortalSSID());
    Serial.println("IP: " + WiFi.softAPIP().toString());
  }
  
void setup()
{
    for(int i=0; i<NUM_VIBRATORS; i++){
        pinMode(vibrators[i],OUTPUT);
        digitalWrite(vibrators[i],LOW);
    }
    
    WiFi.hostname("HapticFinder");
    
    Serial.begin(115200);
    Serial.println("Hello!");
    
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    Serial.println("Connecting to wifi..");
    wifiManager.setAPCallback(configModeCallback); //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wifiManager.setConnectTimeout(30); //try to connect to known wifis for a long time before defaulting to AP mode
    
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "HapticFinder"
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("HapticFinder")) { Serial.println("failed to connect and hit timeout"); ESP.restart(); }
  
    Serial.println("Wifi Up! IPV4: " + WiFi.localIP().toString());
    //OTA:
    // ArduinoOTA.setPort(8266);
    // Port defaults to 8266
    // Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setHostname("HapticFinder");// No authentication by default
    ArduinoOTA.onStart([]() { Serial.println("OTA START!"); delay(500); });
    ArduinoOTA.onEnd([]() { Serial.println("OTA End.. brace for reset"); ESP.restart(); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error) { String buffer=String("Error[") + String(error) + String("]: ");  if (error == OTA_AUTH_ERROR) buffer+=String("Auth Failed"); else if (error == OTA_BEGIN_ERROR) buffer+=String("Begin Failed"); else if (error == OTA_CONNECT_ERROR) buffer+=String("Connect Failed"); else if (error == OTA_RECEIVE_ERROR) buffer+=String("Receive Failed"); else if (error == OTA_END_ERROR) buffer+=String("End Failed"); Serial.println(buffer); });
    ArduinoOTA.begin();

    webserver.on("/", handleRoot);
    webserver.on("/move", handleMove);
    
    webserver.onNotFound(handleNotFound);
    webserver.begin();

    Serial.println("HTTP server started");

    Udp.begin(localPort);
    
	
}

void loop()
{
    if (Udp.parsePacket()) UDPrx();
    
    webserver.handleClient();
    
    ArduinoOTA.handle();
    
    yield();
        
}


//http stuff:
void handleRoot() {
    Serial.println("client trying to access /");
    //server.send(200, "text/plain", "hello from esp8266!");
    webserver.sendContent("HTTP/1.1 200 OK\r\n"); //send new p\r\nage
    webserver.sendContent("Content-Type: text/html\r\n");
    webserver.sendContent("\r\n");
    webserver.sendContent("<HTML>\r\n");
    webserver.sendContent("<HEAD>\r\n");
    webserver.sendContent("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"); //zoom to width of window
    webserver.sendContent("<meta name='apple-mobile-web-app-capable' content='yes' />\r\n");
    webserver.sendContent("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />\r\n");
    webserver.sendContent("<link rel='stylesheet' type='text/css' href='https://moore.dk/doorcss.css' />\r\n"); //External CSS
    webserver.sendContent("<TITLE>Touch it.</TITLE>\r\n");
    webserver.sendContent("</HEAD>\r\n");
    webserver.sendContent("<BODY>\r\n");
    webserver.sendContent("<H1>HapticFinder</H1>\r\n");
    webserver.sendContent("<hr />\r\n");
    webserver.sendContent("<br />\r\n");
    webserver.sendContent("<a href=\"/move?dir=U\"\">UP</a>\r\n");
    webserver.sendContent("<br />\r\n");
    webserver.sendContent("<br />\r\n");    
    webserver.sendContent("<a href=\"/move?dir=L\"\">LEFT</a>    <a href=\"/move?dir=R\"\">RIGHT</a>\r\n");
    webserver.sendContent("<br />\r\n");
    webserver.sendContent("<br />\r\n");    
    webserver.sendContent("<a href=\"/move?dir=D\"\">DOWN</a>\r\n");
    webserver.sendContent("<br />\r\n");
    webserver.sendContent("<br />\r\n");    
    webserver.sendContent("<br />\r\n");
    webserver.sendContent("<br />\r\n");    
    webserver.sendContent("<a class=\"red\" href=\"/move?dir=F\"\">FORWARD</a>   <a class=\"red\" href=\"/move?dir=B\"\">BACK</a>\r\n");
    
    webserver.sendContent("</BODY>\r\n");
    webserver.sendContent("</HTML>\r\n");
  
  
  }


  void handleMove(){
    
      Serial.println("Arg: " + webserver.argName(0) + ": " + webserver.arg(0));
        if(webserver.arg(0)=="U") up();
        if(webserver.arg(0)=="D") down();
        if(webserver.arg(0)=="L") left();
        if(webserver.arg(0)=="R") right();
        if(webserver.arg(0)=="F") forward();
        if(webserver.arg(0)=="B") back();
        webserver.sendContent("HTTP/1.1 204 No Content\r\n\r\n");
    }
  
void up(){Serial.println("UP");}  
void down(){Serial.println("DOWN");}
void left(){Serial.println("LEFT");}
void right(){Serial.println("RIGHT");}
void forward(){Serial.println("FORWARD");}
void back(){Serial.println("BACK");}  
  
  void handleNotFound(){
    Serial.println("client getting 404");
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += webserver.uri();
    message += "\nMethod: ";
    message += (webserver.method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += webserver.args();
    message += "\n";
    for (uint8_t i=0; i<webserver.args(); i++){
      message += " " + webserver.argName(i) + ": " + webserver.arg(i) + "\n";
    }
    webserver.send(404, "text/plain", message);
  }
  
  void UDPrx()
  {
  // We've received a packet, read the data from it
      Serial.println("udpRX!");
      memset(packetBuffer, 0, UDP_PACKET_SIZE); //reset packet buffer
      int read_bytes=Udp.read(packetBuffer,UDP_PACKET_SIZE);  // read the packet into the buffer
  
      switch(packetBuffer[0])
      {
          case 'U':
            up();
            break;
          case 'D':
            down();
            break;
          case 'L':
            left();
            break;
          case 'R':
            right();
            break;
          case 'F':
            forward();
            break;
          case 'B':
            back();
            break;
          default:
            Serial.println("UDP Parsing failed. Someone sent a " + String((char)packetBuffer[0]));
            break;
      }
  }
  