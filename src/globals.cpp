#include "globals.h"
#include "config.h"

// Inicialização dos Servidores e Clientes de Rede
WiFiServer telnetServer(23);
WiFiClient telnetClient;

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

ESP8266WebServer server(80);

String topico_telemetria;
String topico_timeout_set;
String topico_heartbeat_set;
String topico_ota_url;

// Armazena quando o último JSON foi enviado 
unsigned long tempoUltimoEnvioMQTT = 0;

// Variável que o Home Assistant poderá alterar via MQTT ou Painel Web
unsigned long tempoLampadaLigada = DEFAULT_TIMEOUT_PRESENCA;

uint32_t contadorBoots = 0; 

// Variáveis da interface Web
float temperaturaAtual = 0.0;
float umidadeAtual = 0.0;
unsigned long intervaloHeartbeat = 60000; // 60 segundos padrão

// Variáveis de configuração do sistema para OTA http
bool pendenteOTA = false;
String urlOTA = "";