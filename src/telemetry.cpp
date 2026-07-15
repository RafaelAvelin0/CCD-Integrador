#include "telemetry.h"
#include "globals.h"
#include "config.h"
#include "utils.h"
#include "secrets.h"

// Função auxiliar para gerar o nome do dispositivo com os 4 últimos dígitos do MAC
String getDeviceName() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  return "ESP8266-" + mac.substring(mac.length() - 4) + "-Datacenter";
}

void logMonitor(String msg)
{
  Serial.println(msg);
  if (telnetClient && telnetClient.connected())
  {
    telnetClient.println(msg);
  }
}

// Recebe mensagens do Home Assistant (Alteração do Timeout)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  
  logMonitor("[MQTT CMD] Topico: " + String(topic) + " | Valor: " + msg);

  if (String(topic) == topico_timeout_set) {
    int novoTimeoutSec = msg.toInt();
    // Limita entre 10 segundos e 1 hora (3600s)
    if (novoTimeoutSec >= 10 && novoTimeoutSec <= 3600) {
      tempoLampadaLigada = novoTimeoutSec * 1000UL; // Converte s para ms
      logMonitor("-> Novo timeout de presenca configurado via MQTT para: " + String(novoTimeoutSec) + " segundos.");
      
      // Força o envio imediato da telemetria para atualizar o slider no painel do HA
      tempoUltimoEnvioMQTT = 0; 
    }
  }
}

