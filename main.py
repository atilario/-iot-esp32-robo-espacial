import sqlite3
import random
from datetime import datetime, timedelta
from contextlib import asynccontextmanager
from typing import Optional, List
from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field

DB_NAME = "sensores.db"


def init_db():
    """Cria a tabela e índices no SQLite caso ainda não existam."""
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS leituras (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            nivel INTEGER NOT NULL,
            temperatura REAL NOT NULL,
            umidade REAL NOT NULL,
            luminosidade INTEGER NOT NULL,
            presenca INTEGER NOT NULL,
            timestamp_esp INTEGER NOT NULL,
            created_at TEXT NOT NULL
        )
    """)
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_leituras_created_at ON leituras(created_at)")
    conn.commit()
    conn.close()


def popular_dados_iniciais(quantidade: int = 15):
    """Povoa o banco com leituras de exemplo simulando o robô/ESP32 caso esteja vazio."""
    init_db()
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("SELECT COUNT(*) FROM leituras")
    total = cursor.fetchone()[0]

    if total == 0:
        agora = datetime.now()
        leituras_mock = []
        for i in range(quantidade):
            tempo_reg = (agora - timedelta(minutes=(quantidade - i) * 2)).strftime("%Y-%m-%d %H:%M:%S")
            nivel = random.randint(70, 95)
            temperatura = round(random.uniform(22.0, 28.5), 1)
            umidade = round(random.uniform(50.0, 75.0), 1)
            luminosidade = random.randint(900, 2500)
            presenca = 1 if random.random() > 0.4 else 0
            ts_esp = 100000 + i * 120000

            leituras_mock.append((nivel, temperatura, umidade, luminosidade, presenca, ts_esp, tempo_reg))

        cursor.executemany("""
            INSERT INTO leituras (nivel, temperatura, umidade, luminosidade, presenca, timestamp_esp, created_at)
            VALUES (?, ?, ?, ?, ?, ?, ?)
        """, leituras_mock)
        conn.commit()
        print(f"[SQLite] Banco inicializado e povoado com {quantidade} registros iniciais.")

    conn.close()


@asynccontextmanager
async def lifespan(app: FastAPI):
    # Ao iniciar a API, garante a criação do banco e povoamento inicial
    popular_dados_iniciais(15)
    yield


app = FastAPI(
    title="API de Telemetria e Monitoramento ESP32",
    description="API REST construída com FastAPI e SQLite para persistência dos dados de sensores recebidos via ESP-NOW.",
    version="1.0.0",
    lifespan=lifespan,
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# ============================================================================
# MODELOS PYDANTIC
# ============================================================================
class LeituraCreate(BaseModel):
    nivel: int = Field(..., description="Nível do tanque ou reservatório (%)")
    temperatura: float = Field(..., description="Temperatura ambiente (°C)")
    umidade: float = Field(..., description="Umidade relativa do ar (%)")
    luminosidade: int = Field(..., description="Luminosidade (valor lido LDR / LUX)")
    presenca: int = Field(..., description="Detecção de presença (0 = ausente, 1 = presente)")
    timestamp: int = Field(..., description="Timestamp ou millis() enviado pelo ESP32")


class LeituraResponse(BaseModel):
    id: int
    nivel: int
    temperatura: float
    umidade: float
    luminosidade: int
    presenca: int
    timestamp_esp: int
    created_at: str


# ============================================================================
# ENDPOINTS
# ============================================================================
@app.get("/")
def raiz():
    return {
        "status": "online",
        "servico": "API Telemetria ESP32 (FastAPI + SQLite)",
        "docs": "/docs",
    }


@app.post("/api/leituras", response_model=dict, status_code=201)
def receber_leitura(dados: LeituraCreate):
    """
    Recebe os dados brutos enviados pelo ESP32 (ou via serial_gateway.py)
    e grava no SQLite.
    """
    agora_iso = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    try:
        conn = sqlite3.connect(DB_NAME)
        cursor = conn.cursor()
        cursor.execute(
            """
            INSERT INTO leituras (nivel, temperatura, umidade, luminosidade, presenca, timestamp_esp, created_at)
            VALUES (?, ?, ?, ?, ?, ?, ?)
            """,
            (
                dados.nivel,
                dados.temperatura,
                dados.umidade,
                dados.luminosidade,
                dados.presenca,
                dados.timestamp,
                agora_iso,
            ),
        )
        conn.commit()
        novo_id = cursor.lastrowid
        conn.close()

        return {
            "status": "sucesso",
            "mensagem": "Leitura gravada com sucesso!",
            "id": novo_id,
            "created_at": agora_iso,
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Erro ao salvar no banco SQLite: {str(e)}")


@app.post("/api/povoar", response_model=dict, status_code=201)
def povoar_banco(quantidade: int = Query(default=10, ge=1, le=100, description="Quantidade de registros a gerar")):
    """
    Endpoint manual para povoar o banco SQLite com dados simulados sob demanda.
    """
    agora = datetime.now()
    leituras_geradas = []
    for i in range(quantidade):
        tempo_reg = (agora - timedelta(minutes=(quantidade - i) * 2)).strftime("%Y-%m-%d %H:%M:%S")
        nivel = random.randint(60, 100)
        temperatura = round(random.uniform(20.0, 32.0), 1)
        umidade = round(random.uniform(40.0, 80.0), 1)
        luminosidade = random.randint(800, 3000)
        presenca = 1 if random.random() > 0.5 else 0
        ts_esp = int(datetime.now().timestamp() * 1000) + (i * 1000)

        leituras_geradas.append((nivel, temperatura, umidade, luminosidade, presenca, ts_esp, tempo_reg))

    try:
        conn = sqlite3.connect(DB_NAME)
        cursor = conn.cursor()
        cursor.executemany("""
            INSERT INTO leituras (nivel, temperatura, umidade, luminosidade, presenca, timestamp_esp, created_at)
            VALUES (?, ?, ?, ?, ?, ?, ?)
        """, leituras_geradas)
        conn.commit()
        conn.close()

        return {
            "status": "sucesso",
            "mensagem": f"{quantidade} registros simulados inseridos com sucesso no SQLite!"
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Erro ao povoar SQLite: {str(e)}")


@app.get("/api/leituras", response_model=List[LeituraResponse])
def listar_leituras(limite: int = Query(default=20, ge=1, le=500, description="Quantidade de registros")):
    """
    Lista as últimas leituras gravadas em ordem decrescente (mais recente primeiro).
    """
    try:
        conn = sqlite3.connect(DB_NAME)
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
        cursor.execute(
            """
            SELECT id, nivel, temperatura, umidade, luminosidade, presenca, timestamp_esp, created_at
            FROM leituras
            ORDER BY id DESC
            LIMIT ?
            """,
            (limite,),
        )
        linhas = cursor.fetchall()
        conn.close()

        return [dict(row) for row in linhas]
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Erro ao consultar SQLite: {str(e)}")


@app.get("/api/leituras/ultima", response_model=Optional[LeituraResponse])
def obter_ultima_leitura():
    """Retorna apenas o registro mais recente (útil para cards de dashboard)."""
    try:
        conn = sqlite3.connect(DB_NAME)
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
        cursor.execute(
            """
            SELECT id, nivel, temperatura, umidade, luminosidade, presenca, timestamp_esp, created_at
            FROM leituras
            ORDER BY id DESC
            LIMIT 1
            """
        )
        linha = cursor.fetchone()
        conn.close()

        if not linha:
            return None

        return dict(linha)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Erro ao consultar SQLite: {str(e)}")
