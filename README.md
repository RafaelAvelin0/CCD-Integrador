# 🌡️ Sistema de Monitoramento Integrado de Climatização Inteligente

![Status](https://img.shields.io/badge/Status-Em%20Desenvolvimento-yellow)
![Plataforma](https://img.shields.io/badge/Plataforma-ESP8266-blue)
![Linguagem](https://img.shields.io/badge/Linguagem-Arduino-00599C)
![Licença](https://img.shields.io/badge/Licença-MIT-green)

**Corisco Centro de Dados**

Este repositório contém o código-fonte e a documentação técnica do sistema IoT de monitoramento ambiental e eficiência energética desenvolvido para o Corisco Centro de Dados (nó sensor **MSBOX**).

## 🎓 Identificação do Projeto

* **Instituição:** IFRN - Campus Natal Central
* **Área:** Projeto Integrador - Redes de Computadores
* **Período de Execução:** 01/04/2026 a 30/06/2026
* **Equipe:**
  * Clenylson Perycles de Oliveira
  * José Bezerra Filho
  * Júlia Rubiane Ferreira de Macêdo
  * Lucas Ribeiro Sales
  * Rafael Avelino dos Santos

## 📋 Resumo Executivo

Este projeto apresenta uma arquitetura híbrida de controle ambiental e eficiência energética para o **Corisco Centro de Dados**. A proposta substitui o rodízio puramente mecânico por uma gestão inteligente via sinal IR e protocolos de rede, utilizando sensores de precisão e centralização de dados no **Home Assistant**.

Ao integrar a automação IoT com a infraestrutura existente (equipamentos Agratto e TCL), o sistema otimiza o funcionamento dos compressores *inverter*, oferece um painel de controle para mitigação de falhas e análise de consumo, e assegura a integridade dos ativos de TI (evitando *down-time* por superaquecimento).

## 🗺️ Roadmap (To-Do List)

O desenvolvimento está dividido em fases. Acompanhe o progresso atual das implementações:

- [x] **Sensoriamento Básico:** Leitura contínua de temperatura, umidade e ponto de orvalho (AM2301).
- [x] **Automação de Iluminação:** Controle via sensor de presença (PIR) e rotinas de *timeout*.
- [x] **Console e Manutenção:** Espelhamento de logs via Telnet e atualizações OTA (Over-The-Air).
- [x] **Integração Home Assistant:** Comunicação via MQTT para centralização dos dados e controle do painel principal.
- [x] **Formatação:** Os dados são enviados em JSON para o broker (facilita a integração no Home Assistant).
- [x] **Autodescoberta no Home Assistant (MQTT Discovery):** Todas as entidades (sensores, diagnósticos e controles) são criadas automaticamente, agrupadas em um único dispositivo.
- [x] **Dashboard Web Local:** Painel em tempo real e página de configurações servidos pelo próprio ESP8266.
- [x] **Configuração Remota:** Timeout de presença e intervalo de heartbeat ajustáveis via Home Assistant (MQTT) ou via painel web.
- [x] **OTA em Múltiplos Formatos:** Upload do `.bin` pela página web e atualização remota via URL enviada por MQTT.
- [x] **Modo de Recuperação (Captive Portal):** Rede Wi-Fi própria para reconfigurar credenciais e gravar firmware offline quando a rede principal falha.
- [x] **MQTT sobre TLS:** Conexão criptografada com o broker e autenticação HTTP Basic nas páginas administrativas.
- [x] **Diagnósticos Embarcados:** Contador de boots (memória RTC), uptime, memória livre, RSSI, versão do firmware, etc.
- [x] **Controle do Ar-Condicionado (IR):** Gerenciamento dos aparelhos Agratto e TCL por automação no Home Assistant.
- [x] **Acionamento de Potência:** Interruptor inteligente Wi-Fi 16A integrado ao Home Assistant; uma automação acende e apaga as luzes a partir da presença detectada.
- [ ] **Aprimoramento de Segurança (parcial):** TLS e autenticação web já implementados; falta ativar a validação do certificado do broker (hoje desativada por `setInsecure()`), HTTPS no painel e verificação de assinatura do firmware (ver [Segurança](#-segurança)).
- [ ] **Persistência de Configuração:** Timeout e heartbeat alterados pelo painel web ou pelo Home Assistant ficam apenas na RAM e voltam ao padrão (600 s e 60 s) quando o ESP reinicia. Apenas as credenciais de Wi-Fi do modo de recuperação são gravadas em flash.

## 🚀 Recursos e Escopo Técnico

* **Microcontrolador Central:** Módulo **ESP-12F** (ESP8266) para processamento local e comunicação Wi-Fi.
* **Monitoramento Ambiental de Precisão:** Leitura a cada 2 segundos de temperatura e umidade via sensor AM2301 (DHT21), com cálculo do ponto de orvalho.
* **Iluminação Automatizada (PIR):** O módulo detecta movimento e mantém o estado de presença ligado até o fim do *timeout* (padrão de 10 minutos, ajustável entre **10 s e 3600 s**). O LED Verde espelha esse estado localmente, e o Home Assistant usa a entidade de presença para acender e apagar as luzes pelo interruptor inteligente.
* **Alertas Visuais:** LED Vermelho acende quando a temperatura atinge **28 °C** (`TEMP_ALERTA` em `config.h`). O estado de alerta também muda a frequência de envio da telemetria.
* **Arquitetura de Controle:** O ESP8266 atua como **nó sensor**: lê, calcula e publica via MQTT. O acionamento do interruptor inteligente (luzes) e o controle IR do ar-condicionado são feitos por **automações no Home Assistant**, não pelo firmware deste módulo.
* **Integração Home Assistant:** Publicação via MQTT com *discovery* automático, *Last Will and Testament* e disponibilidade das entidades baseada no campo `status`.
* **Dashboard Web:** Painel com temperatura, umidade, estado do PIR, sinal Wi-Fi, status MQTT, boots e uptime, atualizado a cada 5 segundos.
* **Resiliência de Rede:** Reconexão de Wi-Fi e MQTT a cada 10 segundos sem bloquear o `loop()`. Após 90 tentativas de Wi-Fi sem sucesso (~15 min) o sistema reinicia sozinho. *Watchdog* de software ativo.
* **Modo de Recuperação (AP + Captive Portal):** Se o Wi-Fi não conectar no boot (espera de até 15 min), o ESP abre a rede `MSBOX-Config`, redireciona qualquer domínio para o formulário de configuração e reinicia automaticamente após 10 minutos sem uso.
* **Console Remoto (Telnet):** Espelhamento dos *logs* na porta 23 (aceita uma conexão por vez), também espelhados na Serial.
* **Atualização OTA (Over-The-Air):** Três caminhos disponíveis: página web (`/update`), modo de recuperação e URL remota via MQTT.
* **Máquina de Estados:** Calibração não-bloqueante de 80 s no *boot* (LED verde piscando) para evitar falsos positivos do PIR.
* **Contador de Boots:** Armazenado na memória RTC e zerado após cada atualização de firmware bem-sucedida.
* **Heartbeat MQTT:** Envio periódico (padrão **60 s**, configurável de 10 a 3600 s), mesmo sem mudança significativa.

### Critérios de envio da telemetria

O sistema publica no broker quando ocorre **qualquer uma** das situações abaixo:

| Situação | Regra |
| :--- | :--- |
| Mudança de estado de alerta | Temperatura cruzou o limite de 28 °C (subindo ou descendo) |
| Variação em modo normal | Temperatura **ou** umidade variou ≥ **0,5** desde o último envio |
| Variação em modo alerta | Variação ≥ 0,5 **e** já se passaram ≥ 10 s desde o último envio (evita spam) |
| Heartbeat | Tempo máximo de silêncio expirou (padrão 60 s) |
| Evento de presença | Movimento detectado ou ambiente vazio (envio imediato) |
| Configuração alterada | Timeout ou heartbeat alterados (via web ou MQTT) forçam novo envio |

## 📌 Pinagem (Hardware)

| **Componente** | **Pino ESP-12F** | **Lógica** |
| :--- | :---: | :--- |
| **Sensor de Presença (PIR)** | GPIO 04 | Ativo Baixo (LOW = detectado) |
| **LED Verde (Iluminação/OK)** | GPIO 16 | Ativo Alto (HIGH = aceso) |
| **LED Vermelho (Alerta/Falha)**| GPIO 02 | Ativo Alto (HIGH = aceso) |
| **Sensor Temp/Umi (AM2301)** | GPIO 14 | Protocolo One-Wire |

> *Nota: O interruptor inteligente Wi-Fi 16A e o controle infravermelho (IR) do ar-condicionado não usam pinos do ESP-12F. São comandados pelo Home Assistant a partir dos dados publicados por este módulo.*

## 🗂️ Estrutura do Projeto

```text
src/
├── main.cpp           # setup/loop, rotas web, modo AP, OTA (web e HTTP)
├── telemetry.cpp/.h   # MQTT, LWT, HA Discovery, callback de comandos, logs
├── globals.cpp/.h     # Clientes/servidores de rede e variáveis compartilhadas
├── utils.cpp/.h       # Ponto de orvalho, timestamp (NTP) e uptime
├── config.h           # Pinos, tempos e limite de alerta térmico
├── webpage.h          # HTML das páginas (PROGMEM)
├── secrets.h          # Declarações (extern) das credenciais
├── secrets.cpp        # Credenciais reais (ignorado pelo git)
└── secrets-modelo.cpp # Modelo para criar o secrets.cpp
```

## 🌐 Interface Web

O ESP8266 serve um painel na porta **80**. As rotas administrativas exigem autenticação HTTP Basic (`http_user` / `http_password` do `secrets.cpp`).

| Rota | Método | Autenticação | Descrição |
| :--- | :---: | :---: | :--- |
| `/` | GET | Não | Dashboard em tempo real |
| `/api/dados` | GET | Não | JSON com os dados exibidos no dashboard |
| `/conf` | GET | Sim | Formulário de timeout e heartbeat |
| `/save_conf` | POST | Sim | Salva as configurações (valores de 10 a 3600 s) |
| `/update` | GET | Sim | Página de upload do firmware |
| `/update_fw` | POST | Sim | Recebe o `.bin`, grava e reinicia |

### Modo de Recuperação

Quando o Wi-Fi principal não conecta, o dispositivo abre a rede **`MSBOX-Config`** (a senha está definida em `main.cpp`; altere antes de colocar em produção). Ao conectar, o captive portal oferece:

* **Configurar nova rede:** SSID e senha são gravados no LittleFS (`/ssid.txt` e `/pass.txt`) e têm prioridade sobre os valores do `secrets.cpp` nos próximos boots.
* **Atualização offline (OTA):** Upload do `.bin` sem autenticação (acesso restrito à rede de recuperação).

Sem interação, o dispositivo reinicia após 10 minutos e tenta a rede principal novamente.

## 📡 Telemetria MQTT

O tópico é montado dinamicamente com os 4 últimos dígitos do MAC, permitindo vários módulos no mesmo broker:

| Finalidade | Tópico | Direção |
| :--- | :--- | :---: |
| Telemetria (JSON, retain) | `ha/corisco/msbox_<xxxx>/telemetria` | ESP → Broker |
| Alterar timeout de presença (s) | `ha/corisco/msbox_<xxxx>/timeout/set` | Broker → ESP |
| Alterar heartbeat (s) | `ha/corisco/msbox_<xxxx>/heartbeat/set` | Broker → ESP |
| URL para OTA remoto | `ha/corisco/msbox_<xxxx>/ota/url` | Broker → ESP |

A conexão com o broker é feita via **TLS** (porta definida em `secrets.cpp`) com usuário e senha. O buffer MQTT foi ampliado para 1536 bytes para comportar o JSON completo.

### Exemplo de payload enviado

```json
{
  "device": "ESP8266-A1B2-MSBOX",
  "status": "ONLINE",
  "data": {
    "temperature": 23.4,
    "humidity": 58.2,
    "dew_point": 14.5,
    "presence": "ON"
  },
  "config": {
    "timeout": 600,
    "heartbeat": 60
  },
  "diagnostics": {
    "ip": "192.168.1.50",
    "rssi": -61,
    "boots": 3,
    "hardware": "MSBOX v1.0",
    "mac": "AA:BB:CC:DD:A1:B2",
    "mcu": "ESP-12F",
    "free_mem": 21840,
    "ssid": "REDE_WIFI",
    "teleperiod": 60,
    "uptime": "0T02:14:09",
    "version": "Monitor_Ambiental_v1.0.1"
  },
  "timestamp": "2026-05-13T14:22:31-03:00"
}
```

> O campo `uptime` segue o formato `diasT hh:mm:ss`. O `teleperiod` indica o tempo real (em segundos) decorrido desde o envio anterior.

## 🏠 Integração com o Home Assistant (MQTT Discovery)

A cada conexão com o broker, o firmware publica (com *retain*) as configurações de descoberta em `homeassistant/<tipo>/<MAC>/<entidade>/config`. Todas as entidades aparecem agrupadas em um único dispositivo (fabricante `IFRN-DIATINF`, modelo `ESP-12F`) e ficam indisponíveis automaticamente quando o status é `OFFLINE`.

| Categoria | Entidades |
| :--- | :--- |
| **Sensores** | Temperatura, Umidade, Ponto de Orvalho (com `state_class: measurement` para histórico/gráficos) |
| **Binários** | Presença (`motion`), Conexão do Sistema (`connectivity`) |
| **Controles (`number`)** | Timeout Presença e Heartbeat Telemetria (10 a 3600 s) |
| **Diagnóstico** | Última Leitura, Dispositivo MSBOX, Sinal Wi-Fi, Endereço IP, Contador de Boots, Hardware, MAC Address, MCU, Memória Livre, Rede Wi-Fi, TelePeriod, Uptime, Versão Firmware |

### Automações no Home Assistant

As ações sobre o ambiente ficam no Home Assistant, usando as entidades criadas pelo discovery:

* **Iluminação:** quando a entidade **Presença** vai para `ON`, uma automação liga as luzes pelo interruptor inteligente; quando volta para `OFF` (fim do *timeout* configurado), a automação apaga as luzes.
* **Ar-condicionado:** os aparelhos Agratto e TCL são controlados por automação no Home Assistant via infravermelho.

> As automações ficam salvas no próprio Home Assistant, não neste repositório.

### Atualização remota via MQTT

Publicar uma URL no tópico `.../ota/url` faz o dispositivo baixar e instalar o firmware. O ESP desconecta o MQTT e o Telnet para liberar RAM, baixa o `.bin` (HTTP ou HTTPS; se o esquema for omitido, assume `https://`), zera o contador de boots em caso de sucesso e reinicia. Em caso de falha, reinicia para restaurar os serviços.

```bash
mosquitto_pub -h <broker> -p <porta> --cafile ca.crt -u <usuario> -P <senha> \
  -t "ha/corisco/msbox_a1b2/ota/url" -m "https://servidor/firmware.bin"
```

## Last Will and Testament (LWT)

Caso o ESP8266 perca energia ou conexão inesperadamente, o broker publica automaticamente, **no mesmo tópico de telemetria**, uma mensagem com a mesma estrutura do payload principal e `status: OFFLINE`:

```json
{
  "device": "ESP8266-A1B2-MSBOX",
  "status": "OFFLINE",
  "data": { "temperature": 0.0, "humidity": 0.0, "dew_point": 0.0, "presence": "DESCONHECIDO" },
  "config": { "timeout": 0, "heartbeat": 0 },
  "diagnostics": { "ip": "", "rssi": 0, "boots": 0, "hardware": "MSBOX v1.0", "mac": "", "mcu": "ESP-12F", "free_mem": 0, "ssid": "", "teleperiod": 0, "uptime": "", "version": "Monitor_Ambiental_v1.0" },
  "timestamp": ""
}
```

> As mensagens são publicadas com flag *retain*, permitindo que o Home Assistant visualize imediatamente o último estado conhecido do dispositivo. É esse campo `status` que o Home Assistant usa para marcar as entidades como indisponíveis.

## 🔐 Configuração de credenciais

O projeto usa um arquivo externo chamado `secrets.cpp`, criado a partir do modelo `secrets-modelo.cpp`.

Antes de compilar:

1. renomeie `secrets-modelo.cpp` para `secrets.cpp`;
2. preencha SSID e senha do Wi-Fi;
3. informe os dados do broker MQTT (servidor, porta, usuário e senha);
4. defina o usuário e a senha do painel web (`http_user` / `http_password`);
5. cole o certificado da CA do broker na variável `ca_cert`.

> Observações:
> * `mqtt_port` deve ser um **número inteiro**, por exemplo `8883`.
> * O **IP estático** está desativado no código atual (o dispositivo usa DHCP). O bloco correspondente está comentado em `secrets.h`/`secrets.cpp`; para usá-lo, descomente-o e chame `WiFi.config(...)` antes do `WiFi.begin(...)`. Se usar DHCP, recomenda-se reservar o IP do MSBOX no roteador.
> * As credenciais de Wi-Fi salvas pelo modo de recuperação (LittleFS) sobrescrevem as do `secrets.cpp`.
> * **Nunca** envie o `secrets.cpp` ao repositório (ele deve constar no `.gitignore`).

## 🔒 Segurança

Já implementado:

* MQTT sobre **TLS** com usuário e senha;
* Credenciais fora do código versionado (`secrets.cpp` no `.gitignore`);
* Autenticação HTTP Basic em `/conf`, `/save_conf`, `/update` e `/update_fw`.

Pontos de atenção conhecidos (a tratar nas próximas versões):

* O certificado da CA é carregado, mas logo em seguida o firmware chama `setInsecure()`, que faz o ESP **aceitar qualquer certificado**. A conexão é criptografada, porém a identidade do broker não é verificada (risco de ataque *man-in-the-middle*). Para ativar a validação, remova as chamadas a `setInsecure()` em `main.cpp` e confirme que o nome em `mqtt_server` consta no certificado do broker e que o relógio (NTP) está sincronizado antes da conexão.
* O painel web usa HTTP (sem TLS): a autenticação Basic trafega sem criptografia. Use apenas em rede interna confiável.
* `/api/dados` é público e expõe IP, SSID e MAC.
* O OTA via MQTT/URL não valida a cadeia TLS nem a assinatura do firmware; restrinja no broker (ACL) quem pode publicar em `.../ota/url`.
* O modo de recuperação usa senha de AP definida no código e o upload de firmware nele é sem autenticação.

## 🛠️ Como Instalar e Compilar

### Pré-requisitos

* [Visual Studio Code](https://code.visualstudio.com/)
* Extensão [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) instalada no VS Code.

### Passo a Passo de Configuração

1. Clonar o repositório:
```bash
git clone https://github.com/RafaelAvelin0/CCD-Integrador.git
```

2. **Configurar as credenciais (Segurança):**

    * Navegue até a pasta `src/`.
    * Renomeie o arquivo `secrets-modelo.cpp` para `secrets.cpp`.
    * Edite o `secrets.cpp` conforme a seção [Configuração de credenciais](#-configuração-de-credenciais).
    * _O arquivo `secrets.cpp` está incluído no `.gitignore` e não será compartilhado publicamente._

3. **Primeira Gravação (Via Cabo USB-Serial):**

    * Conecte o ESP-12F via cabo ao computador (necessário conversor FTDI caso seja módulo _standalone_).
    * Coloque a placa em **Modo Flash**: segure o botão `GPIO 0`, aperte e solte o botão `RESET`, e em seguida solte o `GPIO 0`.
    * No PlatformIO (VS Code), clique no ícone **Upload (➔)** na barra inferior.

4. **Gravações Seguintes (OTA - Over-The-Air):**

    Uma vez gravado o código base e conectado à rede, não é mais necessário o cabo. Gere o `.bin` com **Build** no PlatformIO (`.pio/build/<ambiente>/firmware.bin`) e escolha uma das opções:

    * **Pelo navegador:** acesse `http://<IP_DO_ESP>/update`, autentique-se e envie o arquivo `.bin`;
    * **Por MQTT:** hospede o `.bin` em um servidor web e publique a URL no tópico `.../ota/url`;
    * **Pelo modo de recuperação:** se o dispositivo estiver sem rede, conecte-se a `MSBOX-Config` e use o formulário de atualização offline.

## 💻 Acesso ao console remoto

Com o dispositivo conectado à rede, é possível acompanhar os logs via Telnet ou monitor serial do VSCode:

```bash
nc <IP_DO_ESP> 23
```

Substitua `<IP_DO_ESP>` pelo endereço do dispositivo (visível no dashboard, no Home Assistant ou na lista de clientes do roteador). Durante uma atualização OTA via URL, o console Telnet é encerrado e deve ser reconectado após o reboot.

## 🧩 Dependências usadas no código

* `Arduino.h`
* `DHT.h`
* `ESP8266WiFi.h` / `WiFiClientSecure.h`
* `PubSubClient.h`
* `ESP8266WebServer.h`
* `DNSServer.h` (captive portal)
* `LittleFS.h` (credenciais de emergência)
* `Updater.h` (OTA via web)
* `ESP8266httpUpdate.h` (OTA via URL)
* `math.h`
* `time.h`

## 📎 Observações técnicas

* O sistema usa `millis()` para evitar bloqueios no `loop()`.
* O horário é obtido via NTP (`pool.ntp.org` / `time.nist.gov`, fuso UTC-3) e formatado em ISO 8601.
* O envio MQTT usa buffer ampliado (1536 bytes) para suportar o JSON com diagnósticos.
* O console Telnet aceita apenas uma conexão por vez.
* As páginas HTML ficam em `PROGMEM` (`webpage.h`) e são enviadas direto da flash para reduzir o uso de RAM.
* O contador de boots usa a memória RTC do ESP8266 (posição 64) e é zerado após atualização de firmware.
* Timeout de presença e heartbeat alterados em tempo de execução **se mantém persistidos:** não voltam aos valores padrão (600 s e 60 s) após reiniciar.

## ❌ Problemas detectados

* Os dois equipamentos de ar-condicionado utilizam os mesmos códigos para funcionar (necessário criação de proteção para evitar duplo acionamento).
  * Problema resolvido ao retirar excesso de emissores de infravermelho do módulo utilizado.
* Falta de um servidor próprio para o datacenter, por enquanto apenas servidor de testes.
  * Servidor forncecido na Rede IoT do CNAT.
* Necessário instalação de novos pontos de energia para o sistema.
  * Instalação de pontos concluída. 

## 📋 Estado atual do código

O firmware (`main.cpp`, `telemetry.cpp` e módulos auxiliares) já implementa:

* leitura de **temperatura** e **umidade** com sensor **DHT21**;
* leitura de **presença/movimento** com sensor **PIR**, com calibração inicial de 80 s;
* cálculo de **ponto de orvalho**;
* envio de dados em **JSON** via **MQTT sobre TLS**, com *retain* e **LWT**;
* **autodescoberta** das entidades no Home Assistant;
* **configuração remota** de timeout e heartbeat (MQTT e web);
* **dashboard web** e API JSON local;
* **OTA** via web, via URL (MQTT) e pelo modo de recuperação;
* **captive portal** com troca de credenciais Wi-Fi e persistencia de configurações de timeout e heartbeat gravadas em LittleFS ;
* reconexão automática de **Wi-Fi** e **MQTT**, com reinício de segurança e *watchdog*;
* sincronização de horário via **NTP**;
* console remoto via **Telnet** na porta **23**;
* contador de boots em memória RTC e diagnósticos do sistema;
* lógica não bloqueante com `millis()`;
* LED verde para presença/calibração e LED vermelho para alerta térmico.

## 📌 Próximos passos sugeridos

* validar assinatura/hash do firmware no OTA remoto;
* documentar as automações do Home Assistant (presença → luzes e controle do ar-condicionado), por exemplo exportando o YAML para o repositório.