/**
 * Monitoramento do Corisco 2026 - MSBOX
 * Módulo: ESP8266 (Nó Sensor - MSBOX)
 * Funcionalidades: Leitura de Temperatura/Umidade (DHT21), Presença (PIR),
 * Cálculo de Ponto de Orvalho, Comunicação MQTT TLS e Servidor HTTP (Dashboard + OTA).
 */
#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <Updater.h>   // Biblioteca Nativa para OTA via Web
#include <LittleFS.h>  // Sistema de arquivos para o certificado SSL
#include <ESP8266httpUpdate.h> // Biblioteca para OTA via HTTP

#include "secrets.h"   // Arquivo externo contendo senhas e IPs (Segurança)
#include "config.h"    // Mapeamento de pinos e tempos do sistema
#include "globals.h"   // Instâncias de rede e variáveis compartilhadas
#include "utils.h"     // Funções matemáticas e de tempo
#include "telemetry.h" // Lógica de envio MQTT e logs
#include <DNSServer.h>
#include <time.h>
#include "webpage.h"   // CÓDIGOS HTML DAS PÁGINAS WEB

#define DHTTYPE DHT21 // Especifica o modelo do sensor DHT
DHT dht(PIN_DHT, DHTTYPE);

// ==========================================
// VARIÁVEIS DE ESTADO E CRONÔMETROS
// ==========================================

// Variáveis para gerenciar o tempo sem travar o processador (Programação-Bloqueante)
bool sistemaCalibrado = false;
bool ntpConfigurado = false;

unsigned long tempoInicioSessao = 0;
unsigned long tempoUltimaPresenca = 0;
unsigned long tempoUltimaLeituraDHT = 0;
unsigned long tempoUltimaTentativaWiFi = 0;
unsigned long tempoUltimaTentativaMQTT = 0;
unsigned long tempoUltimoPisca = 0;

int ultimoRestanteLog = -1;
int tentativasConexaoWiFi = 0;

// Variáveis de memória para a lógica de acionamento MQTT e presença
bool lampadaLigada = false;
float ultimaTempEnviada = -100.0;
float ultimaUmidEnviada = -100.0;

// Váriaveis para armazenar a rede em uso
String ssidAtivo;
String senhaAtiva;

// ==========================================
// ROTAS DO SERVIDOR WEB
// ==========================================

void handleRoot() {
  // Envia a página direto da memória flash (PROGMEM) sem fragmentar a RAM
  server.send_P(200, "text/html", html_dashboard);
}

void handleApiDados() {
  // Salva dados complexos em Strings locais para evitar ponteiros inválidos
  String ip = WiFi.localIP().toString();
  String ssid = WiFi.SSID();
  String mac = WiFi.macAddress();
  String uptime = getUptimeFormatado();
  
  // Buffer leve na memória Stack (não causa fragmentação)
  char json[600];
  snprintf(json, sizeof(json),
    "{\"wifi\":%d,\"mqtt\":\"%s\",\"mqtt_color\":\"%s\",\"presenca\":\"%s\",\"pir_color\":\"%s\",\"timeout\":%lu,\"heartbeat\":%lu,\"temp\":%.1f,\"umid\":%.1f,\"ip\":\"%s\",\"ssid\":\"%s\",\"mac\":\"%s\",\"boots\":%lu,\"uptime\":\"%s\"}",
    WiFi.RSSI(),
    mqttClient.connected() ? "ON" : "OFF",
    mqttClient.connected() ? "green" : "red",
    lampadaLigada ? "MOVIMENTO DETECTADO" : "VAZIO",
    lampadaLigada ? "#e74c3c" : "#2d3748",
    tempoLampadaLigada / 1000,
    intervaloHeartbeat / 1000,
    temperaturaAtual,
    umidadeAtual,
    ip.c_str(),
    ssid.c_str(),
    mac.c_str(),
    (unsigned long)contadorBoots,
    uptime.c_str()
  );
  
  server.send(200, "application/json", json);
}

void handleConf() {
  if (!server.authenticate(http_user, http_password)) return server.requestAuthentication();
  
  String html = FPSTR(html_config);
  html.reserve(2048);
  html.replace("{{TIMEOUT}}", String(tempoLampadaLigada / 1000));
  html.replace("{{HEARTBEAT}}", String(intervaloHeartbeat / 1000));
  server.send(200, "text/html", html);
}

