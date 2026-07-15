#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>

void logMonitor(String msg);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void setupHADiscovery();
void reconnectMQTT();
void enviarPayloadJSON(float t, float h, float orvalho, const char *statusPresenca);

#endif
