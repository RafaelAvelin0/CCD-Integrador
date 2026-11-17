#ifndef GLOBALS_H
#define GLOBALS_H

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>

// Servidores e Clientes de Rede
extern WiFiServer telnetServer;
extern WiFiClient telnetClient;
extern WiFiClientSecure espClient;
extern PubSubClient mqttClient;
extern ESP8266WebServer server;

// Tópicos MQTT
extern String topico_telemetria; 
extern String topico_timeout_set;
extern String topico_heartbeat_set;
extern String topico_ota_url;

// Variáveis de estado globais compartilhadas
extern unsigned long tempoUltimoEnvioMQTT; 
extern unsigned long tempoLampadaLigada;
extern uint32_t contadorBoots;
extern unsigned long intervaloHeartbeat; // Intervalo de envio de telemetria (heartbeat) em milissegundos

// Passaram a ser globais para o Dashboard Web conseguir acessar a leitura em tempo real
extern float temperaturaAtual;
extern float umidadeAtual;

// Variáveis de configuração do sistema para OTA http
extern bool pendenteOTA;
extern String urlOTA;

#endif