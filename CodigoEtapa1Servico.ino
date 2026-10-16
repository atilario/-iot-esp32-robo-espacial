/*
  Projeto: Controle Remoto Robô Explorador (Wokwi)
  Placa: ESP32-S3
  Etapa 01 - Práticas Integradas: Camada de Percepção (Frente e Trás Ajustados)
*/

// Definição dos Pinos na ESP32-S3
#define PIN_JOYSTICK_X 4   // Eixo X do Joystick
#define PIN_JOYSTICK_Y 5   // Eixo Y do Joystick
#define PIN_BTN_STOP   6   // Botão de Parada/Religar Remoto
#define PIN_LED_GREEN  15  // LED Status: Ligado / Conectado
#define PIN_LED_RED    16  // LED Status: Desligado / Parado

// Parâmetros de leitura do ADC (ESP32 possui resolução de 12 bits: 0 a 4095)
#define ADC_CENTER    2048
#define ADC_THRESHOLD 800  // Margem de tolerância para considerar movimento

bool sistemaLigado = true;
String ultimoComando = "";
int ultimoEstadoBotao = HIGH;

void setup() {
  Serial.begin(115200);
  
  // Configuração dos pinos dos LEDs
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  // Configuração do Botão com Pull-Up interno
  pinMode(PIN_BTN_STOP, INPUT_PULLUP);

  // Estado Inicial do Sistema
  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_RED, LOW);

  Serial.println("==========================================");
  Serial.println("Controle Remoto ESP32-S3 Inicializado!");
  Serial.println("Status: Robô LIGADO e CONECTADO (LED Verde)");
  Serial.println("==========================================");
}

void loop() {
  // 1. Leitura do botão com detecção de clique (Borda de Descida)
  int estadoAtualBotao = digitalRead(PIN_BTN_STOP);

  if (ultimoEstadoBotao == HIGH && estadoAtualBotao == LOW) {
    sistemaLigado = !sistemaLigado;

    if (sistemaLigado) {
      digitalWrite(PIN_LED_GREEN, HIGH);
      digitalWrite(PIN_LED_RED, LOW);
      
      Serial.println("\n[SISTEMA] Botão Pressionado!");
      Serial.println("Comando enviado: LIGAR / RECONECTAR");
      Serial.println("Status do Robô: LIGADO e OPERACIONAL (LED Verde)");
      Serial.println("------------------------------------------");
    } else {
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_RED, HIGH);
      
      Serial.println("\n[ALERTA] Botão Pressionado!");
      Serial.println("Comando enviado: DESLIGAR");
      Serial.println("Status do Robô: DESLIGADO / DESCONECTADO (LED Vermelho)");
      Serial.println("------------------------------------------");
    }

    delay(200); // Debounce
  }
  
  ultimoEstadoBotao = estadoAtualBotao;

  if (!sistemaLigado) {
    delay(100);
    return;
  }

  // 2. Leitura dos eixos do Joystick
  int valX = analogRead(PIN_JOYSTICK_X);
  int valY = analogRead(PIN_JOYSTICK_Y);

  String comandoAtual = "PARADO";

  // Invertido: VRY maior que o centro agora envia "Frente" e menor envia "Tras"
  if (valY > (ADC_CENTER + ADC_THRESHOLD)) {
    comandoAtual = "Frente";
  } else if (valY < (ADC_CENTER - ADC_THRESHOLD)) {
    comandoAtual = "Tras";
  } else if (valX < (ADC_CENTER - ADC_THRESHOLD)) {
    comandoAtual = "Esquerda";
  } else if (valX > (ADC_CENTER + ADC_THRESHOLD)) {
    comandoAtual = "Direita";
  }

  // Imprime no Monitor Serial apenas quando houver alteração de movimento ou a cada 1 segundo
  static unsigned long ultimaAtualizacao = 0;
  if (comandoAtual != ultimoComando || millis() - ultimaAtualizacao > 1000) {
    ultimoComando = comandoAtual;
    ultimaAtualizacao = millis();

    Serial.print("Comando: ");
    Serial.print(comandoAtual);
    Serial.print(" | Status: Conectado (LED Verde) | Eixo X: ");
    Serial.print(valX);
    Serial.print(" Y: ");
    Serial.println(valY);
  }

  delay(100);
}