#include "Configuration.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <WiFiManager.h> 

#define verbose
#ifdef verbose
 #define DEBUG_PRINT(x)         Serial.print (x)
 #define DEBUG_PRINTDEC(x)      Serial.print (x, DEC)
 #define DEBUG_PRINTLN(x)       Serial.println (x)
 #define DEBUG_PRINTF(x, y)     Serial.printf (x, y)
 #define PORTSPEED 115200
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTDEC(x)
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINTF(x, y)
#endif 

WiFiManager wifiManager;
  
OneWire onewire(ONE_WIRE_BUS); // pin for onewire DALLAS bus
DallasTemperature dsSensor(&onewire);
//DeviceAddress tempDeviceAddress;

//DeviceAddress tempDeviceAddresses[NUMBER_OF_DEVICES];
//unsigned int numberOfDevices; // Number of temperature devices found
//float sensor[NUMBER_OF_DEVICES];
float teplota = 0;

unsigned int const dsMeassureDelay=750; //delay between start meassurement and read valid data in ms

ESP8266WebServer server(80);

WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

unsigned int const sendTimeDelay=30000;
signed long lastSendTime = sendTimeDelay * -1;

/****************************** Feeds ***************************************/
Adafruit_MQTT_Publish _teplota = Adafruit_MQTT_Publish(&mqtt,  "/home/bedNew/esp03T/tLivingRoom");


void MQTT_connect(void);

void handleRoot()
{
  mereni();
  
  DEBUG_PRINTLN("Client request");
  digitalWrite(LED_BUILTIN, LOW);
  
	char temp[450];
	int sec = millis() / 1000;
	int min = sec / 60;
	int hr = min / 60;

	snprintf(temp, 450,
		"<html>\
  <head>\
    <meta http-equiv='Content-Type' content='text/html; charset=utf-8' />\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 Temperatures</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Teplota obývák</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <p>Teplota: %02d.%01d&deg;C</p>\
  </body>\
</html>",

  hr, min % 60, sec % 60, 
  (int)(teplota/TEMPERATURE_DIVIDOR), (abs((int)teplota))%TEMPERATURE_DIVIDOR
);
	server.send(200, "text/html", temp);
  digitalWrite(LED_BUILTIN, HIGH);
}

float versionSW=1.0;
char versionSWString[] = "Teplomer v"; //SW name & version

void setup() {
#ifdef verbose
  Serial.begin(PORTSPEED);
  DEBUG_PRINT(versionSWString);
  DEBUG_PRINTLN(versionSW);
#endif

	pinMode (LED_BUILTIN, OUTPUT );
  digitalWrite(LED_BUILTIN, LOW );
  
  dsInit();
  
  pinMode (LED_BUILTIN, OUTPUT );
  digitalWrite(LED_BUILTIN, LOW );
  //WiFi.config(ip); 
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  //WiFi.begin(ssid, password);
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    DEBUG_PRINTLN("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  
  server.on("/", handleRoot);
	server.on("/inline", []() {
		server.send(200, "text/plain", "this works as well");
	});
	server.begin();
	DEBUG_PRINTLN("HTTP server started");

  digitalWrite(LED_BUILTIN, HIGH );

}

uint32_t x=0;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

 if (millis() - lastSendTime >= sendTimeDelay) {
    lastSendTime = millis();
    
    mereni();
  }

  server.handleClient();
}

void dsInit(void) {
  dsSensor.begin();
  DEBUG_PRINT("Found DS18B20 device.");

  // // Loop through each device, print out address
  // for (byte i=0;i<numberOfDevices; i++) {
      // // Search the wire for address
    // if (dsSensor.getAddress(tempDeviceAddress, i)) {
      // memcpy(tempDeviceAddresses[i],tempDeviceAddress,8);
      // //DEBUG_PRINTLN(tempDeviceAddresses[i]);
    // }
    // else
    // {
      // //DEBUG_PRINTLN("Unable to get device address for sensor " + i);
    // }
  // }
  dsSensor.setResolution(TEMPERATURE_PRECISION);
  dsSensor.setWaitForConversion(false);
}


void mereni() {
  dsSensor.requestTemperatures(); 
  delay(dsMeassureDelay);
  teplota = dsSensor.getTempCByIndex(0) * TEMPERATURE_DIVIDOR;
  DEBUG_PRINT("Teplota - ");
  DEBUG_PRINT(teplota / TEMPERATURE_DIVIDOR);
  DEBUG_PRINTLN(" C");
  
  DEBUG_PRINT("Send temperature ");
  DEBUG_PRINT(teplota / TEMPERATURE_DIVIDOR);
  DEBUG_PRINT(" of sensor ");
  DEBUG_PRINT(" to MQTT broker");
  if (! _teplota.publish(teplota / TEMPERATURE_DIVIDOR)) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
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