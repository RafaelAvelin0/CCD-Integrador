#include "telemetry.h"
#include "globals.h"
#include "utils.h"
#include "secrets.h"

void logMonitor(String msg)
{
  Serial.println(msg);
  if (telnetClient && telnetClient.connected())
  {
    telnetClient.println(msg);
  }
}

void reconnectMQTT()
{
  logMonitor("Tentando conexao MQTT no IP: " + String(mqtt_server));
  String clientId = "ESP8266-Datacenter-" + String(random(0xffff), HEX);

  // ========================================================
  // CONFIGURAÇÃO DO LAST WILL AND TESTAMENT (LWT)
  // ========================================================
  // Payload JSON que o broker vai publicar automaticamente se o ESP8266 cair
  const char* willMessage = "{"
                            "\"team\":\"piredes2026\","
                            "\"device\":\"ESP8266-Datacenter\","
                            "\"status\":\"OFFLINE\","
                            "\"data\":{"
                            "\"presence\":\"DESCONHECIDO\""
                            "}"
                            "}";
  uint8_t willQos = 0;       // Qualidade de Serviço 0 (padrão)
  boolean willRetain = true; // Define que a mensagem de "morte" deve ficar retida

  // Conecta passando os parâmetros do LWT
  if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_password, topico_telemetria, willQos, willRetain, willMessage))
  {
    logMonitor("-> MQTT Conectado com sucesso!");
    
  }
  else
  {
    logMonitor("-> Falha MQTT, rc=" + String(mqttClient.state()));
  }
}

void enviarPayloadJSON(float t, float h, float orvalho, const char *statusPresenca)
{
  if (!mqttClient.connected())
    return;

  char payload[512];
  snprintf(payload, sizeof(payload),
           "{"
           "\"team\":\"piredes2026\","
           "\"device\":\"ESP8266-Datacenter\","
           "\"status\":\"ONLINE\","
           "\"ip\":\"%s\","
           "\"ssid\":\"%s\","
           "\"sensor\":\"DHT21+PIR\","
           "\"data\":{"
           "\"temperature\":%.1f,"
           "\"humidity\":%.1f,"
           "\"dew_point\":%.1f,"
           "\"presence\":\"%s\""
           "},"
           "\"timestamp\":\"%s\""
           "}",
           WiFi.localIP().toString().c_str(),
           WiFi.SSID().c_str(),
           t, h, orvalho, statusPresenca,
           getTimestamp().c_str());

  // O 'true' no final garante que a mensagem normal também fique retida no Broker
  mqttClient.publish(topico_telemetria, payload, true); 
  
  tempoUltimoEnvioMQTT = millis(); // Reseta o cronômetro de 1 minuto
  logMonitor("JSON Enviado -> " + String(payload));
}