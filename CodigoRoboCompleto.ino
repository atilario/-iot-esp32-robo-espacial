// ESP32 - Controle Remoto (Wokwi)
#define PIN_JOYSTICK_X 34
#define PIN_JOYSTICK_Y 35
#define PIN_BTN_DESLIGAR 32
#define PIN_LED_VERDE 2
#define PIN_LED_VERMELHO 4

bool sistemaLigado = true;

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_BTN_DESLIGAR, INPUT_PULLUP);
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);

  // Status inicial
  digitalWrite(PIN_LED_VERDE, HIGH);
  digitalWrite(PIN_LED_VERMELHO, LOW);
  Serial.println("Controle Remoto Iniciado.");
}

void loop() {
  // Leitura do botão de desligar
  if (digitalRead(PIN_BTN_DESLIGAR) == LOW) {
    sistemaLigado = false;
    digitalWrite(PIN_LED_VERDE, LOW);
    digitalWrite(PIN_LED_VERMELHO, HIGH);
    Serial.println("Comando enviado: DESLIGAR");
    delay(500); // Debounce
  }

  if (sistemaLigado) {
    int xVal = analogRead(PIN_JOYSTICK_X);
    int yVal = analogRead(PIN_JOYSTICK_Y);

    // Mapeamento de direções simples
    if (yVal < 1000) {
      Serial.println("Comando: Frente");
    } else if (yVal > 3000) {
      Serial.println("Comando: Trás");
    } else if (xVal < 1000) {
      Serial.println("Comando: Esquerda");
    } else if (xVal > 3000) {
      Serial.println("Comando: Direita");
    } else {
      Serial.println("Comando: Parado");
    }
  }

  delay(1000); // Intervalo de envio/leitura
}


Etapa 02

// ESP32-S3 - Robô Explorador Físico (Laboratório)
#include <WiFi.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <ESP32Servo.h>
#include "DHT.h"

// Configurações da rede Wi-Fi e CallmeBot
const char* ssid = "CIMATEC-VISITANTE";
const char* password = "";
String phoneNumber = "557197341630"; // Ex: 557199999999 (Sem o 9 inicial do DDD)
String apiKey = "8221290";

// --- MAPEAMENTO DE PINOS (ESP32-S3) ---
#define PIN_DHT          15
#define DHTTYPE          DHT22 // Altere para DHT11 se necessário

#define PIN_LDR          4    // Entrada Analógica (ADC1_CH3 no ESP32-S3)

// Sensor Ultrassônico HC-SR04
#define PIN_TRIG         13   // Saída Digital
#define PIN_ECHO         5    // Entrada Digital

#define PIN_JOYSTICK_X   1    // Eixo X do Joystick (ADC1_CH0 no ESP32-S3)
#define PIN_JOYSTICK_Y   2    // Eixo Y do Joystick (ADC1_CH1 no ESP32-S3)
#define PIN_SERVO        18   // Pino PWM do Servo

#define PIN_LED_VERDE    12
#define PIN_LED_VERMELHO 14

DHT dht(PIN_DHT, DHTTYPE);
Servo meuServo;

bool alertaEnviado = false;

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

void setup() {
  Serial.begin(115200);

  // Configuração dos Pinos
  pinMode(PIN_LDR, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  pinMode(PIN_JOYSTICK_X, INPUT);
  pinMode(PIN_JOYSTICK_Y, INPUT);

  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);

  // Resolução e atenuação de leitura analógica do ESP32-S3 (12 bits: 0 a 4095)
  analogReadResolution(12);

  // Configuração segura do Servo no ESP32-S3
  meuServo.setPeriodHertz(50);             // Frequência padrão de Servos (50Hz)
  meuServo.attach(PIN_SERVO, 500, 2400);   // Mapeamento min/max em microssegundos

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
}

void loop() {
  // 1. Controle do Servo via Joystick
  int valorJoyX = analogRead(PIN_JOYSTICK_X);
  int anguloServo = map(valorJoyX, 0, 4095, 0, 180);
  meuServo.write(anguloServo);

  // 2. Leitura dos Sensores
  float temp = dht.readTemperature();
  float umid = dht.readHumidity();
  int valorLDR = analogRead(PIN_LDR);
  float distancia = lerDistancia();

  // Validação do DHT
  if (isnan(temp) || isnan(umid)) {
    Serial.println("Falha na leitura do sensor DHT!");
    delay(2000);
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
  Serial.print("Angulo do Servo (Joystick X): "); Serial.print(anguloServo); Serial.println("°");
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

  // Intervalo de 2 segundos exigido pelo edital
  delay(2000);
}