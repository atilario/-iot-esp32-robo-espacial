// ESP32-S3 - Robô Explorador Físico (Laboratório) + WebServer + API Rest (Etapa 03)
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <ESP32Servo.h>
#include "DHT.h"
#include <vector>
#include "time.h"

// ============================================================================
// 1. CONFIGURAÇÕES DE REDE, CALLMEBOT E API (ESCOPO GLOBAL)
// ============================================================================
const char* ssid = "iPhone de Titila";
const char* password = "tainalinda";

// >>> COLOQUE AQUI A URL DO SEU LOCALTUNNEL (Use http:// sem o 's') <<<
const char* apiEndpoint = "http://172.20.10.5:8000/leituras";

String phoneNumber = "557197341630"; 
String apiKey = "8221290";

// Buffer em memória para guardar leituras se a API/Internet falhar (Passo 8)
std::vector<String> bufferLeituras;

// Configuração de Servidor NTP para gerar o Timestamp real
const char* ntpServer = "pool.ntp.org";

// --- MAPEAMENTO DE PINOS (ESP32-S3) ---
#define PIN_DHT          15
#define DHTTYPE          DHT22

#define PIN_LDR          4    // Entrada Analógica (ADC1_CH3)
#define PIN_TRIG         13   // HC-SR04 Trig
#define PIN_ECHO         5    // HC-SR04 Echo
#define PIN_SERVO        18   // Pino PWM do Servo
#define PIN_LED_VERDE    12
#define PIN_LED_VERMELHO 14

DHT dht(PIN_DHT, DHTTYPE);
Servo meuServo;
WebServer server(80); // Servidor Web na porta 80

bool alertaEnviado = false;
bool roboLigado = true;
int anguloServoAtual = 90; // Posição neutra inicial do Servo
unsigned long ultimoTempoLeitura = 0;

// ============================================================================
// FUNÇÕES AUXILIARES (SENSORES, TIMESTAMP, WHATSAPP E API)
// ============================================================================

// Gera o timestamp no formato exato do edital: "YYYY-MM-DDTHH:MM:SSZ"
String obterTimestampISO() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 100)) {
    return "2026-10-21T12:00:00Z"; // Fallback caso o NTP da rede demore a sincronizar
  }
  char timeStringBuff[25];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(timeStringBuff);
}

// Leitura do Sensor Ultrassônico
float lerDistancia() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duracao = pulseIn(PIN_ECHO, HIGH, 25000); 
  if (duracao == 0) return 400.0;
  return (duracao * 0.0343) / 2.0;
}

// Envio de Alerta via CallmeBot (WhatsApp)
void sendMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(url);
    if (httpResponseCode == 200) {
      Serial.println("Notificação de alerta enviada via WhatsApp!");
    } else {
      Serial.print("Erro ao enviar mensagem HTTP: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

// >>> FUNÇÃO DA ETAPA 03: ENVIO HTTP POST PARA A API PYTHON + BUFFER <<<
void enviarDadosAPI(float temp, float umid, int luz, int presenca, float probVida) {
  // Monta o JSON conforme especificação do Passo 8
  String jsonPayload = "{";
  jsonPayload += "\"timestamp\": \"" + obterTimestampISO() + "\",";
  jsonPayload += "\"temperatura_c\": " + String(temp, 1) + ",";
  jsonPayload += "\"umidade_pct\": " + String(umid, 0) + ",";
  jsonPayload += "\"luminosidade\": " + String(luz) + ",";
  jsonPayload += "\"presenca\": " + String(presenca) + ",";
  jsonPayload += "\"probabilidade_vida\": " + String(probVida, 1);
  jsonPayload += "}";

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(apiEndpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Bypass-Tunnel-Reminder", "true"); // Evita a tela de bloqueio do LocalTunnel!

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode == 200 || httpResponseCode == 201) {
      Serial.printf("[API] Leitura salva no banco! (Status: %d)\n", httpResponseCode);

      // Se houver leituras antigas presas no buffer, aproveita a conexão para reenviar
      if (!bufferLeituras.empty()) {
        Serial.printf("[Buffer] Reenviando leitura pendente (%d restantes)...\n", bufferLeituras.size());
        String payloadPendente = bufferLeituras.front();
        int retryCode = http.POST(payloadPendente);
        if (retryCode == 200 || retryCode == 201) {
          bufferLeituras.erase(bufferLeituras.begin());
          Serial.println("[Buffer] Leitura pendente sincronizada com sucesso!");
        }
      }
    } else {
      Serial.printf("[API] Falha no envio (Erro HTTP: %d). Guardando no buffer...\n", httpResponseCode);
      if (bufferLeituras.size() < 50) { // Limite de 50 leituras para proteger a RAM do ESP32
        bufferLeituras.push_back(jsonPayload);
      }
    }
    http.end();
  } else {
    Serial.println("[WiFi] Sem conexão. Guardando leitura no buffer...");
    if (bufferLeituras.size() < 50) {
      bufferLeituras.push_back(jsonPayload);
    }
  }
}

// Rota HTTP POST para receber comandos do Joystick Remoto/Virtual
void handleControl() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");

    if (cmd == "DESLIGAR") {
      roboLigado = false;
      Serial.println("\n[COMANDO REMOTO RECEIVED]: DESLIGAR");
    } else if (cmd == "LIGAR") {
      roboLigado = true;
      Serial.println("\n[COMANDO REMOTO RECEIVED]: LIGAR");
    } else if (cmd == "Esquerda") {
      anguloServoAtual = constrain(anguloServoAtual - 15, 0, 180);
      meuServo.write(anguloServoAtual);
      Serial.println("\n[COMANDO REMOTO RECEIVED]: Mover Esquerda");
    } else if (cmd == "Direita") {
      anguloServoAtual = constrain(anguloServoAtual + 15, 0, 180);
      meuServo.write(anguloServoAtual);
      Serial.println("\n[COMANDO REMOTO RECEIVED]: Mover Direita");
    } else if (cmd == "Centro") {
      anguloServoAtual = 90;
      meuServo.write(anguloServoAtual);
      Serial.println("\n[COMANDO REMOTO RECEIVED]: Centro");
    }
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Parametro 'cmd' ausente");
  }
}

