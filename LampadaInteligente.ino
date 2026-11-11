#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h> // Biblioteca nativa para controle de horário/NTP

// ==========================================
// CONFIGURAÇÃO WI-FI E API FASTAPI
// ==========================================
const char* WIFI_SSID     = "motorola edge 50 pro_6985";
const char* WIFI_PASSWORD = "123321123";

// COLOQUE AQUI O LINK / ENDPOINT DA SUA API FASTAPI:
const char* API_URL = "http://10.134.42.63:8000/telemetria";

// ==========================================
// CONFIGURAÇÃO DO SERVIDOR NTP (TIMESTAMP)
// ==========================================
const char* NTP_SERVER = "pool.ntp.org";
const long  GMT_OFFSET_SEC = 0;      // 0 para obter Timestamp Unix puro (UTC)
const int   DAYLIGHT_OFFSET_SEC = 0; // Horário de verão (0 para desativado)

// ==========================================
// DEFINIÇÃO DOS PINOS (ESP32-S3)
// ==========================================
#define PIN_DHT          4   
#define PIN_LDR          5   
#define PIN_POT          6   
#define PIN_BOTAO        7   
#define PIN_BUZZER       15  

#define PIN_LED_R        16  
#define PIN_LED_G        17  
#define PIN_LED_B        18  

#define DHTTYPE DHT22
DHT dht(PIN_DHT, DHTTYPE);

// ==========================================
// PARÂMETROS
// ==========================================
const int LIMIAR_LUMINOSIDADE = 1800; 
const bool LED_ANODO_COMUM = false; 

volatile bool sistemaAtivo = true; 
volatile unsigned long ultimoDebounce = 0;
const unsigned long TEMPO_DEBOUNCE = 250; 

int corR = 0, corG = 0, corB = 0;
bool estadoLedRGB = false;

// Intervalo de envio para API (5 segundos)
unsigned long tempoUltimoEnvio = 0;
const unsigned long INTERVALO_ENVIO_API = 5000; 

void IRAM_ATTR isrBotao() {
  unsigned long agora = millis();
  if (agora - ultimoDebounce > TEMPO_DEBOUNCE) {
    sistemaAtivo = !sistemaAtivo;
    ultimoDebounce = agora;
  }
}

void calcularCorEspectro(int valorPot, int &r, int &g, int &b) {
  if (valorPot <= 1023) {
    r = 255;
    g = map(valorPot, 0, 1023, 0, 128);
    b = 0;
  } else if (valorPot <= 2047) {
    r = 255;
    g = map(valorPot, 1024, 2047, 128, 255);
    b = map(valorPot, 1024, 2047, 0, 255);
  } else if (valorPot <= 3071) {
    r = map(valorPot, 2048, 3071, 255, 0);
    g = 255;
    b = 255;
  } else {
    r = 0;
    g = map(valorPot, 3072, 4095, 255, 0);
    b = 255;
  }
}

void aplicarCorLed(int r, int g, int b) {
  if (LED_ANODO_COMUM) {
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
  }
  analogWrite(PIN_LED_R, r);
  analogWrite(PIN_LED_G, g);
  analogWrite(PIN_LED_B, b);
}

// Função para obter o timestamp Unix atual
unsigned long obterTimestampAtual() {
  time_t agora;
  time(&agora);
  return (unsigned long)agora;
}

void enviarParaFastAPI(unsigned long ts, float temp, int ldr, int pot, bool ledStatus, bool perigo, bool sistema) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Wi-Fi] Desconectado. Pulando envio...");
    return;
  }

  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  // Montagem do payload JSON contendo o campo "timestamp"
  char jsonBuffer[300];
  snprintf(jsonBuffer, sizeof(jsonBuffer),
    "{\"timestamp\":%lu,\"temperatura\":%.1f,\"luminosidade\":%d,\"potenciometro\":%d,"
    "\"led_ligado\":%s,\"perigo\":%s,\"sistema_ativo\":%s}",
    ts, temp, ldr, pot,
    ledStatus ? "true" : "false",
    perigo ? "true" : "false",
    sistema ? "true" : "false"
  );

  int httpResponseCode = http.POST(jsonBuffer);

  if (httpResponseCode > 0) {
    Serial.printf("[API] Sucesso (%d): %s\n", httpResponseCode, http.getString().c_str());
  } else {
    Serial.printf("[API] Erro ao enviar: %s (Codigo: %d)\n", http.errorToString(httpResponseCode).c_str(), httpResponseCode);
  }

  http.end();
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  aplicarCorLed(0, 0, 0);

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  pinMode(PIN_BOTAO, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_BOTAO), isrBotao, FALLING);

  dht.begin();

  // Conexão Wi-Fi
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado com sucesso!");
    Serial.print("IP do ESP32: ");
    Serial.println(WiFi.localIP());

    // Inicializa a sincronização de data/hora via NTP
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    Serial.println("Sincronizando relogio NTP...");
  } else {
    Serial.println("\nFalha ao conectar no Wi-Fi.");
  }
}

void loop() {
  float temperatura = dht.readTemperature();
  int leituraLDR = analogRead(PIN_LDR);
  int leituraPot = analogRead(PIN_POT);

  if (isnan(temperatura)) {
    temperatura = 20.0;
  }

  bool ambienteEscuro = (leituraLDR < LIMIAR_LUMINOSIDADE);
  bool temperaturaSegura = (temperatura >= 0.0 && temperatura <= 25.0);
  bool perigoTemperatura = (temperatura < 0.0 || temperatura > 25.0);

  calcularCorEspectro(leituraPot, corR, corG, corB);

  // Lógica do LED RGB
  if (sistemaAtivo && ambienteEscuro && temperaturaSegura) {
    aplicarCorLed(corR, corG, corB);
    estadoLedRGB = true;
  } else {
    aplicarCorLed(0, 0, 0);
    estadoLedRGB = false;
  }

  // Lógica do Buzzer
  if (sistemaAtivo && perigoTemperatura) {
    digitalWrite(PIN_BUZZER, HIGH);
  } else {
    digitalWrite(PIN_BUZZER, LOW);
  }

  // Disparo periódico do envio HTTP e Log Serial
  if (millis() - tempoUltimoEnvio >= INTERVALO_ENVIO_API) {
    tempoUltimoEnvio = millis();
    unsigned long timestampAtual = obterTimestampAtual();

    Serial.println("=========================================");
    Serial.printf("Timestamp (Unix): %lu\n", timestampAtual);
    Serial.printf("Status Sistema:   %s\n", sistemaAtivo ? "ATIVO" : "DESATIVADO (MUTE)");
    Serial.printf("Temperatura:      %.1f ºC\n", temperatura);
    if (perigoTemperatura) Serial.println(">>> Perigo! Desligar! <<<");
    Serial.printf("Estado LED RGB:   %s | LDR: %d | Pot: %d\n", estadoLedRGB ? "ON" : "OFF", leituraLDR, leituraPot);

    // Envio para FastAPI com timestamp
    enviarParaFastAPI(timestampAtual, temperatura, leituraLDR, leituraPot, estadoLedRGB, perigoTemperatura, sistemaAtivo);
  }
}