void handleSaveConf() {
  if (!server.authenticate(http_user, http_password)) return server.requestAuthentication();
  
  bool alterouConfig = false;

  if (server.hasArg("timeout")) {
    int novoTimeoutSec = server.arg("timeout").toInt();
    if (novoTimeoutSec >= 10 && novoTimeoutSec <= 3600) {
      unsigned long novoTimeout = novoTimeoutSec * 1000UL;
      if (novoTimeout != tempoLampadaLigada) {
        tempoLampadaLigada = novoTimeout;
        alterouConfig = true;
      }
      tempoUltimoEnvioMQTT = 0; // Força reenvio de telemetria
      logMonitor("-> Timeout alterado via Web para: " + String(novoTimeoutSec) + "s");
    }
  }

  if (server.hasArg("heartbeat")) {
    int novoHeartbeat = server.arg("heartbeat").toInt();
    if (novoHeartbeat >= 10 && novoHeartbeat <= 3600) {
      unsigned long novoIntervalo = novoHeartbeat * 1000UL;
      if (novoIntervalo != intervaloHeartbeat) {
        intervaloHeartbeat = novoIntervalo;
        alterouConfig = true;
      }
      logMonitor("-> Heartbeat alterado via Web para: " + String(novoHeartbeat) + "s");
    }
  }
  
  // Grava no LittleFS apenas se algo mudou (poupa a flash)
  if (alterouConfig) salvarConfig();

  // Redireciona o utilizador de volta para o Dashboard
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}


void iniciarModoAP() {
  Serial.println("\n[ERRO] Falha ao conectar. Iniciando Captive Portal...");
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP("MSBOX-Config", "admin123"); // Nome e senha da rede gerada pelo ESP
  
  Serial.print("Ponto de Acesso Aberto. IP: ");
  Serial.println(WiFi.softAPIP());

  // Inicia o DNS na porta 53 para redirecionar todos os domínios para o IP do ESP
  DNSServer dnsServer;
  dnsServer.start(53, "*", WiFi.softAPIP());

  // Rota raiz que exibe o formulário
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", FPSTR(html_ap_config));
  });

  // Redirecionamento de Captive Portal para dispositivos móveis
  server.onNotFound([]() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
  });

  // Gravação das novas credenciais no LittleFS
  server.on("/salvar_wifi", HTTP_POST, []() {
    String novoSSID = server.arg("ssid");
    String novaSenha = server.arg("senha");

    File f1 = LittleFS.open("/ssid.txt", "w");
    if (f1) { f1.print(novoSSID); f1.close(); }
    
    File f2 = LittleFS.open("/pass.txt", "w");
    if (f2) { f2.print(novaSenha); f2.close(); }

    server.send(200, "text/html", "<h2 style='font-family:sans-serif;text-align:center;padding:50px;'>Credenciais salvas!<br>Reiniciando o MSBOX...</h2>");
    delay(2000);
    ESP.restart();
  });

  // Rota de OTA idêntica à original, mas sem autenticação obrigatória para acesso offline
  server.on("/update_fw", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FALHA NA ATUALIZACAO." : "ATUALIZACAO CONCLUIDA! Reiniciando...");
    
    // Zera o contador se nao houve erro no upload
    if (!Update.hasError()) {
      contadorBoots = 0;
      ESP.rtcUserMemoryWrite(64, (uint32_t*)&contadorBoots, sizeof(contadorBoots));
    }

    delay(1000);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("OTA AP Mode: %s\n", upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace, U_FLASH)) Update.printError(Serial);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) Update.printError(Serial);
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) Serial.printf("OTA Concluido: %u bytes\n", upload.totalSize);
      else Update.printError(Serial);
    }
  });

server.begin();

  // Marca o tempo exato em que o AP foi aberto
  unsigned long inicioAP = millis(); 

  // Trava o sistema neste loop enquanto estiver em modo de manutenção
  while(true) {
    dnsServer.processNextRequest();
    server.handleClient();
    ESP.wdtFeed(); // Mantém o hardware watchdog alimentado
    delay(1);

    // NOVO: Verifica se passaram 10 minutos (600.000 ms) no modo AP
    if (millis() - inicioAP >= 600000) {
      Serial.println("\n[AVISO] Tempo do AP esgotado (10 min). Reiniciando...");
      delay(1000);
      ESP.restart(); // Reinicia e começa de novo, como o professor pediu
    }
  }
}