// ============================================================================
// 2. SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_LDR, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);

  analogReadResolution(12);

  meuServo.setPeriodHertz(50);
  meuServo.attach(PIN_SERVO, 500, 2400);
  meuServo.write(anguloServoAtual);

  dht.begin();

  // Conexão Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado com Sucesso!");
  Serial.print(">>> IP DO ROBÔ FÍSICO: ");
  Serial.println(WiFi.localIP());

  // Sincroniza relógio via NTP (UTC) para gerar o timestamp da Etapa 03
  configTime(0, 0, ntpServer);

  // Rota do servidor HTTP
  server.on("/cmd", HTTP_POST, handleControl);
  server.begin();
  Serial.println("Servidor HTTP de Controle Remoto Ativo!");
}

// ============================================================================
// 3. LOOP PRINCIPAL
// ============================================================================
void loop() {
  // Trata conexões HTTP do controle virtual continuamente sem travar o código
  server.handleClient();

  // Se o robô foi desligado remotamente
  if (!roboLigado) {
    digitalWrite(PIN_LED_VERDE, LOW);
    digitalWrite(PIN_LED_VERMELHO, HIGH);
    return;
  }

  // Bloco não-bloqueante executado a cada 2 segundos para telemetria, sensores e API
  if (millis() - ultimoTempoLeitura >= 2000) {
    ultimoTempoLeitura = millis();

    float temp = dht.readTemperature();
    float umid = dht.readHumidity();
    int valorLDR = analogRead(PIN_LDR);
    float distancia = lerDistancia();

    if (isnan(temp) || isnan(umid)) {
      Serial.println("Falha na leitura do sensor DHT!");
      return;
    }

    // Converte a distância do ultrassônico em flag de presença (0 ou 1) para o banco (Passo 8)
    int presencaDetectada = (distancia > 0 && distancia <= 30.0) ? 1 : 0;

    // Algoritmo de Probabilidade de Vida
    int probVida = 0;
    if (temp >= 15.0 && temp <= 30.0) probVida += 25;
    if (umid >= 40.0 && umid <= 70.0) probVida += 25;
    if (valorLDR > 1500) probVida += 20;
    if (presencaDetectada == 1) probVida += 30;

    // Telemetria via Monitor Serial
    Serial.println("----------------------------------------");
    Serial.print("Angulo Atual do Servo: "); Serial.print(anguloServoAtual); Serial.println("°");
    Serial.print("Temperatura: "); Serial.print(temp, 1); Serial.println(" °C");
    Serial.print("Umidade: "); Serial.print(umid, 1); Serial.println(" %");
    Serial.print("Intensidade de Luz (LDR): "); Serial.println(valorLDR);
    Serial.print("Distancia Ultrassom: "); Serial.print(distancia, 1); Serial.println(" cm");
    Serial.print("Probabilidade de Vida: "); Serial.print(probVida); Serial.println("%");

    // Tomada de Decisão
    if (probVida <= 75) {
      digitalWrite(PIN_LED_VERDE, HIGH);
      digitalWrite(PIN_LED_VERMELHO, LOW);
      Serial.println("Estado: LIGADO - Exploração normal.");
      alertaEnviado = false;
    } else {
      digitalWrite(PIN_LED_VERDE, LOW);
      digitalWrite(PIN_LED_VERMELHO, HIGH);
      Serial.println("Estado: ALERTA! Alta probabilidade de vida detectada!");

      if (!alertaEnviado) {
        sendMessage("Alerta! Alta probabilidade de vida detectada no planeta.");
        alertaEnviado = true;
      }
    }

    if (presencaDetectada == 1) {
      Serial.println("Sensor de Presenca: Presença detectada");
    } else {
      Serial.println("Sensor de Presenca: Sem presença");
    }

    // >>> CHAMADA DA API (ETAPA 03) <<<
    enviarDadosAPI(temp, umid, valorLDR, presencaDetectada, (float)probVida);
  }
}