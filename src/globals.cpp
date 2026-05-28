#include "globals.h"

// Inicialização dos Servidores e Clientes de Rede
WiFiServer telnetServer(23);
WiFiClient telnetClient;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

const char *topico_telemetria = "piredes2026/datacenter/telemetria";

// Armazena quando o último JSON foi enviado (Compartilhado com main e telemetry)
unsigned long tempoUltimoEnvioMQTT = 0;