#ifndef GLOBALS_H
#define GLOBALS_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Servidores e Clientes de Rede
extern WiFiServer telnetServer;
extern WiFiClient telnetClient;
extern WiFiClient espClient;
extern PubSubClient mqttClient;

// Tópicos MQTT
extern const char *topico_telemetria;

// Variáveis de estado globais compartilhadas
extern unsigned long tempoUltimoEnvioMQTT; 

#endif