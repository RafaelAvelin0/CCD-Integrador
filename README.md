# 🌡️ Sistema de Monitoramento Integrado de Climatização Inteligente

![Status](https://img.shields.io/badge/Status-Em%20Desenvolvimento-yellow)
![Plataforma](https://img.shields.io/badge/Plataforma-ESP8266-blue)
![Linguagem](https://img.shields.io/badge/Linguagem-Arduino-00599C)
![Licença](https://img.shields.io/badge/Licença-MIT-green)

**Corisco Centro de Dados**

Este repositório contém o código-fonte e a documentação técnica do sistema IoT de monitoramento ambiental e eficiência energética desenvolvido para o Corisco Centro de Dados.

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

- [x] **Sensoriamento Básico:** Leitura contínua de temperatura,  umidade e ponto de orvalho (AM2301).
- [x] **Automação de Iluminação:** Controle via sensor de presença (PIR) e rotinas de *timeout*.
- [x] **Console e Manutenção:** Espelhamento de logs via Telnet e atualizações OTA (Over-The-Air).
- [x] **Integração Home Assistant:** Estabelecer comunicação (via MQTT) para centralização dos dados e controle do painel principal.
- [x] **Formatação:** Os dados são enviados em JSON para o broker (facilitar integração no home assistant).
- [ ] **Controle Infravermelho (IR):** Mapeamento e emissão de códigos para gerenciamento dos aparelhos Agratto e TCL.
- [ ] **Acionamento de Potência:** Integração de hardware com o interruptor inteligente Wi-Fi 16A para *failsafe* industrial.
- [ ] **Aprimoramento de Segurança:** Implementação de handshakes seguros ou credenciais encriptadas para comunicação de rede.


## 🚀 Recursos e Escopo Técnico
* **Microcontrolador Central:** Utilização do módulo **ESP-12F** (ESP8266) para processamento local e comunicação Wi-Fi.
* **Monitoramento Ambiental de Precisão:** Leitura periódica de temperatura e umidade via sensor AM2301 (DHT21).
* **Iluminação Automatizada (PIR):** Acionamento e *timeout* de iluminação (simulada por LED Verde) baseado em detecção de movimento/presença no datacenter.
* **Alertas Visuais e Failsafe:** Indicação de alerta térmico (>25°C) ou falhas de hardware através de LED Vermelho, integrado a um mecanismo de *failsafe* industrial com mini interruptor Wi-Fi 16A.
* **Integração Home Assistant & IR:** Mapeamento de códigos infravermelhos para controle integrado na central inteligente.
* **Resiliência de Rede:** Auto-reconexão de Wi-Fi independente do *loop* principal e utilização de IP Fixo para garantir estabilidade.
* **Console Remoto (Telnet):** Espelhamento dos *logs* na porta 23 via TCP/IP para fácil depuração e auditoria sem necessidade de acesso físico.
* **Atualização OTA (Over-The-Air):** Permite a gravação de novos códigos e atualizações remotamente.
* **Máquina de Estados:** Calibração não-bloqueante no *boot* para evitar falsos positivos do sensor de movimento.
* **Heartbeat MQTT:** envio periódico a cada **60 segundos**, mesmo sem mudança significativa.
* **Critério de envio:** o sistema publica quando há alerta, variação de temperatura igual ou superior a **0,5 °C** ou quando o heartbeat expira.

## 📌 Pinagem (Hardware)

| **Componente** | **Pino ESP-12F** | **Lógica** |
| :--- | :---: | :--- |
| **Sensor de Presença (PIR)** | GPIO 04 | Ativo Baixo (LOW = detectado) |
| **LED Verde (Iluminação/OK)** | GPIO 16 | Ativo Alto (HIGH = aceso) |
| **LED Vermelho (Alerta/Falha)**| GPIO 02 | Ativo Alto (HIGH = aceso) |
| **Sensor Temp/Umi (AM2301)** | GPIO 14 | Protocolo One-Wire |

> *Nota: Os componentes de controle infravermelho (IR) e relés de 16A ainda não foram implementados e serão adicionados a esta tabela nas próximas *releases*.*

## 📡 Telemetria MQTT

O firmware publica os dados no tópico:

```text
piredes2026/datacenter/telemetria
```

O payload JSON enviado contém, entre outros campos:

* `team`
* `device`
* `ip`
* `ssid`
* `sensor`
* `temperature`
* `humidity`
* `dew_point`
* `presence`
* `timestamp`

## 🔐 Configuração de credenciais

O projeto usa um arquivo externo chamado `secrets.h`, criado a partir do modelo `secrets-modelo.h`.

Antes de compilar:

1. renomeie `secrets-modelo.h` para `secrets.h`;
2. preencha SSID e senha do Wi-Fi;
3. configure o IP estático;
4. informe os dados do broker MQTT.

> Observação: no arquivo modelo, `mqtt_port` deve ser um **número inteiro**, por exemplo `1883`.

## 🛠️ Como Instalar e Compilar

### Pré-requisitos

* [Visual Studio Code](https://code.visualstudio.com/)
* Extensão [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) instalada no VS Code.

### Passo a Passo de Configuração

1. Clonar o repositório:
```bash
git clone https://github.com/RafaelAvelin0/CCD-Integrador.git
```

    
2.  **Configurar as credenciais (Segurança):**
    
    *   Navegue até à pasta ``src/.``
        
    *   Renomeie o arquivo ``secrets.example.h`` para ``secrets.h.``
        
    *   Edite o arquivo ``secrets.h`` preenchendo o SSID e Password do seu Wi-Fi, além da faixa de IP Estático da rede do datacenter.
        
    *   _O arquivo ``secrets.h`` está incluído no ``.gitignore`` e não será partilhado publicamente._
        
3.  **Primeira Gravação (Via Cabo USB-Serial):**
    
    *   Conecte o ESP-12F via cabo ao computador (necessário conversor FTDI caso seja módulo _standalone_).
        
    *   Coloque a placa em **Modo Flash**: Segure o botão ``GPIO 0``, aperte e solte o botão ``RESET``, e em seguida solte o ``GPIO 0``.
        
    *   No PlatformIO (VS Code), clique no ícone **Upload (➔)** na barra inferior.
        
4.  **Gravações Seguintes (OTA - Over-The-Air):**
    
    *   Uma vez gravado o código base e conectado à rede, não precisa mais do cabo.
        
    *   Edite o arquivo ``platformio.ini``, descomente as linhas de configuração ``espota`` e defina o IP estático do equipamento.
        
    *   O _upload_ passará a ser feito pela rede Wi-Fi automaticamente.

## 💻 Acesso ao console remoto

Com o dispositivo conectado à rede, é possível acompanhar os logs via Telnet ou monitor serial do VSCode:

```bash
nc 192.168.1.50 23
```

Substitua o IP pelo endereço configurado em `secrets.h`.

## 🧩 Dependências usadas no código

* `Arduino.h`
* `DHT.h`
* `ESP8266WiFi.h`
* `ArduinoOTA.h`
* `PubSubClient.h`
* `math.h`
* `time.h`

## 📎 Observações técnicas

* O sistema usa `millis()` para evitar bloqueios no `loop()`.
* O horário é obtido via NTP e formatado em ISO 8601.
* O envio MQTT usa buffer ampliado para suportar o JSON.
* O console Telnet aceita apenas uma conexão por vez.

## ❌ Problemas detectados

* Os dois equipamentos de ar-condicionado utilizam os mesmos códigos para funcionar (necessário criação de proteção para evitar duplo acionamento).
* Falta de um servidor próprio para o datacentar, por enquanto apenas servidor de testes.
* Necessário instalação de novos pontos de energia para o sistema.

## 📋 Estado atual do código

O arquivo `main.cpp` já implementa as seguintes funcionalidades:

* leitura de **temperatura** e **umidade** com sensor **DHT21**;
* leitura de **presença/movimento** com sensor **PIR**;
* cálculo de **ponto de orvalho**;
* envio de dados em **JSON** para um tópico MQTT;
* reconexão automática de **Wi-Fi** e **MQTT**;
* sincronização de horário via **NTP**;
* atualização remota via **OTA**;
* console remoto via **Telnet** na porta **23**;
* lógica não bloqueante com `millis()`;
* LED verde para presença/calibração e LED vermelho para alerta térmico.

## 📌 Próximos passos sugeridos

* integrar descoberta automática no Home Assistant;
* adicionar persistência de configuração;
* implementar controle IR;
* adicionar relé ou outro acionamento físico;
* melhorar a camada de segurança da comunicação MQTT.