// ==========================================
// FUNÇÃO PARA EXECUTAR OTA VIA HTTP
// ==========================================
void executarOTAHTTP(String url) {
  if (!url.startsWith("http://") && !url.startsWith("https://")) {
    url = "https://" + url;
  }

  logMonitor("[OTA] Iniciando download do firmware pela URL: " + url);

  // 1. Liberação de memória essencial para a nova conexão TLS funcionar
  logMonitor("[OTA] Desconectando MQTT e Telnet para liberar memoria RAM...");
  
  if (mqttClient.connected()) {
    mqttClient.disconnect();
  }
  espClient.stop(); // Força a destruição do buffer SSL do MQTT
  
  if (telnetClient && telnetClient.connected()) {
    telnetClient.println("\n[OTA] Fechando console para atualizacao. Reconecte apos o boot.");
    telnetClient.stop();
  }
  
  // 2. Aguarda o fechamento dos sockets pelo sistema operacional do ESP
  delay(1000);
  Serial.printf("[OTA] Memoria RAM livre para o download: %u bytes\n", ESP.getFreeHeap());

  t_httpUpdate_return ret;

  // 3. Inicia o download
  if (url.startsWith("https://")) {
    WiFiClientSecure clientSecure;
    clientSecure.setInsecure(); // Ignora a cadeia do certificado (ideal para self-signed/Certbot local)
    ret = ESPhttpUpdate.update(clientSecure, url);
  } else {
    WiFiClient client;
    ret = ESPhttpUpdate.update(client, url);
  }

  // 4. Tratamento do resultado
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      // Imprimimos apenas na Serial pois o Telnet e MQTT estão desligados
      Serial.println("[OTA ERRO] Falha na Atualizacao: " + ESPhttpUpdate.getLastErrorString());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[OTA] Nenhuma atualizacao encontrada no servidor.");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("[OTA] ATUALIZACAO CONCLUIDA! O ESP sera reiniciado...");
      // Zera o contador na memoria RTC antes do reboot
      contadorBoots = 0;
      ESP.rtcUserMemoryWrite(64, (uint32_t*)&contadorBoots, sizeof(contadorBoots));
      break;
  }

  // Reinicia o ESP de forma limpa caso o OTA falhe para restaurar o MQTT e o Telnet
  if (ret != HTTP_UPDATE_OK) {
    Serial.println("[OTA] Reiniciando sistema para restaurar servicos de rede...");
    delay(2000);
    ESP.restart();
  }
}

// ==========================================
// SETUP (Executado apenas uma vez no boot)
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(2500); // Pausa para permitir a conexão do Monitor Serial do PC
  Serial.println("\n\n=== BOOT DO SISTEMA ===");

  // Lógica para ler, incrementar e gravar o Contador de Boots na memória RTC do ESP8266
  ESP.rtcUserMemoryRead(64, (uint32_t*)&contadorBoots, sizeof(contadorBoots));
  
  // Proteção contra lixo de memória caso a placa nunca tenha sido gravada
  if (contadorBoots > 1000000) contadorBoots = 0; 
  contadorBoots++;
  ESP.rtcUserMemoryWrite(64, (uint32_t*)&contadorBoots, sizeof(contadorBoots));

  // Configuração dos Pinos
  pinMode(PIN_PRESENCE, INPUT_PULLUP);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  dht.begin(); // Inicializa o sensor de temperatura

