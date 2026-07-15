#include "utils.h"
#include <math.h>
#include <time.h>

float calcularPontoOrvalho(float temp, float umid)
{
  float a = 17.271;
  float b = 237.7;
  float gama = (a * temp) / (b + temp) + log(umid / 100.0);
  float pontoOrvalho = (b * gama) / (a - gama);
  return pontoOrvalho;
}

String getTimestamp()
{
  time_t now = time(nullptr);
  if (now < 100000)
    return "1970-01-01T00:00:00";
  struct tm *timeinfo = localtime(&now);
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", timeinfo);
  return String(buffer);
}

// Formata o tempo de atividade do sistema para diagnósticos
String getUptimeFormatado()
{
  unsigned long sec = millis() / 1000;
  unsigned long min = sec / 60;
  unsigned long hr = min / 60;
  unsigned long days = hr / 24;
  char buffer[30];
  snprintf(buffer, sizeof(buffer), "%luT%02lu:%02lu:%02lu", days, hr % 24, min % 60, sec % 60);
  return String(buffer);
}
