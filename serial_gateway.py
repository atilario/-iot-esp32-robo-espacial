import serial
import json
import requests
import time
import sys

# Ajuste a porta COM de acordo com o Gerenciador de Dispositivos (ex: COM3, COM4, COM5)
SERIAL_PORT = "COM3"
BAUD_RATE = 115200
API_URL = "http://127.0.0.1:8000/api/leituras"

def run_gateway():
    porta = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    print(f"[*] Tentando abrir a porta serial {porta} a {BAUD_RATE} bps...")
    
    try:
        ser = serial.Serial(porta, BAUD_RATE, timeout=2)
        time.sleep(2)  # Aguarda estabilizar a conexão
        print(f"[+] Conectado com sucesso na {porta}!")
        print(f"[*] Encaminhando leituras para {API_URL}...")

        while True:
            linha = ser.readline().decode("utf-8", errors="ignore").strip()
            if not linha:
                continue

            # Identifica pacotes JSON enviados pelo receptor
            if linha.startswith("{") and linha.endswith("}"):
                try:
                    payload = json.loads(linha)
                    resp = requests.post(API_URL, json=payload, timeout=3)
                    print(f"[OK HTTP {resp.status_code}] Dado gravado: {payload}")
                except json.JSONDecodeError:
                    print(f"[Aviso] JSON mal formatado: {linha}")
                except requests.RequestException as err:
                    print(f"[Erro API] Falha ao enviar para {API_URL}: {err}")
            else:
                # Log serial comum (ex: prints de depuração do Arduino)
                print(f"[ESP32]: {linha}")

    except serial.SerialException as err:
        print(f"[ERRO SERIAL] Não foi possível abrir {porta}: {err}")
        print("Dica: Verifique se o monitor serial do Arduino IDE ou VSCode está fechado e qual porta COM o ESP32 está usando.")

if __name__ == "__main__":
    run_gateway()
