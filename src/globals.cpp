#include "globals.h"
#include "config.h"

// Inicialização dos Servidores e Clientes de Rede
WiFiServer telnetServer(23);
WiFiClient telnetClient;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

ESP8266WebServer server(80);

const char *topico_telemetria = "piredes2026/datacenter/telemetria";
const char *topico_timeout_set = "piredes2026/datacenter/timeout/set"; 

// Armazena quando o último JSON foi enviado 
unsigned long tempoUltimoEnvioMQTT = 0;

// Variável que o Home Assistant poderá alterar via MQTT ou Painel Web
unsigned long tempoLampadaLigada = DEFAULT_TIMEOUT_PRESENCA;

uint32_t contadorBoots = 0; 

// Variáveis da interface Web
float temperaturaAtual = 0.0;
float umidadeAtual = 0.0;