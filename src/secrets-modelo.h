#ifndef SECRETS_H
#define SECRETS_H
#include <ESP8266WiFi.h>

// ⚠️ INSTRUÇÕES:
// 1. Renomeie este arquivo para "secrets.h"
// 2. Preencha com os dados reais da sua rede
// 3. O arquivo "secrets.h" será ignorado pelo git (não subirá para o GitHub)

// Credenciais do Wi-Fi
const char* ssid = "NOME_DA_SUA_REDE";
const char* password = "SENHA_DA_REDE";

// Configuração de IP Estático
// Altere o "192, 168, 1, X" de acordo com o padrão do roteador do seu datacenter
IPAddress local_IP(192, 168, 1, 50); // O IP fixo que o ESP vai usar
IPAddress gateway(192, 168, 1, 1);   // O IP do seu Roteador
IPAddress subnet(255, 255, 255, 0);  // Máscara de sub-rede
IPAddress primaryDNS(8, 8, 8, 8);    // DNS do Google (opcional)

// Credenciais MQTT (Home Assistant)
const char* mqtt_server = "IP_OU_ENDEREÇO_DO_BROKER";
const int mqtt_port = "PORTA_DO_BROKER";
const char* mqtt_user = "USUARIO";
const char* mqtt_password = "SENHA";

#endif
