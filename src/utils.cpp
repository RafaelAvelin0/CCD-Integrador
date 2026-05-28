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