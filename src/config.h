#ifndef CONFIG_H
#define CONFIG_H

// Mapeamento de Pinos
#define PIN_PRESENCE 4
#define PIN_LED_GREEN 16
#define PIN_LED_RED 2
#define PIN_DHT 14

// Configurações de Tempo
const unsigned long DURACAO_CALIBRACAO = 80000;
const unsigned long TEMPO_LAMPADA_LIGADA = 15000;
const unsigned long INTERVALO_LEITURA_DHT = 2000;
const unsigned long INTERVALO_RECONEXAO_WIFI = 10000;
const unsigned long INTERVALO_RECONEXAO_MQTT = 10000;
const unsigned long INTERVALO_HEARTBEAT = 60000; // 1 minuto para envio fixo (estabilidade)
const float TEMP_ALERTA = 25.0;

#endif