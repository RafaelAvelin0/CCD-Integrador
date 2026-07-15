#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h> // Necessário para a macro PROGMEM

// ==========================================
// CÓDIGOS HTML DAS PÁGINAS WEB
// ==========================================

const char html_dashboard[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang='pt-br'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <meta http-equiv='refresh' content='10'>
    <title>Dashboard - Corisco Centro de Dados</title>
    <style>
        body{font-family:'Segoe UI',sans-serif;text-align:center;background:#f0f4f8;margin:0;padding:15px;color:#333;}
        .card{background:white;max-width:550px;margin:10px auto;padding:25px;border-radius:20px;box-shadow:0 15px 35px rgba(0,0,0,0.08);}
        .status-row{display:flex;justify-content:center;gap:10px;margin-bottom:20px;}
        .badge{padding:6px 15px;border-radius:50px;color:white;font-size:0.7rem;font-weight:bold;text-transform:uppercase;}
        .grid{display:grid;grid-template-columns:1fr 1fr;gap:15px;margin:20px 0;}
        .box{background:#fff;padding:15px;border-radius:15px;border:1px solid #edf2f7;box-shadow:0 4px 6px rgba(0,0,0,0.02);text-align:left;position:relative;overflow:hidden;}
        .box::before{content:'';position:absolute;left:0;top:0;height:100%;width:5px;background:#3498db;}
        .val{font-size:1.6rem;font-weight:900;color:#2d3748;display:block;margin:5px 0;}
        .info-footer{background:#f8fafc;padding:15px;border-radius:12px;text-align:center;font-size:0.8rem;color:#64748b;margin-top:20px;border:1px solid #e2e8f0;}
        .btn{display:block; width:100%; box-sizing:border-box; padding:12px; margin:10px 0; border-radius:8px; border:none; color:white; font-weight:bold; font-size:0.9rem; cursor:pointer; text-decoration:none; text-align:center; font-family:inherit;}
        .btn:active{transform:scale(0.98);}
        .blue{background:#3498db;} .grey{background:#94a3b8;} .green{background:#2ecc71;} .red{background:#e74c3c;} .orange{background:#f59e0b;}
    </style>
</head>
<body>
    <div class='card'>
        <h2 style='margin:0 0 5px 0;'>Corisco Centro de Dados</h2>
        <p style='margin:0 0 20px 0;color:#94a3b8;font-size:0.9rem;'>ESP-12F Monitor Ambiental</p>
        
        <div class='status-row'>
            <span class='badge green'>WiFi: {{WIFI_RSSI}} dBm</span>
            <span class='badge {{MQTT_COLOR}}'>MQTT: {{MQTT_STATUS}}</span>
        </div>
        
        <div class='grid'>
            <div class='box' style='grid-column: span 2;'>
                <small style='color:#a0aec0;font-weight:bold;'>ESTADO DO AMBIENTE (PIR)</small>
                <span class='val' style='color: {{PIR_COLOR}};'>{{PRESENCA}}</span>
                <small style='color:#cbd5e0;'>Timeout configurado: {{TIMEOUT}} seg</small>
            </div>
            <div class='box'>
                <small style='color:#a0aec0;font-weight:bold;'>TEMPERATURA</small>
                <span class='val'>{{TEMP}} &deg;C</span>
            </div>
            <div class='box'>
                <small style='color:#a0aec0;font-weight:bold;'>UMIDADE</small>
                <span class='val'>{{UMID}} %</span>
            </div>
        </div>
        
        <div style='background:#fcf8e3; padding:12px; border-radius:10px; text-align:left; font-size:0.8rem; border:1px solid #faebcc; margin-top:15px; color:#000000;'>
            <strong>IP Local:</strong> {{IP}}<br>
            <strong>SSID:</strong> {{SSID}}<br>
            <strong>MAC:</strong> {{MAC}}<br>
            <strong>Contador de Boots:</strong> {{BOOTS}}<br>
            <hr style='border:0; border-top:1px solid #faebcc; margin:8px 0;'>
            <strong>Sensor:</strong> DHT21 + PIR<br>
            <strong>MCU:</strong> ESP8266 (ESP-12F)<br>
            <strong>Projeto:</strong> Integrador Redes 2026<br>
        </div>
        
        <div style='margin-top:15px;'>
            <a href='/conf' class='btn blue'>CONFIGURAÇÕES (Autenticado)</a>
            <a href='/update' class='btn grey'>ATUALIZAR FIRMWARE (OTA)</a>
        </div>
        
        <div class='info-footer'>
            Uptime: {{UPTIME}} | Atualizado via Web
        </div>
    </div>
</body>
</html>
)rawliteral";

const char html_config[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang='pt-br'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <title>Configurações - Corisco</title>
    <style>
        body{font-family:'Segoe UI',sans-serif;text-align:center;background:#f0f4f8;margin:0;padding:15px;color:#333;}
        .card{background:white;max-width:550px;margin:10px auto;padding:25px;border-radius:20px;box-shadow:0 15px 35px rgba(0,0,0,0.08);}
        .box{background:#fff;padding:20px;border-radius:15px;border:1px solid #edf2f7;text-align:left;position:relative; margin-top: 20px;}
        .box::before{content:'';position:absolute;left:0;top:0;height:100%;width:5px;background:#3498db; border-radius: 15px 0 0 15px;}
        .btn{display:block; width:100%; box-sizing:border-box; padding:12px; margin:20px 0 0 0; border-radius:8px; border:none; color:white; font-weight:bold; font-size:1rem; cursor:pointer; background:#3498db;}
        .btn:active{transform:scale(0.98);}
        input[type='number']{width:100%; padding:12px; margin-top:8px; border:1px solid #cbd5e0; border-radius:8px; box-sizing:border-box; font-size:1.1rem; color:#2d3748;}
        input[type='number']:focus{outline: none; border-color: #3498db; box-shadow: 0 0 0 3px rgba(52,152,219,0.2);}
    </style>
</head>
<body>
    <div class='card'>
        <h2 style='margin:0 0 5px 0;'>Configurações do Sistema</h2>
        <p style='margin:0;color:#94a3b8;font-size:0.9rem;'>Ajuste os parâmetros locais</p>
        
        <form action='/save_conf' method='POST'>
            <div class='box'>
                <label style='font-weight:bold; color:#2d3748;'>Timeout da Presença (segundos)</label>
                <input type='number' name='timeout' min='10' max='3600' value='{{TIMEOUT}}' required>
                <small style='color:#64748b; display:block; margin-top:8px;'>Define o tempo que o sistema considerará o ambiente ocupado após o último movimento detetado.</small>
            </div>
            <button type='submit' class='btn'>GUARDAR CONFIGURAÇÕES</button>
        </form>
        
        <a href='/' style='display:inline-block; margin-top:20px; color:#3498db; text-decoration:none; font-weight:bold;'>Voltar ao Dashboard</a>
    </div>
</body>
</html>
)rawliteral";

const char html_update[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang='pt-br'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <title>Atualização OTA - Corisco</title>
    <style>
        body{font-family:'Segoe UI',sans-serif;text-align:center;background:#f0f4f8;margin:0;padding:15px;color:#333;}
        .card{background:white;max-width:550px;margin:10px auto;padding:25px;border-radius:20px;box-shadow:0 15px 35px rgba(0,0,0,0.08);}
        .box{background:#fff;padding:20px;border-radius:15px;border:1px solid #edf2f7;text-align:left;position:relative; margin-top: 20px;}
        .box::before{content:'';position:absolute;left:0;top:0;height:100%;width:5px;background:#e74c3c; border-radius: 15px 0 0 15px;}
        .btn{display:block; width:100%; box-sizing:border-box; padding:12px; margin:20px 0 0 0; border-radius:8px; border:none; color:white; font-weight:bold; font-size:1rem; cursor:pointer; background:#94a3b8; transition: background 0.3s;}
        .btn.active{background:#e74c3c;}
        .btn:active{transform:scale(0.98);}
        input[type='file']{width:100%; padding:10px; margin-top:8px; box-sizing:border-box; color:#2d3748;}
    </style>
    <script>
        function checkFile() {
            var fileInput = document.getElementById('fw_file');
            var btn = document.getElementById('btn-up');
            if(fileInput.files.length > 0) {
                btn.classList.add('active');
                btn.disabled = false;
            }
        }
        function submitForm() {
            var btn = document.getElementById('btn-up');
            btn.innerHTML = "A ENVIAR... POR FAVOR AGUARDE";
            btn.style.background = "#7f8c8d";
            btn.style.pointerEvents = "none";
        }
    </script>
</head>
<body>
    <div class='card'>
        <h2 style='margin:0 0 5px 0;'>Atualização de Firmware</h2>
        <p style='margin:0;color:#94a3b8;font-size:0.9rem;'>Selecione apenas ficheiros do tipo .bin</p>
        
        <form method='POST' action='/update_fw' enctype='multipart/form-data' onsubmit='submitForm()'>
            <div class='box'>
                <label style='font-weight:bold; color:#2d3748;'>Ficheiro do Firmware (.pio/build/...)</label>
                <input type='file' name='update' id='fw_file' accept='.bin' required onchange='checkFile()'>
                <small style='color:#64748b; display:block; margin-top:8px;'>Não feche o navegador nem desligue o equipamento durante a transferência.</small>
            </div>
            <button type='submit' id='btn-up' class='btn' disabled>INICIAR GRAVAÇÃO</button>
        </form>
        
        <a href='/' style='display:inline-block; margin-top:20px; color:#3498db; text-decoration:none; font-weight:bold;'>Voltar ao Dashboard</a>
    </div>
</body>
</html>
)rawliteral";

#endif