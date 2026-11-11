# Lâmpada Inteligente - ESP32 + FastAPI + SQLite

Projeto de monitoramento e controle para **Lâmpada Inteligente** com ESP32-S3, sensores de temperatura (DHT22), luminosidade (LDR), potenciômetro de espectro de cor, botão de mute/controle e persistência em banco de dados **SQLite** via **FastAPI**.

---

## 📁 Estrutura da Branch `lampada-inteligente`

```text
-iot-esp32-robo-espacial/
├── LampadaInteligente.ino   # Firmware Arduino para ESP32-S3 com envio HTTP POST
├── main.py                  # API REST em FastAPI integrada com SQLite (lampada.db)
├── requirements.txt         # Dependências Python
└── .gitignore               # Arquivos ignorados (.venv, lampada.db, etc.)
```

---

## ⚙️ Configuração do ESP32

No arquivo [`LampadaInteligente.ino`](./LampadaInteligente.ino), configure as credenciais do seu Wi-Fi e a URL da sua máquina/servidor onde a API está rodando:

```cpp
const char* WIFI_SSID     = "SEU_WIFI";
const char* WIFI_PASSWORD = "SUA_SENHA";

// Exemplo local: use o IP da sua máquina na mesma rede Wi-Fi (ex: via ipconfig)
const char* API_URL = "http://192.168.1.105:8000/telemetria";
```

---

## 🚀 Como Executar a API

### 1. Instalar as dependências

```powershell
pip install -r requirements.txt
```

### 2. Iniciar o servidor FastAPI

```powershell
uvicorn main:app --host 0.0.0.0 --port 8000 --reload
```

> **Atenção:** Usar `--host 0.0.0.0` é fundamental para permitir que o ESP32 conectado na mesma rede Wi-Fi consiga acessar a API usando o IP do seu computador!

---

## 📡 Endpoints da API

- **Swagger UI Interativo:** `http://localhost:8000/docs`
- **`POST /telemetria`**: Recebe a telemetria do ESP32 a cada 5 segundos e persiste no SQLite.
- **`GET /telemetria?limite=50`**: Retorna as últimas leituras em ordem decrescente.
- **`GET /telemetria/ultima`**: Retorna apenas a última leitura registrada (ideal para dashboards e cards em tempo real).

---

## 📊 Formato do JSON Enviado pelo ESP32

```json
{
  "temperatura": 23.4,
  "luminosidade": 1450,
  "potenciometro": 2048,
  "led_ligado": true,
  "perigo": false,
  "sistema_ativo": true
}
```
