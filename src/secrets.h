#ifndef SECRETS_H
#define SECRETS_H
#include <ESP8266WiFi.h>

// Apenas declaramos que essas variáveis existem em algum lugar (extern)
extern const char* ssid;
extern const char* password;

// Configuração de IP Estático
extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress primaryDNS;

// Credenciais MQTT (Home Assistant)
extern const char* mqtt_server;
extern const int mqtt_port;
extern const char* mqtt_user;
extern const char* mqtt_password;

// Credenciais HTTP para acesso à interface web do ESP8266
extern const char* http_user;
extern const char* http_password;

#endif