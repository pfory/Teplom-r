#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define ONE_WIRE_BUS 2 //GPIO2
#define TEMPERATURE_PRECISION 12

#define AIO_SERVER      "192.168.1.56"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "datel"
#define AIO_KEY         "hanka12"

IPAddress _ip           = IPAddress(192, 168, 1, 105);
IPAddress _gw           = IPAddress(192, 168, 1, 1);
IPAddress _sn           = IPAddress(255, 255, 255, 0);

#define TEMPERATURE_DIVIDOR 100
