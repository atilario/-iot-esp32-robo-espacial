import sqlite3
from datetime import datetime, timezone
from contextlib import asynccontextmanager
from typing import Optional, List, Annotated
from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field

DB_NAME = "lampada.db"


def init_db():
    """Cria a tabela e índices no SQLite caso ainda não existam."""
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS telemetria (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            temperatura REAL NOT NULL,
            luminosidade INTEGER NOT NULL,
            potenciometro INTEGER NOT NULL,
            led_ligado INTEGER NOT NULL,
            perigo INTEGER NOT NULL,
            sistema_ativo INTEGER NOT NULL,
            created_at TEXT NOT NULL
        )
    """)
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_telemetria_created_at ON telemetria(created_at)")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_telemetria_timestamp ON telemetria(timestamp)")
    conn.commit()
    conn.close()


@asynccontextmanager
async def lifespan(app: FastAPI):
    # Inicializa o banco de dados SQLite ao subir a API
    init_db()
    yield


app = FastAPI(
    title="API Lâmpada Inteligente - Telemetria ESP32",
    description="API REST FastAPI para recebimento de telemetria da Lâmpada Inteligente (ESP32-S3) com Timestamp NTP e persistência em SQLite.",
    version="1.1.0",
    lifespan=lifespan,
)

# Libera CORS para conexões de dashboards e navegadores
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# ============================================================================
# MODELOS DE DADOS (PYDANTIC)
# ============================================================================
class TelemetriaCreate(BaseModel):
    timestamp: int = Field(..., description="Timestamp Unix (epoch em segundos) gerado via NTP pelo ESP32")
    temperatura: float = Field(..., description="Temperatura ambiente lida pelo sensor DHT22 (°C)")
    luminosidade: int = Field(..., description="Valor analógico lido pelo LDR")
    potenciometro: int = Field(..., description="Valor analógico lido pelo potenciômetro")
    led_ligado: bool = Field(..., description="Estado do LED RGB (true=ligado, false=desligado)")
    perigo: bool = Field(..., description="Indicação de perigo térmico")
    sistema_ativo: bool = Field(..., description="Indica se o sistema está ativo ou desativado/mudo")


class TelemetriaResponse(BaseModel):
    id: int
    timestamp: int
    temperatura: float
    luminosidade: int
    potenciometro: int
    led_ligado: bool
    perigo: bool
    sistema_ativo: bool
    created_at: str


# ============================================================================
# ENDPOINTS REST
# ============================================================================
@app.get("/")
def raiz():
    return {
        "status": "online",
        "servico": "API Telemetria Lâmpada Inteligente",
        "docs": "/docs",
    }


@app.post("/telemetria", response_model=dict, status_code=201)
def receber_telemetria(dados: TelemetriaCreate):
    """
    Endpoint chamado pelo ESP32 via HTTP POST.
    Recebe os dados com timestamp e grava no SQLite.
    """
    # Converte o timestamp Unix para data legível se for válido, senão usa a hora do servidor
    if dados.timestamp and dados.timestamp > 1000000000:
        data_formatada = datetime.fromtimestamp(dados.timestamp, tz=timezone.utc).strftime("%Y-%m-%d %H:%M:%S")
    else:
        data_formatada = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    try:
        conn = sqlite3.connect(DB_NAME)
        cursor = conn.cursor()
        cursor.execute(
            """
            INSERT INTO telemetria (timestamp, temperatura, luminosidade, potenciometro, led_ligado, perigo, sistema_ativo, created_at)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                dados.timestamp,
                dados.temperatura,
                dados.luminosidade,
                dados.potenciometro,
                1 if dados.led_ligado else 0,
                1 if dados.perigo else 0,
                1 if dados.sistema_ativo else 0,
                data_formatada,
            ),
        )
        conn.commit()
        novo_id = cursor.lastrowid
        conn.close()

        return {
            "status": "sucesso",
            "mensagem": "Telemetria registrada com sucesso!",
            "id": novo_id,
            "timestamp": dados.timestamp,
            "created_at": data_formatada,
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Erro ao salvar no banco SQLite: {str(e)}")


@app.get("/telemetria", response_model=List[TelemetriaResponse])
def listar_telemetria(limite: Annotated[int, Query(ge=1, le=500, description="Quantidade de registros")] = 50):
    """
    Lista os últimos registros salvos em ordem decrescente (mais recente primeiro).
    """
    try:
        conn = sqlite3.connect(DB_NAME)
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
        cursor.execute(
            """
            SELECT id, timestamp, temperatura, luminosidade, potenciometro, led_ligado, perigo, sistema_ativo, created_at
            FROM telemetria
            ORDER BY id DESC
            LIMIT ?
            """,
            (limite,),
        )
        linhas = cursor.fetchall()
        conn.close()

        resultado = []
        for row in linhas:
            resultado.append(
                TelemetriaResponse(
                    id=row["id"],
                    timestamp=row["timestamp"],
                    temperatura=row["temperatura"],
                    luminosidade=row["luminosidade"],
                    potenciometro=row["potenciometro"],
                    led_ligado=bool(row["led_ligado"]),
                    perigo=bool(row["perigo"]),
                    sistema_ativo=bool(row["sistema_ativo"]),
                    created_at=row["created_at"],
                )
            )

        return resultado
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Erro ao consultar SQLite: {str(e)}")


@app.get("/telemetria/ultima", response_model=Optional[TelemetriaResponse])
def obter_ultima_telemetria():
    """
    Retorna apenas a leitura mais recente (ideal para exibição de status em tempo real).
    """
    try:
        conn = sqlite3.connect(DB_NAME)
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
        cursor.execute(
            """
            SELECT id, timestamp, temperatura, luminosidade, potenciometro, led_ligado, perigo, sistema_ativo, created_at
            FROM telemetria
            ORDER BY id DESC
            LIMIT 1
            """
        )
        linha = cursor.fetchone()
        conn.close()

        if not linha:
            return None

        return TelemetriaResponse(
            id=linha["id"],
            timestamp=linha["timestamp"],
            temperatura=linha["temperatura"],
            luminosidade=linha["luminosidade"],
            potenciometro=linha["potenciometro"],
            led_ligado=bool(linha["led_ligado"]),
            perigo=bool(linha["perigo"]),
            sistema_ativo=bool(linha["sistema_ativo"]),
            created_at=linha["created_at"],
        )
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Erro ao consultar SQLite: {str(e)}")
