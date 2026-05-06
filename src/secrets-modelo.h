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

// Configuração de IP Estático (Ajuste para a faixa do seu roteador)
IPAddress local_IP(192, 168, 1, 50); // IP que o ESP vai assumir
IPAddress gateway(192, 168, 1, 1);   // IP do Roteador
IPAddress subnet(255, 255, 255, 0);  // Máscara
IPAddress primaryDNS(8, 8, 8, 8);    // DNS (Google)

#endif
