#ifndef GLOBALS_H
#define GLOBALS_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>

// Servidores e Clientes de Rede
extern WiFiServer telnetServer;
extern WiFiClient telnetClient;
extern WiFiClient espClient;
extern PubSubClient mqttClient;
extern ESP8266WebServer server;

// Tópicos MQTT
extern const char *topico_telemetria; 
extern const char *topico_timeout_set;

// Variáveis de estado globais compartilhadas
extern unsigned long tempoUltimoEnvioMQTT; 
extern unsigned long tempoLampadaLigada;
extern uint32_t contadorBoots;

// Passaram a ser globais para o Dashboard Web conseguir acessar a leitura em tempo real
extern float temperaturaAtual;
extern float umidadeAtual;

#endif