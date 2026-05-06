#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include "secrets.h"

// Mapeamento de Pinos
#define PIN_PRESENCE 4
#define PIN_LED_GREEN 16
#define PIN_LED_RED 2
#define PIN_DHT 14

#define DHTTYPE DHT21
DHT dht(PIN_DHT, DHTTYPE);

// Configurações de Tempo
const unsigned long DURACAO_CALIBRACAO = 80000;   // 80 segundos (mude para 5000 para testes)
const unsigned long TEMPO_LAMPADA_LIGADA = 15000; // 30 segundos
const unsigned long INTERVALO_LEITURA_DHT = 2000;
const unsigned long INTERVALO_RECONEXAO_WIFI = 10000;
const float TEMP_ALERTA = 25.0;

// Variáveis de controle de Estado
bool sistemaCalibrado = false;
unsigned long tempoInicioSessao = 0;
unsigned long tempoUltimaPresenca = 0;
unsigned long tempoUltimaLeituraDHT = 0;
unsigned long tempoUltimaTentativaWiFi = 0;
unsigned long tempoUltimoPisca = 0;

bool lampadaLigada = false;
float ultimaTemperatura = -100.0;
float ultimaUmidade = -100.0;

WiFiServer telnetServer(23);
WiFiClient telnetClient;

void logMonitor(String msg)
{
  Serial.println(msg);
  if (telnetClient && telnetClient.connected())
  {
    telnetClient.println(msg);
  }
}

void setupOTA()
{
  ArduinoOTA.setHostname("esp-datacenter");
  ArduinoOTA.begin();
}

void setup()
{
  Serial.begin(115200);

  pinMode(PIN_PRESENCE, INPUT_PULLUP);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  dht.begin();

  // Wi-Fi com IP Estático
  WiFi.config(local_IP, gateway, subnet, primaryDNS);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Note que não usamos mais o while(WiFi.status() != WL_CONNECTED) com delay
  // O loop cuidará da conexão para não travar o início do programa

  setupOTA();
  telnetServer.begin();

  tempoInicioSessao = millis();
  Serial.println("\n--- SISTEMA INICIADO ---");
  Serial.println("Aguardando conexao de rede e calibracao do sensor...");
}

void loop()
{
  unsigned long tempoAtual = millis();

  // --- 1. GESTÃO DE REDE (Sempre rodando, nunca bloqueia) ---
  if (WiFi.status() != WL_CONNECTED)
  {
    if (tempoAtual - tempoUltimaTentativaWiFi >= INTERVALO_RECONEXAO_WIFI)
    {
      WiFi.begin(ssid, password);
      tempoUltimaTentativaWiFi = tempoAtual;
    }
  }
  else
  {
    ArduinoOTA.handle();
    if (telnetServer.hasClient())
    {
      if (!telnetClient || !telnetClient.connected())
      {
        if (telnetClient)
          telnetClient.stop();
        telnetClient = telnetServer.accept();
        telnetClient.println("=== Console Datacenter Conectado ===");
      }
      else
      {
        telnetServer.accept().stop();
      }
    }
  }

  // --- 2. LÓGICA DE CALIBRAÇÃO NÃO BLOQUEANTE ---
  if (!sistemaCalibrado)
  {
    // Pisca o LED Verde a cada 500ms para indicar calibração
    if (tempoAtual - tempoUltimoPisca >= 500)
    {
      tempoUltimoPisca = tempoAtual;
      digitalWrite(PIN_LED_GREEN, !digitalRead(PIN_LED_GREEN));

      // Informa o tempo restante no Serial/Telnet para você saber que está vivo
      long restante = (DURACAO_CALIBRACAO - (tempoAtual - tempoInicioSessao)) / 1000;
      if (restante >= 0)
      {
        logMonitor("Calibrando sensor... " + String(restante) + "s restantes.");
      }
    }

    // Verifica se terminou o tempo de calibração
    if (tempoAtual - tempoInicioSessao >= DURACAO_CALIBRACAO)
    {
      sistemaCalibrado = true;
      digitalWrite(PIN_LED_GREEN, LOW); // Desliga o pisca-pisca
      tempoUltimaPresenca = tempoAtual;
      logMonitor(">>> SISTEMA PRONTO E CALIBRADO <<<");
    }

    return; // Sai do loop aqui, não executa o monitoramento enquanto não calibrar
  }

  // --- 3. MONITORAMENTO ATIVO (Só executa após calibração) ---

  // Sensor de Presença
  if (digitalRead(PIN_PRESENCE) == LOW)
  {
    tempoUltimaPresenca = tempoAtual;
    if (!lampadaLigada)
    {
      digitalWrite(PIN_LED_GREEN, HIGH);
      lampadaLigada = true;
      logMonitor("[EVENTO] Movimento detectado. Lampada ligada.");
    }
  }

  if (lampadaLigada && (tempoAtual - tempoUltimaPresenca >= TEMPO_LAMPADA_LIGADA))
  {
    digitalWrite(PIN_LED_GREEN, LOW);
    lampadaLigada = false;
    logMonitor("[INFO] Ambiente vazio. Lampada desligada.");
  }

  // Sensor de Temperatura
  if (tempoAtual - tempoUltimaLeituraDHT >= INTERVALO_LEITURA_DHT)
  {
    tempoUltimaLeituraDHT = tempoAtual;
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t))
    {
      bool emAlerta = (t >= TEMP_ALERTA);
      digitalWrite(PIN_LED_RED, emAlerta ? HIGH : LOW);

      if (emAlerta || abs(t - ultimaTemperatura) >= 0.5 || abs(h - ultimaUmidade) >= 1.0)
      {
        ultimaTemperatura = t;
        ultimaUmidade = h;
        String msg = "Status: " + String(t, 1) + "C, " + String(h, 1) + "%";
        logMonitor(emAlerta ? "[ALERTA] " + msg : msg);
      }
    }
  }
}
