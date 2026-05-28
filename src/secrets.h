#ifndef SECRETS_H
#define SECRETS_H
#include <ESP8266WiFi.h>

// Apenas declaramos que essas variáveis existem em algum lugar (extern)
extern const char* ssid;
extern const char* password;

extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress primaryDNS;

extern const char* mqtt_server;
extern const int mqtt_port;
extern const char* mqtt_user;
extern const char* mqtt_password;

#endif