# Monitoramento IoT - ESP32 + Matriz 8x8 + FastAPI + SQLite

Projeto de recepção de telemetria via rádio **ESP-NOW**, exibição cíclica em **Matriz de LED 8x8 (MAX7219)** e persistência em banco de dados **SQLite** através de uma API REST construída com **FastAPI**.

---

## 📁 Estrutura dos Arquivos

```text
-iot-esp32-robo-espacial/
├── ReceptorMatriz8x8.ino   # Firmware ESP32 (ESP-NOW + Matriz MAX7219 + Saída Serial JSON)
├── main.py                # API REST em FastAPI integrada com SQLite (sensores.db)
├── serial_gateway.py      # Gateway Python para leitura da porta COM e POST na API
├── requirements.txt       # Dependências Python (fastapi, uvicorn, pyserial, etc.)
└── .gitignore             # Arquivos ignorados pelo Git (.venv, sensores.db, etc.)
```

---

## 🚀 Como Executar o Projeto

### 1. Preparar o Ambiente Python

Certifique-se de que o ambiente virtual está ativado ou instale as dependências:

```powershell
pip install -r requirements.txt
```

---

### 2. Iniciar a API REST (FastAPI + SQLite)

Execute o servidor Uvicorn dentro da pasta do projeto:

```powershell
uvicorn main:app --host 0.0.0.0 --port 8000 --reload
```

- **Documentação Swagger interativa:** [http://localhost:8000/docs](http://localhost:8000/docs)
- **Endpoint de inserção:** `POST /api/leituras`
- **Endpoint de listagem:** `GET /api/leituras?limite=20`
- **Última leitura:** `GET /api/leituras/ultima`

---

### 3. Conectar o ESP32 ao Sistema

Você tem duas formas de operação:

#### Opção A: Gateway Serial (Recomendada para laboratório)
1. Conecte o ESP32 receptor via USB ao computador.
2. Grave o sketch [`ReceptorMatriz8x8.ino`](./ReceptorMatriz8x8.ino) no ESP32.
3. Abra outro terminal e inicie o gateway serial:
   ```powershell
   python serial_gateway.py
   ```
   *(Caso sua porta COM seja diferente de `COM3`, você pode passar como argumento: `python serial_gateway.py COM4`)*.

#### Opção B: Envio Direto via Wi-Fi pelo ESP32
Caso configure o ESP32 para enviar direto via Wi-Fi (modo STA + ESP-NOW), aponte a URL do endpoint para:
```text
http://<IP_DO_SEU_PC>:8000/api/leituras
```

---

## 📊 Formato dos Dados (JSON)

```json
{
  "nivel": 85,
  "temperatura": 24.5,
  "umidade": 60.0,
  "luminosidade": 1200,
  "presenca": 1,
  "timestamp": 123456
}
```