// ========================================
  // INICIALIZA O LITTLEFS (Apenas para credenciais de emergência)
  // ========================================
  if (!LittleFS.begin()) {
    Serial.println("[AVISO] Falha ao montar o LittleFS! Formatando a partição...");
    
    if (LittleFS.format()) {
      Serial.println("[INFO] LittleFS formatado com sucesso!");
      LittleFS.begin(); // Tenta montar novamente após formatar
    } else {
      Serial.println("[CRÍTICO] Não foi possível formatar o LittleFS. As credenciais web não serão salvas.");
    }
  }

  // Restaura timeout e heartbeat salvos (se houver); senão mantém os padrões
  carregarConfig();

  // ========================================
  // CARREGA O CERTIFICADO SSL EMBUTIDO NO CÓDIGO
  // ========================================
  X509List *cert = new X509List(ca_cert);
  espClient.setTrustAnchors(cert);
  
  // Validação estrita ATIVADA: o ESP só conecta se o broker apresentar um certificado
  // assinado pela CA embutida em secrets.cpp (e dentro da validade, por isso exige NTP).
  
  // O nome em mqtt_server também é conferido contra o certificado do broker.
  // ========================================

  // Configuração do MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setBufferSize(1536); 
  mqttClient.setCallback(mqttCallback); 

  // ========================================
  // CARREGAMENTO DE CREDENCIAIS E FALLBACK
  // ========================================
  ssidAtivo = String(ssid);       // Padrão do secrets.cpp
  senhaAtiva = String(password);  // Padrão do secrets.cpp

  // Sobrescreve com o que estiver gravado no LittleFS
  if (LittleFS.exists("/ssid.txt")) {
    File f = LittleFS.open("/ssid.txt", "r");
    ssidAtivo = f.readString();
    ssidAtivo.trim();
    f.close();
  }
  if (LittleFS.exists("/pass.txt")) {
    File f = LittleFS.open("/pass.txt", "r");
    senhaAtiva = f.readString();
    senhaAtiva.trim();
    f.close();
  }

  WiFi.mode(WIFI_STA);
  inicializarTopicosMQTT(); // Inicializa os tópicos MQTT com o MAC dinâmico
  WiFi.begin(ssidAtivo.c_str(), senhaAtiva.c_str());
  
  Serial.print("Conectando ao Wi-Fi: ");
  Serial.println(ssidAtivo);

  // Aguarda até 900 segundos (15 minutos) pela rede
  int tempoEspera = 0;
  while (WiFi.status() != WL_CONNECTED && tempoEspera < 900) {
    delay(1000);
    Serial.print(".");
    tempoEspera++;
    ESP.wdtFeed(); // Alimenta o Watchdog para evitar reset durante a espera
  }

  // Se esgotou o tempo e não conectou, sobe a rede de emergência
  if (WiFi.status() != WL_CONNECTED) {
    iniciarModoAP(); 
  } else {
    Serial.println("\n[WIFI] Conectado com sucesso!");
  }

  telnetServer.begin();

  // ----------------------------------------
  // DEFINIÇÃO DAS ROTAS HTTP DO SERVIDOR WEB
  // ----------------------------------------
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/dados", HTTP_GET, handleApiDados);
  server.on("/conf", HTTP_GET, handleConf);
  server.on("/save_conf", HTTP_POST, handleSaveConf);

  // Rota para mostrar a página de Atualização (GET)
  server.on("/update", HTTP_GET, []() {
    if (!server.authenticate(http_user, http_password)) return server.requestAuthentication();
    server.send(200, "text/html", FPSTR(html_update));
  });

  // Rota para processar a receção do Firmware (POST)
  server.on("/update_fw", HTTP_POST, []() {
    if (!server.authenticate(http_user, http_password)) return server.requestAuthentication();
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FALHA NA ATUALIZACAO. Tente novamente." : "ATUALIZACAO CONCLUIDA! O ESP sera reiniciado em instantes.");
    
    // Zera o contador se nao houve erro no upload
    if (!Update.hasError()) {
      contadorBoots = 0;
      ESP.rtcUserMemoryWrite(64, (uint32_t*)&contadorBoots, sizeof(contadorBoots));
    }

    delay(1000);
    ESP.restart();
  }, []() {
    // Bloco de Execução Interna durante o Upload do Ficheiro
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("\nIniciando OTA Web: %s\n", upload.filename.c_str());
      
      // Configura o espaço para receber apenas Firmware (U_FLASH) e bloqueia SPIFFS
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace, U_FLASH)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("OTA Concluido. Tamanho: %u bytes\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.begin();
  Serial.println("Servidor Web Iniciado na porta 80");

  // Ativa o Watchdog de software do ESP8266 com tempo padrão
  ESP.wdtEnable(0);
  tempoInicioSessao = millis(); // Marca a hora que o dispositivo ligou
}

// ==========================================
// LOOP PRINCIPAL (Executado continuamente)
// ==========================================
void loop() {
  // Alimenta o Watchdog no início de cada ciclo para evitar resets indesejados
  ESP.wdtFeed();
 
  // VERIFICA SE HÁ UM OTA PENDENTE
  if (pendenteOTA) {
    pendenteOTA = false; // Limpa a flag
    executarOTAHTTP(urlOTA); // Trava o loop para baixar e instalar
  }

  unsigned long tempoAtual = millis(); // Captura o tempo atual em cada ciclo

  // Escuta requisições HTTP (Abre o site)
  server.handleClient();

  // ----------------------------------------
  // ETAPA 1: GESTÃO DE REDE E SERVIÇOS
  // ----------------------------------------
  if (WiFi.status() != WL_CONNECTED) {
    // Tenta reconectar ao Wi-Fi a cada 10 segundos sem travar o loop
    if (tempoAtual - tempoUltimaTentativaWiFi >= INTERVALO_RECONEXAO_WIFI) {
      WiFi.begin(ssidAtivo.c_str(), senhaAtiva.c_str());
      tempoUltimaTentativaWiFi = tempoAtual;
      tentativasConexaoWiFi++;
      Serial.println("[WIFI] Tentativa de reconexao: " + String(tentativasConexaoWiFi));

      if (tentativasConexaoWiFi >= 90) {
        Serial.println("[ERRO] Falha ao reconectar ao Wi-Fi após 15 minutos. Reiniciando o sistema...");
        delay(1000);  // Pausa breve para garantir a exibição do log serial
        ESP.restart(); // Reinicia o microcontrolador
      }
    }
  } else { 
    // Se o Wi-Fi estiver conectado:
    if (tentativasConexaoWiFi > 0) {
      tentativasConexaoWiFi = 0; // Reseta o contador ao conectar com sucesso
    }

    // Sincroniza o relógio na primeira vez que conectar (Fuso UTC-3)
    if (!ntpConfigurado) {
      configTime(-10800, 0, "pool.ntp.org", "time.nist.gov");
      ntpConfigurado = true;
    }

    // Mantém a conexão MQTT viva ou tenta reconectar
    if (!mqttClient.connected()) {
      if (tempoAtual - tempoUltimaTentativaMQTT >= INTERVALO_RECONEXAO_MQTT) {
        tempoUltimaTentativaMQTT = tempoAtual;
        // A validação do certificado TLS depende do relógio: aguarda o NTP sincronizar
        if (time(nullptr) < 100000) {
          logMonitor("[MQTT] Aguardando sincronizacao NTP para validar o certificado TLS...");
        } else {
          reconnectMQTT();
        }
      }
    } else {
      mqttClient.loop(); // Processa mensagens MQTT em background
    }

    // Gerencia conexões Telnet de entrada para debug remoto
    if (telnetServer.hasClient()) {
      if (!telnetClient || !telnetClient.connected()) {
        if (telnetClient) {
          telnetClient.stop();
        }
        telnetClient = telnetServer.accept();
        telnetClient.println("=== Console MSBOX Conectado ===");
      }
    }
  }

  // ----------------------------------------
  // ETAPA 2: AQUECIMENTO E CALIBRAÇÃO DO PIR
  // ----------------------------------------
  // O sensor PIR precisa de ~80s lendo o ambiente antes de enviar dados confiáveis
  if (!sistemaCalibrado) {
    // Pisca o LED Verde a cada 500ms para indicar que está em calibração
    if (tempoAtual - tempoUltimoPisca >= 500) {
      tempoUltimoPisca = tempoAtual;
      digitalWrite(PIN_LED_GREEN, !digitalRead(PIN_LED_GREEN));

      long restante = (DURACAO_CALIBRACAO - (tempoAtual - tempoInicioSessao)) / 1000;
      if (restante >= 0 && restante % 10 == 0 && restante != ultimoRestanteLog) { // Imprime no log a cada 10s
        ultimoRestanteLog = restante;
        logMonitor("Calibrando PIR... " + String(restante) + "s restantes.");
      }
    }

    // Finaliza a calibração
    if (tempoAtual - tempoInicioSessao >= DURACAO_CALIBRACAO) {
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
  
  // A. EVENTOS DO SENSOR DE PRESENÇA
  if (digitalRead(PIN_PRESENCE) == LOW) { // LOW indica movimento detectado
    tempoUltimaPresenca = tempoAtual; // Renova o cronômetro de presença
    if (!lampadaLigada) {
      digitalWrite(PIN_LED_GREEN, HIGH); // Acende indicador
      lampadaLigada = true;
      logMonitor("[EVENTO] Movimento detectado.");
      
      // Força o envio imediato dos dados do clima atual junto com o alerta de movimento
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      if (!isnan(h) && !isnan(t)) {
        ultimaTempEnviada = t;
        ultimaUmidEnviada = h;
        enviarPayloadJSON(t, h, calcularPontoOrvalho(t, h), "ON");
      }
    }
  }

  // Desativa a presença após o tempo limite configurado sem detectar movimento
  if (lampadaLigada && (tempoAtual - tempoUltimaPresenca >= tempoLampadaLigada)) {
    digitalWrite(PIN_LED_GREEN, LOW);
    lampadaLigada = false;
    logMonitor("[INFO] Ambiente vazio.");
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) {
      ultimaTempEnviada = t;
      ultimaUmidEnviada = h;
      enviarPayloadJSON(t, h, calcularPontoOrvalho(t, h), "OFF");
    }
  }

// B. EVENTOS DO SENSOR DE CLIMA (Leitura Baseada em Tempo e Variação)
  if (tempoAtual - tempoUltimaLeituraDHT >= INTERVALO_LEITURA_DHT) {
    tempoUltimaLeituraDHT = tempoAtual;
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Verifica se a leitura foi bem-sucedida (não Not-A-Number)
    if (!isnan(h) && !isnan(t)) {
      bool emAlerta = (t >= TEMP_ALERTA);
      digitalWrite(PIN_LED_RED, emAlerta ? HIGH : LOW); // Acende LED vermelho se houver superaquecimento
      
      // Salva as leituras nas variáveis globais para a página Web mostrar em tempo real
      temperaturaAtual = t;
      umidadeAtual = h;
      
      // LÓGICA DE ECONOMIA DE REDE E CONTROLE DE SPAM DE ALERTA
      
      // 1. Verifica se acabou de entrar ou sair do estado de emergência
      static bool estadoAlertaAnterior = false;
      bool mudouEstadoAlerta = (emAlerta != estadoAlertaAnterior);
      estadoAlertaAnterior = emAlerta;
      
      // 2. Verifica se houve aumento/queda de pelo menos 0.5 graus ou % de umidade
      bool mudancaTemp = (abs(t - ultimaTempEnviada) >= 0.5);
      bool mudancaUmid = (abs(h - ultimaUmidEnviada) >= 0.5);
      bool variacaoSignificativa = (mudancaTemp || mudancaUmid);
      
      // 3. Cronômetros de envio
      bool estourouHeartbeat = (tempoAtual - tempoUltimoEnvioMQTT >= intervaloHeartbeat);
      bool tempoAlertaPassou = (tempoAtual - tempoUltimoEnvioMQTT >= 10000); // 10 segundos (5 leituras de 2s)

      // Regras de disparo baseadas no estado atual
      bool dispararNormal = (!emAlerta && variacaoSignificativa);
      bool dispararAlerta = (emAlerta && variacaoSignificativa && tempoAlertaPassou);

      // O pacote é enviado se acontecer QUALQUER UMA destas 4 coisas:
      // - A sala acabou de cruzar a linha do alerta (para cima ou para baixo)
      // - Estourou o tempo máximo de silêncio (Heartbeat de 60s)
      // - Em modo Normal, variou 0.5
      // - Em modo Alerta, variou 0.5 E já se passaram 10 segundos
      if (mudouEstadoAlerta || estourouHeartbeat || dispararNormal || dispararAlerta) {
        ultimaTempEnviada = t;
        ultimaUmidEnviada = h;
        float orvalho = calcularPontoOrvalho(t, h);
        enviarPayloadJSON(t, h, orvalho, lampadaLigada ? "ON" : "OFF");
      }
    }
  }
}
