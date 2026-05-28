#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>

void logMonitor(String msg);
void reconnectMQTT();
void enviarPayloadJSON(float t, float h, float orvalho, const char *statusPresenca);

#endif