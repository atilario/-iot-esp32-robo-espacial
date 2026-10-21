# 🚀 Robô Espacial Explorador - Práticas Integradas (IoT)

Repositório do projeto **iot-esp32-robo-espacial** desenvolvido para a prática da **Camada de Percepção e Camada de Serviço** (SENAI CIMATEC).

O projeto integra sensores embarcados em uma **ESP32-S3**, controle de atuador (servo motor), tomada de decisão local com algoritmo de detecção de probabilidade de vida, notificações automáticas e uma **API REST Backend em Python (FastAPI + SQLite)** para persistência e consulta das leituras de telemetria.

---

## 🛠️ 1. Como Montar o Robô

### Componentes Utilizados
- **1x** Placa ESP32-S3 (ou ESP32 convencional)
- **1x** Sensor de Temperatura e Umidade (DHT22 ou DHT11)
- **1x** Sensor de Luminosidade (LDR) + Resistor de 10kΩ (divisor de tensão)
- **1x** Sensor Ultrassônico HC-SR04 (detecção de presença/distância)
- **1x** Micro Servo Motor SG90 (atuador de direção/varredura)
- **1x** LED Verde (Status: Operação normal / Conectado)
- **1x** LED Vermelho (Status: Alerta / Desconectado)
- **2x** Resistores de 220Ω (para os LEDs)
- **1x** Protoboard e Jumpers

---

### Mapeamento de Pinos (Pinout)

| Componente | Pino do Componente | Pino na ESP32-S3 | Observação |
| :--- | :--- | :--- | :--- |
| **DHT22 / DHT11** | Dados (DATA) | **GPIO 15** | Alimentação em 3.3V / 5V e GND |
| **LDR (Luminosidade)**| Sinal Analógico | **GPIO 4** (ADC1_CH3) | Divisor de tensão com resistor de 10kΩ ligado ao GND |
| **HC-SR04** | Trigger (TRIG) | **GPIO 13** | Saída digital da ESP32 |
| **HC-SR04** | Echo (ECHO) | **GPIO 5** | Entrada digital da ESP32 |
| **Servo Motor** | Sinal PWM (Laranja/Amarelo) | **GPIO 18** | Alimentação no 5V (VCC) e GND |
| **LED Verde** | Ânodo (+) | **GPIO 12** | Resistor de 220Ω em série |
| **LED Vermelho** | Ânodo (+) | **GPIO 14** | Resistor de 220Ω em série |
| **GND / VCC** | Terra / Alimentação | **GND / 3.3V - 5V** | Barramento comum da protoboard |

> **Nota para o Controle Remoto (Etapa 1 - Joystick):**  
> Caso utilize a ESP32 como controle remoto dedicado com joystick:
> - **Joystick Eixo X:** GPIO 4
> - **Joystick Eixo Y:** GPIO 5
> - **Botão de Parada/Emergência:** GPIO 6 (pull-up interno ativado)
> - **LEDs de Status do Controle:** Verde no GPIO 15 e Vermelho no GPIO 16.

---

## 💻 2. Como Rodar o Backend Python (FastAPI)

A camada de serviço consiste em uma API assíncrona desenvolvida com **FastAPI**, com validação de dados via **Pydantic** e persistência local em banco de dados **SQLite** (`leituras.db`).

### Pré-requisitos
- Python 3.10+ instalado no computador.

### Passo a Passo

1. **Abra o terminal** na pasta do projeto:
   ```powershell
   cd "c:\Users\ATILA\OneDrive\Área de Trabalho\praticas iot"
   ```

2. **Crie e ative o ambiente virtual (venv):**
   ```powershell
   # Criação do ambiente virtual
   py -m venv .venv

   # Ativação no Windows (PowerShell)
   .\.venv\Scripts\Activate.ps1

   # (Se usar o Prompt de Comando CMD tradicional: .venv\Scripts\activate.bat)
   ```

3. **Instale as dependências:**
   ```powershell
   pip install fastapi uvicorn pydantic
   ```

4. **Inicie o servidor da API:**
   ```powershell
   uvicorn main:app --host 0.0.0.0 --port 8000 --reload
   ```
   - O parâmetro `--host 0.0.0.0` permite que dispositivos externos na mesma rede local (como a ESP32) se conectem à API.
   - O parâmetro `--reload` reinicia o servidor automaticamente a cada alteração no código.

5. *(Opcional)* **Expor para a Internet via LocalTunnel:**  
   Caso a ESP32 esteja em outra rede ou em simulador na nuvem (ex: Velxio / Wokwi), abra outro terminal e execute:
   ```powershell
   npx localtunnel --port 8000 --subdomain robo-espacial-cimatec
   ```
   A API ficará acessível globalmente em: `https://robo-espacial-cimatec.loca.lt`

---

## 📊 3. Como Consultar os Dados Salvos

Existem três maneiras práticas de visualizar e consultar os dados coletados pelos sensores e salvos no banco:

### Método A: Documentação Interativa Swagger UI (Recomendado)
1. Com a API rodando, abra o navegador e acesse:
   ```text
   http://127.0.0.1:8000/docs
   ```
2. Localize a rota **`GET /leituras`**.
3. Clique em **Try it out** e depois em **Execute**.
4. Você verá a lista das últimas 100 leituras registradas com timestamp, temperatura, umidade, luminosidade, status de presença e probabilidade de vida.

---

### Método B: Endpoint HTTP Direto (Navegador, cURL ou Postman)
Faça uma requisição `GET` para a rota `/leituras`:

- **Pelo Navegador:** Acesse diretamente [http://localhost:8000/leituras](http://localhost:8000/leituras)
- **Via cURL (Terminal):**
  ```bash
  curl -X GET "http://127.0.0.1:8000/leituras" -H "accept: application/json"
  ```
- **Formato da Resposta (JSON):**
  ```json
  [
    {
      "id": 1,
      "timestamp": "2026-10-21T09:45:12.345678",
      "temperatura_c": 24.8,
      "umidade_pct": 58.2,
      "luminosidade": 1820.0,
      "presenca": 1,
      "probabilidade_vida": 85.0
    }
  ]
  ```

---

### Método C: Consulta Direta no Banco SQLite (`leituras.db`)
Como a API utiliza o SQLite, o arquivo `leituras.db` é gerado automaticamente na raiz do projeto.

1. **Via Linha de Comando (PowerShell/Bash):**
   ```bash
   sqlite3 leituras.db "SELECT id, timestamp, temperatura_c, umidade_pct, presenca, probabilidade_vida FROM leituras ORDER BY id DESC LIMIT 10;"
   ```
2. **Via VS Code / IDE:**
   - Instale a extensão **SQLite Viewer** ou **Database Client**.
   - Clique com o botão direito no arquivo `leituras.db` e selecione *Open with SQLite Viewer* para visualizar os dados em formato de tabela interativa.

---

## 🔗 Links Úteis e Simulação
- **Simulação no Velxio (Etapa 1 - Controle Remoto):** [Acessar Projeto Velxio](https://velxio.dev/project/30a22f6f-aa6a-4abe-a6c6-5e90e69ef379)
