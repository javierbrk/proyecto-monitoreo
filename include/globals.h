#ifndef GLOBALS_H
#define GLOBALS_H

#include <WebServer.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include "sensors/ISensor.h"

extern WebServer server;
extern ISensor* sensor;
extern WiFiManager wifiManager;
extern WiFiClientSecure clientSecure;
extern WiFiClient client;
extern HTTPClient http;

#endif // GLOBALS_H