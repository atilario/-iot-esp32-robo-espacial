#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// ==========================================
// DEFINIÇÃO DA MATRIZ DE LED 8x8
// ==========================================
// Caso o texto fique invertido ou de lado, troque FC16_HW por GENERIC_HW
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   1   // 1 módulo 8x8

#define DATA_PIN  23  // DIN (IN)
#define CLK_PIN   18  // CLK (IN)
#define CS_PIN    5   // CS  (IN)

MD_Parola matriz = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Configurações de velocidade e pausa do scroll (ms)
const uint16_t VELOCIDADE_SCROLL = 50;
const uint16_t PAUSA_ENTRE_TELAS = 800;

// ==========================================
// ESTRUTURA DOS DADOS RECEBIDOS
// ==========================================
typedef struct {
  int nivel;
  int temperatura;
  int umidade;
  int luminosidade;
  unsigned long timestamp;
} PacoteDados;

PacoteDados pacoteRecebido;

volatile bool novoPacoteRecebido = false;
unsigned long lastRxMillis = 0;
const unsigned long TIMEOUT_MS = 5000;

bool presenca = true;

// ==========================================
// CONTROLE DO CICLO DAS TELAS
// ==========================================
int telaAtual = 1;
char bufferTexto[25] = "INIC";

void carregarTextoTela() {
  switch (telaAtual) {
    case 1:
      snprintf(bufferTexto, sizeof(bufferTexto), "NVL %d%%", pacoteRecebido.nivel);
      break;
    case 2:
      snprintf(bufferTexto, sizeof(bufferTexto), "TMP %dC", pacoteRecebido.temperatura);
      break;
    case 3:
      snprintf(bufferTexto, sizeof(bufferTexto), "UMD %d%%", pacoteRecebido.umidade);
      break;
    case 4:
      snprintf(bufferTexto, sizeof(bufferTexto), "LUX %d", pacoteRecebido.luminosidade);
      break;
    case 5:
      snprintf(bufferTexto, sizeof(bufferTexto), "PRS %s", presenca ? "ON" : "OFF");
      break;
  }
  matriz.displayText(bufferTexto, PA_LEFT, VELOCIDADE_SCROLL, PAUSA_ENTRE_TELAS, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}

// ==========================================
// CALLBACK DE RECEPÇÃO ESP-NOW
// ==========================================
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
#else
void onDataRecv(const uint8_t *macOrigem, const uint8_t *incomingData, int len) {
#endif
  if (len == sizeof(PacoteDados)) {
    memcpy(&pacoteRecebido, incomingData, sizeof(PacoteDados));
    novoPacoteRecebido = true;
    lastRxMillis = millis();
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Monitoramento ESP32 - Matriz 8x8 + Gateway Serial ===");

  matriz.begin();
  matriz.setIntensity(8); // Brilho de 0 a 15
  matriz.displayClear();

  carregarTextoTela();

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao inicializar o ESP-NOW!");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);
}

void loop() {
  // 1. ANIMAÇÃO DE SCROLL E AVANÇO CÍCLICO NA MATRIZ
  if (matriz.displayAnimate()) {
    telaAtual++;
    if (telaAtual > 5) {
      telaAtual = 1;
    }
    carregarTextoTela();
    matriz.displayReset();
  }

  // 2. ENVIO DO PACOTE VIA SERIAL EM FORMATO JSON (Consumido pelo serial_gateway.py)
  if (novoPacoteRecebido) {
    novoPacoteRecebido = false;
    Serial.printf("{\"nivel\":%d,\"temperatura\":%d,\"umidade\":%d,\"luminosidade\":%d,\"presenca\":%d,\"timestamp\":%lu}\n",
                  pacoteRecebido.nivel,
                  pacoteRecebido.temperatura,
                  pacoteRecebido.umidade,
                  pacoteRecebido.luminosidade,
                  presenca ? 1 : 0,
                  pacoteRecebido.timestamp);
  }

  // 3. CONTROLE DE TIMEOUT (> 5s)
  if (lastRxMillis > 0 && (millis() - lastRxMillis > TIMEOUT_MS)) {
    static unsigned long lastAlerta = 0;
    if (millis() - lastAlerta > 5000) {
      Serial.println("[ALERTA] Timeout: Sem comunicação há mais de 5s!");
      lastAlerta = millis();
    }
  }
}
