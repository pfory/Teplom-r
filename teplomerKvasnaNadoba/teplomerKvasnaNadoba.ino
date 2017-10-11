#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define ONE_WIRE_BUS 2 //IO2
#define TEMPERATURE_PRECISION 12
OneWire onewire(ONE_WIRE_BUS); // pin for onewire DALLAS bus
DallasTemperature dsSensors(&onewire);
DeviceAddress tempDeviceAddress;
#define NUMBER_OF_DEVICES 2

DeviceAddress tempDeviceAddresses[NUMBER_OF_DEVICES];
//int  resolution = 12;
unsigned int numberOfDevices; // Number of temperature devices found
float sensor[NUMBER_OF_DEVICES];

unsigned int const dsMeassureDelay=750; //delay between start meassurement and read valid data in ms

const char *ssid = "Datlovo";
const char *password = "Nu6kMABmseYwbCoJ7LyG";

ESP8266WebServer server(80);

#define AIO_SERVER      "192.168.1.56"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "datel"
#define AIO_KEY         "hanka12"

WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

unsigned int const sendTimeDelay=60000;
signed long lastSendTime = sendTimeDelay * -1;

/****************************** Feeds ***************************************/
Adafruit_MQTT_Publish temperaturesNadoba = Adafruit_MQTT_Publish(&mqtt,  "/home/kvasnaNadoba/teplotaNadoba");
Adafruit_MQTT_Publish temperaturesOkoli = Adafruit_MQTT_Publish(&mqtt,  "/home/kvasnaNadoba/teplotaOkoli");
Adafruit_MQTT_Publish temperaturesVersionSW = Adafruit_MQTT_Publish(&mqtt,  "/home/kvasnaNadoba/VersionSW");
Adafruit_MQTT_Publish temperaturesHeartBeat= Adafruit_MQTT_Publish(&mqtt,  "/home/kvasnaNadoba/HeartBeat");

// Setup a feed called 'onoff' for subscribing to changes.
//Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");

#define SERIALSPEED 115200

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
    <h1>Teploty</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
	<p>Teplota 1: %02d&deg;C</p>\
	<p>Teplota 2: %02d&deg;C</p>\
  </body>\
</html>",

hr, min % 60, sec % 60, (int)sensor[0], (int)sensor[1]
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
  Serial.begin(SERIALSPEED);
  Serial.println("Kvasna nadoba");
  
  dsInit();
  WiFi.begin(ssid, password);

	// Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
  
  server.on("/", handleRoot);
	server.on("/inline", []() {
		server.send(200, "text/plain", "this works as well");
	});
	server.onNotFound(handleNotFound);
	server.begin();
	Serial.println("HTTP server started");
  
  // Setup MQTT subscription for onoff feed.
  //mqtt.subscribe(&onoffbutton);

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
      // Serial.print(F("Got: "));
      // Serial.println((char *)onoffbutton.lastread);
    // }
  // }

 if (millis() - lastSendTime >= sendTimeDelay) {
    lastSendTime = millis();
    
    mereni();
    
    if (! temperaturesNadoba.publish(sensor[0])) {
      Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));
    }
    if (! temperaturesOkoli.publish(sensor[1])) {
      Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));
    }
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
  Serial.print("Found ");
  Serial.print(numberOfDevices);
  Serial.print(" DS18B20 device(s).");

  // Loop through each device, print out address
  for (byte i=0;i<numberOfDevices; i++) {
      // Search the wire for address
    if (dsSensors.getAddress(tempDeviceAddress, i)) {
      /*for (byte j=0; j<8; j++) {
        if (tempDeviceAddress[j] < 16) Serial.print("0");
      }
      */
      memcpy(tempDeviceAddresses[i],tempDeviceAddress,8);
    }
    else
    {
      //Serial.println("Unable to get device address for sensor " + i);
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
    for (byte j=0;j<10;j++) { //try to read temperature ten times
      //tempTemp = dsSensors.getTempCByIndex(i);
      tempTemp = dsSensors.getTempC(tempDeviceAddresses[i]);
      if (tempTemp>=-55) {
        break;
      }
    }
    sensor[i]=tempTemp;
    Serial.print("Sensor");
    Serial.print(i);
    Serial.print(" (");
    dsSensors.getAddress(tempDeviceAddress, i);
    Serial.print(8);
    Serial.print("):");
    Serial.print(sensor[i]);
    Serial.println("°C");
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

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
