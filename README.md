# Robô Espacial Explorador - Controle Remoto (Etapa 1)

Repositório do projeto **iot-esp32-robo-espacial** desenvolvido para a prática da Camada de Percepção / Serviço (SENAI CIMATEC).

---

##  Objetivo da Etapa
Desenvolver e simular no **Wokwi** o **controle remoto** do robô explorador espacial utilizando uma placa **ESP32**. O controle é responsável por:
- Ler os comandos direcionais através de um **joystick analógico** (Frente, Trás, Esquerda, Direita).
- Enviar comando para parar/desligar remotamente o robô através de um **botão de desligar**.
- Indicar o status da comunicação através de **LEDs**:
  - **LED Verde:** controle remoto conectado e funcionando normalmente.
  - **LED Vermelho:** robô desligado ou desconectado.
- Exibir os comandos e o status continuamente no **Monitor Serial**.

---

##  Componentes do Circuito
- **1x** Placa ESP32
- **1x** Módulo Joystick Analógico
- **1x** Push Button (Botão remoto de desligar)
- **1x** LED Verde (Status: Conectado/Normal)
- **1x** LED Vermelho (Status: Desligado/Desconectado)
- Resistores de 220Ω para os LEDs 

---

## 🔗 Link do Projeto no Velxio
- [Acessar Simulação no Velxio](https://velxio.dev/project/30a22f6f-aa6a-4abe-a6c6-5e90e69ef379)

---

## 🚀 Como Rodar no Velxio
1. Abra o link do projeto no **Velxio**.
2. Clique no botão **Start Simulation** (botão verde de "Play").
3. Abra a aba **Serial Monitor** para visualizar as saídas e mensagens enviadas.
4. **Testes do circuito:**
   - Movimente o analógico do **joystick** para verificar o envio dos comandos (`Frente`, `Trás`, `Esquerda`, `Direita`).
   - Pressione o **botão** para testar o envio do comando `DESLIGAR` e a troca de status nos LEDs.
