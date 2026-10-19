// ESP32-S3 - Robô Explorador Físico (Laboratório)
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <ESP32Servo.h>
#include "DHT.h"

// Configurações da rede Wi-Fi e CallmeBot
const char* ssid = "motorola edge 50 pro_6985";
const char* password = "123321123";
String phoneNumber = "557197341630"; // Ex: 557199999999 (Sem o 9 inicial do DDD)
String apiKey = "8221290";

// --- MAPEAMENTO DE PINOS (ESP32-S3) ---
#define PIN_DHT          15
#define DHTTYPE          DHT22 // Altere para DHT11 se necessário

#define PIN_LDR          4    // Entrada Analógica (ADC1_CH3 no ESP32-S3)

// Sensor Ultrassônico HC-SR04
#define PIN_TRIG         13   // Saída Digital
#define PIN_ECHO         5    // Entrada Digital

#define PIN_SERVO        18   // Pino PWM do Servo

#define PIN_LED_VERDE    12
#define PIN_LED_VERMELHO 14

DHT dht(PIN_DHT, DHTTYPE);
Servo meuServo;
WebServer server(80);

bool alertaEnviado = false;
int anguloServo = 90; // Posição inicial (90° - centro)
unsigned long ultimaLeitura = 0;
const unsigned long intervaloLeitura = 2000; // Intervalo de 2 segundos exigido pelo edital

// Função para ler a distância do Sensor Ultrassônico (em cm)
float lerDistancia() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // pulseIn com timeout estrito de 25.000 microsegundos (~4 metros max)
  long duracao = pulseIn(PIN_ECHO, HIGH, 25000); 
  
  if (duracao == 0) return 400.0; // Fora de alcance ou falha de leitura

  // Velocidade do som (0.0343 cm/us) / 2
  return (duracao * 0.0343) / 2.0;
}

// Envia mensagem para o WhatsApp via HTTPS (API CallMeBot)
void sendMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(url);
    if (httpResponseCode == 200) {
      Serial.println("Notificação enviada via WhatsApp (CallMeBot)!");
    } else {
      Serial.print("Erro ao enviar mensagem HTTP: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

// Página Web de controle do Servo Motor
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Controle do Robô Espacial</title>";
  html += "<style>body{font-family:Arial;text-align:center;margin-top:30px;}";
  html += "button{padding:12px 20px;margin:5px;font-size:16px;cursor:pointer;}</style></head><body>";
  html += "<h2>Controle do Motor - Robô Explorador</h2>";
  html += "<p>Ângulo atual do Servo: <strong>" + String(anguloServo) + "°</strong></p>";
  html += "<div>";
  html += "<a href='/motor?angulo=0'><button>0° (Esquerda)</button></a>";
  html += "<a href='/motor?angulo=45'><button>45°</button></a>";
  html += "<a href='/motor?angulo=90'><button>90° (Centro)</button></a>";
  html += "<a href='/motor?angulo=135'><button>135°</button></a>";
  html += "<a href='/motor?angulo=180'><button>180° (Direita)</button></a>";
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

// Endpoint para receber comandos de controle do motor e notificar via CallMeBot (HTTPS)
void handleMotor() {
  if (server.hasArg("angulo")) {
    int novoAngulo = server.arg("angulo").toInt();
    anguloServo = constrain(novoAngulo, 0, 180);
    meuServo.write(anguloServo);

    String msg = "Comando recebido: Servo motor movido para " + String(anguloServo) + " graus.";
    Serial.println(msg);
    sendMessage(msg);

    server.sendHeader("Location", "/");
    server.send(303);
  } else if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    cmd.toLowerCase();

    if (cmd == "esquerda") anguloServo = 0;
    else if (cmd == "centro" || cmd == "frente" || cmd == "parar") anguloServo = 90;
    else if (cmd == "direita") anguloServo = 180;
    else {
      server.send(400, "text/plain", "Comando invalido. Use: esquerda, centro, direita ou ?angulo=0..180");
      return;
    }

    meuServo.write(anguloServo);
    String msg = "Comando [" + cmd + "] executado: Servo em " + String(anguloServo) + " graus.";
    Serial.println(msg);
    sendMessage(msg);

    server.send(200, "text/plain", msg);
  } else {
    server.send(400, "text/plain", "Informe o parametro 'angulo' (0 a 180) ou 'cmd' (esquerda, centro, direita).");
  }
}

void setup() {
  Serial.begin(115200);

  // Configuração dos Pinos
  pinMode(PIN_LDR, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);

  // Resolução e atenuação de leitura analógica do ESP32-S3 (12 bits: 0 a 4095)
  analogReadResolution(12);

  // Configuração segura do Servo no ESP32-S3
  meuServo.setPeriodHertz(50);             // Frequência padrão de Servos (50Hz)
  meuServo.attach(PIN_SERVO, 500, 2400);   // Mapeamento min/max em microssegundos
  meuServo.write(anguloServo);             // Inicializa na posição central (90°)

  dht.begin();

  // Conexão Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado com Sucesso!");
  Serial.print("IP do ESP32-S3: ");
  Serial.println(WiFi.localIP());

  // Inicialização do Servidor HTTP para receber comandos de controle do motor
  server.on("/", handleRoot);
  server.on("/motor", handleMotor);
  server.begin();
  Serial.print("Servidor de controle ativo! Acesse: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  // 1. Escuta comandos recebidos via rede para movimentar o Servo Motor
  server.handleClient();

  // Executa a leitura de sensores a cada 2 segundos sem travar o recebimento de comandos
  unsigned long agora = millis();
  if (agora - ultimaLeitura < intervaloLeitura) {
    return;
  }
  ultimaLeitura = agora;

  // 2. Leitura dos Sensores
  float temp = dht.readTemperature();
  float umid = dht.readHumidity();
  int valorLDR = analogRead(PIN_LDR);
  float distancia = lerDistancia();

  // Validação do DHT
  if (isnan(temp) || isnan(umid)) {
    Serial.println("Falha na leitura do sensor DHT!");
    return;
  }

  // 3. Algoritmo de Probabilidade de Vida
  int probVida = 0;
  if (temp >= 15.0 && temp <= 30.0) probVida += 25;
  if (umid >= 40.0 && umid <= 70.0) probVida += 25;
  if (valorLDR > 1500) probVida += 20;
  if (distancia > 0 && distancia <= 30.0) probVida += 30; // Objeto/presença a menos de 30 cm

  // 4. Telemetria via Monitor Serial
  Serial.println("----------------------------------------");
  Serial.print("Angulo do Servo (Remoto): "); Serial.print(anguloServo); Serial.println("°");
  Serial.print("Temperatura: "); Serial.print(temp, 1); Serial.println(" °C");
  Serial.print("Umidade: "); Serial.print(umid, 1); Serial.println(" %");
  Serial.print("Intensidade de Luz (LDR): "); Serial.println(valorLDR);
  Serial.print("Distancia Ultrassom: "); Serial.print(distancia, 1); Serial.println(" cm");
  Serial.print("Probabilidade de Vida: "); Serial.print(probVida); Serial.println("%");

  // 5. Tomada de Decisão
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

  // Status do Sensor de Presença no Monitor Serial
  if (distancia > 0 && distancia <= 30.0) {
    Serial.println("Sensor de Presenca: Presença detectada");
  } else {
    Serial.println("Sensor de Presenca: Sem presença");
  }
}