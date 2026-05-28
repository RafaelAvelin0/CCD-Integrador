/**
 * PROJETO INTEGRADOR - Monitoramento de Datacenter
 * Módulo: ESP8266 (Nó Sensor)
 * Funcionalidades: Leitura de Temperatura/Umidade (DHT21), Presença (PIR), 
 * Cálculo de Ponto de Orvalho, Comunicação MQTT e Atualização OTA.
 */

#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include "secrets.h" // Arquivo externo contendo senhas e IPs (Segurança)
#include "config.h"  // Mapeamento de pinos e tempos do sistema
#include "globals.h" // Instâncias de rede e variáveis compartilhadas
#include "utils.h"   // Funções matemáticas e de tempo
#include "telemetry.h" // Lógica de envio MQTT e logs

#define DHTTYPE DHT21 // Especifica o modelo do sensor
DHT dht(PIN_DHT, DHTTYPE);

// ==========================================
// VARIÁVEIS DE ESTADO E CRONÔMETROS
// ==========================================
// Variáveis para gerenciar o tempo sem travar o processador (Programação Não-Bloqueante)
bool sistemaCalibrado = false;
bool ntpConfigurado = false;
unsigned long tempoInicioSessao = 0;
unsigned long tempoUltimaPresenca = 0;
unsigned long tempoUltimaLeituraDHT = 0;
unsigned long tempoUltimaTentativaWiFi = 0;
unsigned long tempoUltimaTentativaMQTT = 0;
unsigned long tempoUltimoPisca = 0;
// unsigned long tempoUltimoEnvioMQTT = 0; // Armazena quando o último JSON foi enviado (Movida para globals.cpp)
int ultimoRestanteLog = -1; // Variável para evitar logs repetidos durante a calibração

// Variáveis de memória para comparar se houve variação no clima
bool lampadaLigada = false;
float ultimaTemperatura = -100.0;
float ultimaUmidade = -100.0;

/**
 * Configura o Over-The-Air, permitindo envio de código via Wi-Fi pelo PlatformIO.
 */
void setupOTA()
{
  ArduinoOTA.setHostname("esp-datacenter");
  ArduinoOTA.begin();
}

// ==========================================
// SETUP (Executado apenas uma vez no boot)
// ==========================================
void setup()
{
  Serial.begin(115200);
  delay(2500); // Pausa para permitir a conexão do Monitor Serial do PC

  Serial.println("\n\n=== BOOT DO SISTEMA ===");

  // Configuração dos Pinos
  pinMode(PIN_PRESENCE, INPUT_PULLUP);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  dht.begin(); // Inicializa o sensor de temperatura

  // Configuração do MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setBufferSize(512); // Aumenta o limite de mensagem para suportar nosso JSON

  // Configuração do Wi-Fi com IP Estático (Definido no secrets.h)
  WiFi.config(local_IP, gateway, subnet, primaryDNS);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  setupOTA();
  telnetServer.begin();

  tempoInicioSessao = millis(); // Marca a hora que o dispositivo ligou
}

