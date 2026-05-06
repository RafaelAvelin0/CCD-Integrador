#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include "secrets.h"

// Mapeamento de hardware
#define PIN_PRESENCE 4
#define PIN_LED_GREEN 16
#define PIN_LED_RED 2
#define PIN_DHT 14

#define DHTTYPE DHT21
DHT dht(PIN_DHT, DHTTYPE);

// Parâmetros de operação
const unsigned long DURACAO_CALIBRACAO = 80000;   // Tempo de acomodação do sensor PIR ao ligar
const unsigned long TEMPO_LAMPADA_LIGADA = 15000; // Timeout da iluminação por inatividade
const unsigned long INTERVALO_LEITURA_DHT = 2000; // Taxa de amostragem do sensor ambiental
const unsigned long INTERVALO_RECONEXAO_WIFI = 10000;
const float TEMP_ALERTA = 25.0; // Limite crítico de temperatura do ambiente

// Variáveis de estado
bool isCalibrated = false;
unsigned long startMillis = 0;
unsigned long lastMotionTime = 0;
unsigned long lastDhtRead = 0;
unsigned long lastWifiAttempt = 0;
unsigned long lastBlink = 0;

bool lampState = false;
float lastTemp = -100.0;
float lastHum = -100.0;

// Serviço de log remoto
WiFiServer telnet(23);
WiFiClient client;

// Envia mensagens simultaneamente para a serial e para o cliente conectado
void log(String msg)
{
  Serial.println(msg);
  if (client && client.connected())
  {
    client.println(msg);
  }
}

void initOTA()
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

  WiFi.config(local_IP, gateway, subnet, primaryDNS);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  initOTA();
  telnet.begin();

  startMillis = millis();
  Serial.println("\nBooting...");
}

void loop()
{
  unsigned long currentMillis = millis();

  // Garante a persistência da conexão WiFi de forma não-bloqueante
  if (WiFi.status() != WL_CONNECTED)
  {
    if (currentMillis - lastWifiAttempt >= INTERVALO_RECONEXAO_WIFI)
    {
      WiFi.begin(ssid, password);
      lastWifiAttempt = currentMillis;
    }
  }
  else
  {
    ArduinoOTA.handle();

    // Gerencia o acesso ao console Telnet (permite apenas 1 cliente ativo)
    if (telnet.hasClient())
    {
      if (!client || !client.connected())
      {
        if (client)
          client.stop();
        client = telnet.accept();
        client.println("Console conectado");
      }
      else
      {
        telnet.accept().stop();
      }
    }
  }

  // O sensor PIR precisa de um tempo inicial para estabilizar o sinal infravermelho
  if (!isCalibrated)
  {
    // Feedback visual do status de calibração
    if (currentMillis - lastBlink >= 500)
    {
      lastBlink = currentMillis;
      digitalWrite(PIN_LED_GREEN, !digitalRead(PIN_LED_GREEN));

      long timeLeft = (DURACAO_CALIBRACAO - (currentMillis - startMillis)) / 1000;
      if (timeLeft >= 0)
      {
        log("Calibrando... " + String(timeLeft) + "s");
      }
    }

    if (currentMillis - startMillis >= DURACAO_CALIBRACAO)
    {
      isCalibrated = true;
      digitalWrite(PIN_LED_GREEN, LOW);
      lastMotionTime = currentMillis;
      log("Calibracao concluida");
    }
    return; // Interrompe o loop até que o sensor esteja confiável
  }

  // Controle de Iluminação via Presença
  if (digitalRead(PIN_PRESENCE) == LOW)
  {
    lastMotionTime = currentMillis;
    if (!lampState)
    {
      digitalWrite(PIN_LED_GREEN, HIGH);
      lampState = true;
      log("Movimento: Lampada ON");
    }
  }

  // Desliga a iluminação após o tempo de timeout
  if (lampState && (currentMillis - lastMotionTime >= TEMPO_LAMPADA_LIGADA))
  {
    digitalWrite(PIN_LED_GREEN, LOW);
    lampState = false;
    log("Timeout: Lampada OFF");
  }

  // Monitoramento Ambiental
  if (currentMillis - lastDhtRead >= INTERVALO_LEITURA_DHT)
  {
    lastDhtRead = currentMillis;
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Valida a leitura do sensor
    if (!isnan(h) && !isnan(t))
    {
      bool alert = (t >= TEMP_ALERTA);
      digitalWrite(PIN_LED_RED, alert ? HIGH : LOW);

      // Reduz o volume de logs reportando apenas mudanças significativas ou estados críticos
      if (alert || abs(t - lastTemp) >= 0.5 || abs(h - lastHum) >= 1.0)
      {
        lastTemp = t;
        lastHum = h;
        String msg = "T: " + String(t, 1) + "C H: " + String(h, 1) + "%";
        log(alert ? "ALERTA - " + msg : msg);
      }
    }
  }
}