import sqlite3
from datetime import datetime
from contextlib import asynccontextmanager
from typing import Optional, List, Annotated
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


@asynccontextmanager
async def lifespan(app: FastAPI):
    # Garante apenas que a estrutura do banco e tabela estejam criadas
    init_db()
    yield


app = FastAPI(
    title="API de Telemetria e Monitoramento ESP32",
    description="API REST construída com FastAPI e SQLite para persistência dos dados de sensores recebidos via ESP-NOW.",
    version="1.0.0",
    lifespan=lifespan,
)

# Habilita CORS para conexão com dashboards e interfaces
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# ============================================================================
# MODELOS PYDANTIC (VALIDAÇÃO E TIPAGEM DOS DADOS)
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
    Recebe os dados brutos enviados pelo ESP32 (direto via Wi-Fi ou pelo gateway serial)
    e grava no banco de dados SQLite.
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


@app.get("/api/leituras", response_model=List[LeituraResponse])
def listar_leituras(limite: Annotated[int, Query(ge=1, le=500, description="Quantidade de registros")] = 50):
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
    """Retorna apenas o registro mais recente (útil para cards ou telas em tempo real)."""
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
