//kompilovat jako Generic ESP8266 Module

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <WiFiManager.h> 

//for LED status
#include <Ticker.h>
Ticker ticker;

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

#define ONE_WIRE_BUS 4 //IO2
#define TEMPERATURE_PRECISION 12
OneWire onewire(ONE_WIRE_BUS); // pin for onewire DALLAS bus
DallasTemperature dsSensors(&onewire);
DeviceAddress tempDeviceAddress;
#define NUMBER_OF_DEVICES 7

DeviceAddress tempDeviceAddresses[NUMBER_OF_DEVICES];
//int  resolution = 12;
unsigned int numberOfDevices; // Number of temperature devices found
float sensor[NUMBER_OF_DEVICES];

unsigned int const dsMeassureDelay        = 750; //delay between start meassurement and read valid data in ms

unsigned long milisLastRunMinOld          = 0;

#define verbose
#ifdef verbose
 #define DEBUG_PRINT(x)         Serial.print (x)
 #define DEBUG_PRINTDEC(x)      Serial.print (x, DEC)
 #define DEBUG_PRINTHEX(x)      Serial.print (x, HEX)
 #define DEBUG_PRINTLN(x)       Serial.println (x)
 #define DEBUG_PRINTF(x, y)     Serial.printf (x, y)
 #define PORTSPEED 115200
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTDEC(x)
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINTF(x, y)
#endif 

ESP8266WebServer server(80);

#define AIO_SERVER      "192.168.1.56"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "datel"
#define AIO_KEY         "hanka12"

WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

unsigned int const sendTimeDelay            = 300000;
signed long lastSendTime = sendTimeDelay *    -1;

/****************************** Feeds ***************************************/
Adafruit_MQTT_Publish temperaturesLivingRoom = Adafruit_MQTT_Publish(&mqtt,  "/home/bedNew/esp03/tLivingRoom");
Adafruit_MQTT_Publish temperaturesBedRoomNew = Adafruit_MQTT_Publish(&mqtt,  "/home/bedNew/esp03/tBedRoomNew");
Adafruit_MQTT_Publish temperaturesBedRoomOld = Adafruit_MQTT_Publish(&mqtt,  "/home/bedNew/esp03/tBedRoomOld");
Adafruit_MQTT_Publish temperaturesCorridor = Adafruit_MQTT_Publish(&mqtt,  "/home/bedNew/esp03/tCorridor");
Adafruit_MQTT_Publish temperaturesHall = Adafruit_MQTT_Publish(&mqtt,  "/home/bedNew/esp03/tHall");
Adafruit_MQTT_Publish temperaturesBath = Adafruit_MQTT_Publish(&mqtt,  "/home/bedNew/esp03/tBath");
Adafruit_MQTT_Publish temperaturesWorkRoom = Adafruit_MQTT_Publish(&mqtt,  "/home/bedNew/esp03/tWorkRoom");
Adafruit_MQTT_Publish temperaturesAttic = Adafruit_MQTT_Publish(&mqtt,  "/home/bedNew/esp03/tAttic");
Adafruit_MQTT_Publish verSW = Adafruit_MQTT_Publish(&mqtt,  "/home/bedNew/esp03/VersionSW");
Adafruit_MQTT_Publish hb= Adafruit_MQTT_Publish(&mqtt,  "/home/bedNew/esp03/HeartBeat");

// Setup a feed called 'onoff' for subscribing to changes.
//Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");

void MQTT_connect(void);

IPAddress _ip           = IPAddress(192, 168, 1, 110);
IPAddress _gw           = IPAddress(192, 168, 1, 1);
IPAddress _sn           = IPAddress(255, 255, 255, 0);

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

float versionSW                   = 0.6;
String versionSWString            = "Teploty v";
uint32_t heartBeat                = 0;


void handleRoot()
{
  mereni();
  
	char temp[450];
	int sec = millis() / 1000;
	int min = sec / 60;
	int hr = min / 60;

	snprintf(temp, 450,
		"<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 Temperatures</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Teploty chata</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
	<p>Teplota venku: %02d.%01d&deg;C</p>\
	<p>Teplota doma: %02d.%01d&deg;C</p>\
  </body>\
</html>",

hr, min % 60, sec % 60, sensor[0], sensor[1]
);
	server.send(200, "text/html", temp);
}

