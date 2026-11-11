# Lâmpada Inteligente - ESP32 + FastAPI + SQLite

Projeto de monitoramento e controle para **Lâmpada Inteligente** com ESP32-S3, sensores de temperatura (DHT22), luminosidade (LDR), potenciômetro de espectro de cor, botão de mute/controle, sincronização de relógio via **NTP (Timestamp Unix)** e persistência em banco de dados **SQLite** via **FastAPI**.

---

## 📁 Estrutura da Branch `lampada-inteligente`

```text
-iot-esp32-robo-espacial/
├── LampadaInteligente.ino   # Firmware Arduino para ESP32-S3 com NTP e HTTP POST
├── main.py                  # API REST em FastAPI integrada com SQLite (lampada.db)
├── requirements.txt         # Dependências Python
└── .gitignore               # Arquivos ignorados (.venv, lampada.db, etc.)
```

---

## ⚙️ Configuração do ESP32

No arquivo [`LampadaInteligente.ino`](./LampadaInteligente.ino), configure as credenciais da rede e a URL da sua API:

```cpp
const char* WIFI_SSID     = "motorola edge 50 pro_6985";
const char* WIFI_PASSWORD = "sua_senha";

// IP do computador na mesma rede (ex: 10.134.42.63)
const char* API_URL = "http://10.134.42.63:8000/telemetria";
```

---

## 🚀 Como Executar a API

### 1. Iniciar o servidor FastAPI

No terminal dentro da pasta:

```powershell
uvicorn main:app --host 0.0.0.0 --port 8000 --reload
```

---

## 📡 Endpoints da API

- **Swagger UI Interativo:** `http://localhost:8000/docs`
- **`POST /telemetria`**: Recebe a telemetria do ESP32 com o campo `timestamp` (Unix epoch) e salva no SQLite.
- **`GET /telemetria?limite=50`**: Retorna os registros em ordem decrescente.
- **`GET /telemetria/ultima`**: Retorna a última leitura registrada.

---

## 📊 Formato do JSON Enviado pelo ESP32

```json
{
  "timestamp": 1731330360,
  "temperatura": 24.1,
  "luminosidade": 1620,
  "potenciometro": 1500,
  "led_ligado": true,
  "perigo": false,
  "sistema_ativo": true
}
```