// Rotina de Autodescoberta MQTT (Home Assistant OS)
void setupHADiscovery() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String deviceName = getDeviceName();
  
  // Dicionário base para agrupar todas as entidades no mesmo "Device" do HA
  String deviceJSON = "\"device\":{\"identifiers\":[\"" + mac + "\"],\"name\":\"" + deviceName + "\",\"manufacturer\":\"IFRN-DIATINF\",\"model\":\"ESP-12F\"}";
  
  // Dicionário de Disponibilidade (LWT) para acinzentar as entidades quando o ESP cair
  String avtyJSON = "\"avty_t\":\"" + String(topico_telemetria) + "\",\"avty_tpl\":\"{{ value_json.status }}\",\"pl_avail\":\"ONLINE\",\"pl_not_avail\":\"OFFLINE\"";
  
  // Junta tudo na mesma string para facilitar o snprintf de cada entidade
  String baseJSON = avtyJSON + "," + deviceJSON;

  char payload[1024];

  // =====================================
  // SENSORES DE CLIMA E AMBIENTE
  // =====================================
  
  // 1. Temperatura (Com stat_cla para gerar gráficos e histórico de longo prazo)
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Temperatura\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.data.temperature }}\",\"dev_cla\":\"temperature\",\"stat_cla\":\"measurement\",\"unit_of_meas\":\"°C\",\"uniq_id\":\"%s_temp\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/temp/config").c_str(), payload, true);

  // 2. Umidade (Com stat_cla)
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Umidade\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.data.humidity }}\",\"dev_cla\":\"humidity\",\"stat_cla\":\"measurement\",\"unit_of_meas\":\"%%\",\"uniq_id\":\"%s_hum\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/hum/config").c_str(), payload, true);

  // 3. Ponto de Orvalho (Com stat_cla)
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Ponto de Orvalho\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.data.dew_point }}\",\"dev_cla\":\"temperature\",\"stat_cla\":\"measurement\",\"unit_of_meas\":\"°C\",\"uniq_id\":\"%s_dew\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/dew/config").c_str(), payload, true);

  // 4. Presença (Sensor Binário)
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Presença\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ 'ON' if value_json.data.presence == 'ON' else 'OFF' }}\",\"dev_cla\":\"motion\",\"uniq_id\":\"%s_pres\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/binary_sensor/") + mac + "/pres/config").c_str(), payload, true);

  // =====================================
  // CONFIGURAÇÕES E INFORMAÇÕES EXTRAS
  // =====================================

  // 5. Configuração Timeout Remoto (Number Input)
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Timeout Presença\",\"stat_t\":\"%s\",\"cmd_t\":\"%s\",\"val_tpl\":\"{{ value_json.config.timeout }}\",\"min\":10,\"max\":3600,\"unit_of_meas\":\"s\",\"ent_cat\":\"config\",\"uniq_id\":\"%s_tout\",%s}", 
    topico_telemetria, topico_timeout_set, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/number/") + mac + "/tout/config").c_str(), payload, true);

  // 6. Última Leitura (Timestamp)
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Última Leitura\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.timestamp }}\",\"dev_cla\":\"timestamp\",\"ent_cat\":\"diagnostic\",\"uniq_id\":\"%s_time\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/time/config").c_str(), payload, true);

  // 7. Dispositivo Datacenter (Nome em texto com ícone customizado)
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Dispositivo Datacenter\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.device }}\",\"ic\":\"mdi:micro-sd\",\"ent_cat\":\"diagnostic\",\"uniq_id\":\"%s_devstr\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/devstr/config").c_str(), payload, true);

  // 8. Conexão do Sistema (Binary Sensor de Conectividade)
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Conexão do Sistema\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.status }}\",\"pl_on\":\"ONLINE\",\"pl_off\":\"OFFLINE\",\"dev_cla\":\"connectivity\",\"ent_cat\":\"diagnostic\",\"uniq_id\":\"%s_conn\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/binary_sensor/") + mac + "/conn/config").c_str(), payload, true);

  // =====================================
  // DIAGNÓSTICOS DO SISTEMA EMBARCADO
  // =====================================

  // Sinal WiFi (RSSI)
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Sinal WiFi\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.diagnostics.rssi }}\",\"dev_cla\":\"signal_strength\",\"unit_of_meas\":\"dBm\",\"ent_cat\":\"diagnostic\",\"uniq_id\":\"%s_rssi\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/rssi/config").c_str(), payload, true);
  
  // IP Local
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"IP Local\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.diagnostics.ip }}\",\"ic\":\"mdi:ip-network\",\"ent_cat\":\"diagnostic\",\"uniq_id\":\"%s_ip\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/ip/config").c_str(), payload, true);

  // Contador de Boots
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Contador de Boots\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.diagnostics.boots }}\",\"ic\":\"mdi:restart\",\"ent_cat\":\"diagnostic\",\"uniq_id\":\"%s_boots\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/boots/config").c_str(), payload, true);

  // Hardware
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Hardware\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.diagnostics.hardware }}\",\"ic\":\"mdi:chip\",\"ent_cat\":\"diagnostic\",\"uniq_id\":\"%s_hw\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/hw/config").c_str(), payload, true);

  // MAC Address
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"MAC Address\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.diagnostics.mac }}\",\"ic\":\"mdi:network-outline\",\"ent_cat\":\"diagnostic\",\"uniq_id\":\"%s_mac\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/mac/config").c_str(), payload, true);

  // MCU
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"MCU\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.diagnostics.mcu }}\",\"ic\":\"mdi:cpu-32-bit\",\"ent_cat\":\"diagnostic\",\"uniq_id\":\"%s_mcu\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/mcu/config").c_str(), payload, true);

  // Memória Livre
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Memória Livre\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.diagnostics.free_mem }}\",\"unit_of_meas\":\"B\",\"ic\":\"mdi:memory\",\"ent_cat\":\"diagnostic\",\"uniq_id\":\"%s_mem\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/mem/config").c_str(), payload, true);

  // SSID WiFi
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Rede Wi-Fi\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.diagnostics.ssid }}\",\"ic\":\"mdi:wifi\",\"ent_cat\":\"diagnostic\",\"uniq_id\":\"%s_ssid\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/ssid/config").c_str(), payload, true);

  // TelePeriod
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"TelePeriod\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.diagnostics.teleperiod }}\",\"unit_of_meas\":\"s\",\"ic\":\"mdi:timer-cog\",\"ent_cat\":\"diagnostic\",\"uniq_id\":\"%s_telep\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/telep/config").c_str(), payload, true);

  // Uptime
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Uptime\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.diagnostics.uptime }}\",\"ic\":\"mdi:clock-outline\",\"ent_cat\":\"diagnostic\",\"uniq_id\":\"%s_uptime\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/uptime/config").c_str(), payload, true);

  // Versão Firmware
  snprintf(payload, sizeof(payload), 
    "{\"name\":\"Versão Firmware\",\"stat_t\":\"%s\",\"val_tpl\":\"{{ value_json.diagnostics.version }}\",\"ic\":\"mdi:check-decagram\",\"ent_cat\":\"diagnostic\",\"uniq_id\":\"%s_ver\",%s}", 
    topico_telemetria, mac.c_str(), baseJSON.c_str());
  mqttClient.publish((String("homeassistant/sensor/") + mac + "/ver/config").c_str(), payload, true);

  logMonitor("HA Discovery payloads publicados com suporte LWT.");
}