void handleNotFound() {
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += (server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";

	for (uint8_t i = 0; i < server.args(); i++) {
		message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
	}

	server.send(404, "text/plain", message);
}



void setup() {
#ifdef verbose
  Serial.begin(PORTSPEED);
#endif
  DEBUG_PRINTLN();
  DEBUG_PRINT(versionSWString);
  DEBUG_PRINTLN(versionSW);
  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);
  
  dsInit();

  DEBUG_PRINTLN(ESP.getResetReason());
  if (ESP.getResetReason()=="Software/System restart") {
    heartBeat=1;
  } else if (ESP.getResetReason()=="Power on") {
    heartBeat=2;
  } else if (ESP.getResetReason()=="External System") {
    heartBeat=3;
  } else if (ESP.getResetReason()=="Hardware Watchdog") {
    heartBeat=4;
  } else if (ESP.getResetReason()=="Exception") {
    heartBeat=5;
  } else if (ESP.getResetReason()=="Software Watchdog") {
    heartBeat=6;
  } else if (ESP.getResetReason()=="Deep-Sleep Wake") {
    heartBeat=7;
  }

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  
  //Serial.println(ESP.getFlashChipRealSize);
  //Serial.println(ESP.getCpuFreqMHz);
  //WiFi.begin(ssid, password);
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  
  if (!wifiManager.autoConnect("Anemometer", "password")) {
    DEBUG_PRINTLN("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

	DEBUG_PRINTLN("");
	DEBUG_PRINT("Connected to ");
	DEBUG_PRINT("IP address: ");
	DEBUG_PRINTLN(WiFi.localIP());
  
  server.on("/", handleRoot);
	server.on("/inline", []() {
		server.send(200, "text/plain", "this works as well");
	});
	server.onNotFound(handleNotFound);
	server.begin();
	DEBUG_PRINTLN("HTTP server started");
  
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);
}

uint32_t x=0;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // Adafruit_MQTT_Subscribe *subscription;
  // while ((subscription = mqtt.readSubscription(5000))) {
    // if (subscription == &onoffbutton) {
      // DEBUG_PRINT(F("Got: "));
      // DEBUG_PRINTLN((char *)onoffbutton.lastread);
    // }
  // }

 if (millis() - lastSendTime >= sendTimeDelay) {
    lastSendTime = millis();
    
    mereni();
  }
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */

  server.handleClient();
}

void dsInit(void) {
  dsSensors.begin();
  numberOfDevices = dsSensors.getDeviceCount();
  DEBUG_PRINT("Found ");
  DEBUG_PRINT(numberOfDevices);
  DEBUG_PRINT(" DS18B20 device(s).");

  // Loop through each device, print out address
  for (byte i=0;i<numberOfDevices; i++) {
      // Search the wire for address
    if (dsSensors.getAddress(tempDeviceAddress, i)) {
      memcpy(tempDeviceAddresses[i],tempDeviceAddress,8);
      //DEBUG_PRINTLN(tempDeviceAddresses[i]);
    }
    else
    {
      //DEBUG_PRINTLN("Unable to get device address for sensor " + i);
    }
  }
  dsSensors.setResolution(12);
  dsSensors.setWaitForConversion(false);
}


void mereni() {
  dsSensors.requestTemperatures(); 
  delay(dsMeassureDelay);
  for (byte i=0;i<numberOfDevices; i++) {
    float tempTemp=-126;
    for (byte j=0;j<10;j++) {
      tempTemp = dsSensors.getTempC(tempDeviceAddresses[i]);
      if (tempTemp>=-55) {
        break;
      }
    }
    sensor[i]=tempTemp;
    DEBUG_PRINT("Sensor ");
    DEBUG_PRINT(i);
    DEBUG_PRINT(" (");
    if (dsSensors.getAddress(tempDeviceAddress, i)) {
      for (byte j=0; j<8; j++) {
        if (tempDeviceAddress[j] < 16) {
          DEBUG_PRINT(0);
        }
        DEBUG_PRINTHEX(tempDeviceAddress[j]);
        if (j<7) DEBUG_PRINT(".");
      }
    }
    DEBUG_PRINT(") - ");
    DEBUG_PRINT(sensor[i]);
    DEBUG_PRINTLN(" C");
  }
  
  if (millis() - milisLastRunMinOld > 60000) {
    milisLastRunMinOld = millis();
    if (! hb.publish(heartBeat)) {
      DEBUG_PRINTLN("Send HB failed");
    } else {
      DEBUG_PRINTLN("Send HB OK!");
    }
    heartBeat++;
    if (! verSW.publish(versionSW)) {
      DEBUG_PRINT(F("Send verSW failed!"));
    } else {
      DEBUG_PRINT(F("Send verSW OK!"));
    }
  }

  
  for (byte i=0; i<numberOfDevices; i++) {
    DEBUG_PRINT("Send temperature ");
    DEBUG_PRINT(sensor[i]);
    DEBUG_PRINT(" of sensor ");
    DEBUG_PRINT(i);
    DEBUG_PRINT(" to MQTT broker, topic ");
    DEBUG_PRINT(" return  ");
    if (i==0) {
      if (! temperaturesLivingRoom.publish(sensor[i])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
    }
    if (i==1) {
      if (! temperaturesAttic.publish(sensor[i])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
    }
    if (i==2) {
      if (! temperaturesBath.publish(sensor[i])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
    }
    if (i==3) {
      if (! temperaturesBedRoomNew.publish(sensor[i])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
    }
    if (i==4) {
      if (! temperaturesBedRoomOld.publish(sensor[i])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
    }
    if (i==5) {
      if (! temperaturesHall.publish(sensor[i])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
    }
    if (i==6) {
      if (! temperaturesWorkRoom.publish(sensor[i])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
    }
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  DEBUG_PRINT("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       DEBUG_PRINTLN(mqtt.connectErrorString(ret));
       DEBUG_PRINTLN("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  DEBUG_PRINTLN("MQTT Connected!");
}