// ==========================================
// LOOP PRINCIPAL (Executado continuamente)
// ==========================================
void loop()
{
  unsigned long tempoAtual = millis(); // Captura o tempo atual em cada ciclo

  // ----------------------------------------
  // ETAPA 1: GESTÃO DE REDE E SERVIÇOS
  // ----------------------------------------
  if (WiFi.status() != WL_CONNECTED)
  {
    // Tenta reconectar ao Wi-Fi a cada 10 segundos sem travar o loop
    if (tempoAtual - tempoUltimaTentativaWiFi >= INTERVALO_RECONEXAO_WIFI)
    {
      WiFi.begin(ssid, password);
      tempoUltimaTentativaWiFi = tempoAtual;
    }
  }
  else // Se o Wi-Fi estiver conectado:
  {
    ArduinoOTA.handle(); // Fica escutando pedidos de atualização de firmware

    // Sincroniza o relógio na primeira vez que conectar (Fuso UTC-3)
    if (!ntpConfigurado)
    {
      configTime(-10800, 0, "pool.ntp.org", "time.nist.gov");
      ntpConfigurado = true;
    }

    // Mantém a conexão MQTT viva ou tenta reconectar
    if (!mqttClient.connected())
    {
      if (tempoAtual - tempoUltimaTentativaMQTT >= INTERVALO_RECONEXAO_MQTT)
      {
        tempoUltimaTentativaMQTT = tempoAtual;
        reconnectMQTT();
      }
    }
    else
    {
      mqttClient.loop(); // Processa mensagens MQTT em background
    }

    // Gerencia conexões Telnet de entrada para debug remoto
    if (telnetServer.hasClient())
    {
      if (!telnetClient || !telnetClient.connected())
      {
        if (telnetClient) telnetClient.stop();
        telnetClient = telnetServer.accept();
        telnetClient.println("=== Console Datacenter Conectado ===");
      }
    }
  }

  // ----------------------------------------
  // ETAPA 2: AQUECIMENTO E CALIBRAÇÃO DO PIR
  // ----------------------------------------
  // O sensor PIR precisa de ~80s lendo o ambiente antes de enviar dados confiáveis
  if (!sistemaCalibrado)
  {
    // Pisca o LED Verde a cada 500ms para indicar que está em calibração
    if (tempoAtual - tempoUltimoPisca >= 500)
    {
      tempoUltimoPisca = tempoAtual;
      digitalWrite(PIN_LED_GREEN, !digitalRead(PIN_LED_GREEN));

      long restante = (DURACAO_CALIBRACAO - (tempoAtual - tempoInicioSessao)) / 1000;
      if (restante >= 0 && restante % 10 == 0 && restante != ultimoRestanteLog) // Imprime no log a cada 10s
      {
        ultimoRestanteLog = restante;
        logMonitor("Calibrando PIR... " + String(restante) + "s restantes.");
      }
    }

    // Finaliza a calibração
    if (tempoAtual - tempoInicioSessao >= DURACAO_CALIBRACAO)
    {
      sistemaCalibrado = true;
      digitalWrite(PIN_LED_GREEN, LOW); 
      tempoUltimaPresenca = tempoAtual;
      logMonitor(">>> SISTEMA PRONTO E CALIBRADO <<<");
    }
    return; // Retorna ao início do loop. Não passa para a Etapa 3 enquanto não calibrar.
  }

  // ----------------------------------------
  // ETAPA 3: MONITORAMENTO DOS SENSORES
  // ----------------------------------------

  // A. EVENTOS DO SENSOR DE PRESENÇA (Gatilho Imediato)
  if (digitalRead(PIN_PRESENCE) == LOW) // LOW indica movimento detectado
  {
    tempoUltimaPresenca = tempoAtual; // Renova o cronômetro de presença
    if (!lampadaLigada)
    {
      digitalWrite(PIN_LED_GREEN, HIGH); // Acende indicador
      lampadaLigada = true;
      logMonitor("[EVENTO] Movimento detectado.");

      // Força o envio imediato dos dados do clima atual junto com o alerta de movimento
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      if (!isnan(h) && !isnan(t))
      {
        enviarPayloadJSON(t, h, calcularPontoOrvalho(t, h), "ON");
      }
    }
  }

  // Desativa a presença após o tempo limite configurado sem detectar movimento
  if (lampadaLigada && (tempoAtual - tempoUltimaPresenca >= TEMPO_LAMPADA_LIGADA))
  {
    digitalWrite(PIN_LED_GREEN, LOW);
    lampadaLigada = false;
    logMonitor("[INFO] Ambiente vazio.");

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t))
    {
      enviarPayloadJSON(t, h, calcularPontoOrvalho(t, h), "OFF");
    }
  }

  // B. EVENTOS DO SENSOR DE CLIMA (Leitura Baseada em Tempo e Variação)
  if (tempoAtual - tempoUltimaLeituraDHT >= INTERVALO_LEITURA_DHT)
  {
    tempoUltimaLeituraDHT = tempoAtual;
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Verifica se a leitura foi bem-sucedida (não é Not-A-Number)
    if (!isnan(h) && !isnan(t))
    {
      bool emAlerta = (t >= TEMP_ALERTA);
      digitalWrite(PIN_LED_RED, emAlerta ? HIGH : LOW); // Acende LED vermelho se houver superaquecimento

      // LÓGICA DE ECONOMIA DE REDE: O pacote só é enviado ao servidor se atender a 1 de 3 critérios:
      // 1. O datacenter está superaquecendo (emAlerta)
      // 2. A temperatura mudou de forma relevante (0.5 graus)
      // 3. Passou 1 minuto desde o último envio (Heartbeat - para garantir que o sistema não travou)
      if (emAlerta ||
          abs(t - ultimaTemperatura) >= 0.5 ||
          (tempoAtual - tempoUltimoEnvioMQTT >= INTERVALO_HEARTBEAT))
      {
        ultimaTemperatura = t;
        ultimaUmidade = h;

        float orvalho = calcularPontoOrvalho(t, h);
        enviarPayloadJSON(t, h, orvalho, lampadaLigada ? "ON" : "OFF");
      }
    }
  }
}