void reconnectMQTT()
{
  logMonitor("Tentando conexao MQTT no IP: " + String(mqtt_server));
  
  String deviceName = getDeviceName();
  // Inclui o nome do dispositivo na ClientID para facilitar a identificação no Broker
  String clientId = deviceName + "-" + String(random(0xffff), HEX);

  // ========================================================
  // CONFIGURAÇÃO DO LAST WILL AND TESTAMENT (LWT)
  // Estrutura atualizada para ser idêntica ao payload principal
  // ========================================================
  String willMessageStr = "{"
    "\"device\":\"" + deviceName + "\","
    "\"status\":\"OFFLINE\","
    "\"data\":{\"temperature\":0.0,\"humidity\":0.0,\"dew_point\":0.0,\"presence\":\"DESCONHECIDO\"},"
    "\"config\":{\"timeout\":0},"
    "\"diagnostics\":{\"ip\":\"\",\"rssi\":0,\"boots\":0,\"hardware\":\"Datacenter Corisco v1.0\",\"mac\":\"\",\"mcu\":\"ESP-12F\",\"free_mem\":0,\"ssid\":\"\",\"teleperiod\":0,\"uptime\":\"\",\"version\":\"Monitor_Ambiental_v1.0\"},"
    "\"timestamp\":\"\""
  "}";
  const char* willMessage = willMessageStr.c_str();
  uint8_t willQos = 0;       // Qualidade de Serviço 0 (padrão)
  boolean willRetain = true; // Define que a mensagem de "morte" deve ficar retida

  // Conecta passando os parâmetros do LWT
  if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_password, topico_telemetria, willQos, willRetain, willMessage))
  {
    logMonitor("-> MQTT Conectado com sucesso!");
    
    setupHADiscovery(); // Cria as entidades no Home Assistant
    mqttClient.subscribe(topico_timeout_set); // Se inscreve para ouvir o slider de configuração
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

  String deviceName = getDeviceName();
  
  // Calcula o tempo real decorrido desde o último envio para refletir no TelePeriod
  unsigned long actualTelePeriod = tempoUltimoEnvioMQTT > 0 ? (millis() - tempoUltimoEnvioMQTT) / 1000 : 0;

  // Buffer aumentado drasticamente para suportar o novo bloco extenso de diagnósticos
  char payload[1536]; 
  
  snprintf(payload, sizeof(payload),
           "{"
           "\"device\":\"%s\","
           "\"status\":\"ONLINE\","
           "\"data\":{"
             "\"temperature\":%.1f,"
             "\"humidity\":%.1f,"
             "\"dew_point\":%.1f,"
             "\"presence\":\"%s\""
           "},"
           "\"config\":{"
             "\"timeout\":%lu"
           "},"
           "\"diagnostics\":{"
             "\"ip\":\"%s\","
             "\"rssi\":%d,"
             "\"boots\":%lu,"
             "\"hardware\":\"Datacenter Corisco v1.0\","
             "\"mac\":\"%s\","
             "\"mcu\":\"ESP-12F\","
             "\"free_mem\":%u,"
             "\"ssid\":\"%s\","
             "\"teleperiod\":%lu,"
             "\"uptime\":\"%s\","
             "\"version\":\"Monitor_Ambiental_v1.0\""
           "},"
           "\"timestamp\":\"%s\""
           "}",
           deviceName.c_str(),
           t, h, orvalho, statusPresenca,
           tempoLampadaLigada / 1000, 
           WiFi.localIP().toString().c_str(),
           WiFi.RSSI(),
           (unsigned long)contadorBoots,
           WiFi.macAddress().c_str(),
           ESP.getFreeHeap(),
           WiFi.SSID().c_str(),
           actualTelePeriod, 
           getUptimeFormatado().c_str(),
           getTimestamp().c_str());

  // O 'true' no final garante que a mensagem normal também fique retida no Broker
  mqttClient.publish(topico_telemetria, payload, true); 
  
  tempoUltimoEnvioMQTT = millis(); // Reseta o cronômetro para iniciar a contagem para o próximo
  logMonitor("JSON Enviado com Diagnosticos -> " + String(payload));
}