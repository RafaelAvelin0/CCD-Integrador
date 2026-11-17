#include "secrets.h"

// ⚠️ INSTRUÇÕES:
// 1. Renomeie este arquivo para "secrets.cpp"
// 2. Preencha com os dados reais da sua rede, do broker e do painel web
// 3. O arquivo "secrets.cpp" será ignorado pelo git (não subirá para o GitHub)

// Credenciais do Wi-Fi
const char* ssid = "NOME_DA_SUA_REDE";
const char* password = "SENHA_DA_REDE";

/* Configuração de IP Estático (opcional - desativada por padrão, o ESP usa DHCP)
// Para usar: descomente este bloco E o bloco correspondente em "secrets.h",
// e chame WiFi.config(local_IP, gateway, subnet, primaryDNS) antes do WiFi.begin().
// Altere os valores de acordo com o padrão da rede do datacenter
IPAddress local_IP(192, 168, 1, 50); // O IP fixo que o ESP vai usar
IPAddress gateway(192, 168, 1, 1);   // O IP do seu Roteador
IPAddress subnet(255, 255, 255, 0);  // Máscara de sub-rede
IPAddress primaryDNS(8, 8, 8, 8);    // DNS do Google (opcional)*/

// Credenciais MQTT (Home Assistant)
const char* mqtt_server = "IP_OU_ENDERECO_DO_BROKER";
const int mqtt_port = 8883; // Número inteiro (sem aspas). Porta do broker com TLS
const char* mqtt_user = "USUARIO_MQTT";
const char* mqtt_password = "SENHA_MQTT";

// Credenciais do Servidor Web (Acesso ao Dashboard e OTA)
const char* http_user = "USUARIO_WEB";
const char* http_password = "SENHA_WEB";

// Certificado da CA do broker MQTT (formato PEM), usado na conexão TLS.
// Cole o conteúdo completo do certificado entre as marcações abaixo,
// incluindo as linhas BEGIN CERTIFICATE e END CERTIFICATE.
const char ca_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
COLE_AQUI_O_CERTIFICADO_DA_CA_DO_BROKER
-----END CERTIFICATE-----
)EOF";
