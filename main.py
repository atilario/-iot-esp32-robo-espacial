from datetime import datetime
import sqlite3
from contextlib import asynccontextmanager
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel

DB_NAME = "leituras.db"


def init_db():
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS leituras (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            temperatura_c REAL NOT NULL,
            umidade_pct REAL NOT NULL,
            luminosidade REAL NOT NULL,
            presenca INTEGER NOT NULL,
            probabilidade_vida REAL NOT NULL
        )
    """)
    conn.commit()
    conn.close()


@asynccontextmanager
async def lifespan(app: FastAPI):
    init_db()
    yield


app = FastAPI(
    title="API de Leituras de Sensores",
    description="API para armazenamento e consulta de dados ambientais e de presença.",
    version="1.0.0",
    lifespan=lifespan,
)


class LeituraCreate(BaseModel):
    temperatura_c: float
    umidade_pct: float
    luminosidade: float
    presenca: int
    probabilidade_vida: float
    timestamp: str | None = None


class LeituraResponse(LeituraCreate):
    id: int
    timestamp: str


@app.post("/leituras", response_model=LeituraResponse, status_code=201)
def criar_leitura(leitura: LeituraCreate):
    ts = leitura.timestamp or datetime.now().isoformat()

    try:
        conn = sqlite3.connect(DB_NAME)
        cursor = conn.cursor()
        cursor.execute(
            """
            INSERT INTO leituras (timestamp, temperatura_c, umidade_pct, luminosidade, presenca, probabilidade_vida)
            VALUES (?, ?, ?, ?, ?, ?)
            """,
            (
                ts,
                leitura.temperatura_c,
                leitura.umidade_pct,
                leitura.luminosidade,
                leitura.presenca,
                leitura.probabilidade_vida,
            ),
        )
        conn.commit()
        leitura_id = cursor.lastrowid
        conn.close()

        return {
            "id": leitura_id,
            "timestamp": ts,
            "temperatura_c": leitura.temperatura_c,
            "umidade_pct": leitura.umidade_pct,
            "luminosidade": leitura.luminosidade,
            "presenca": leitura.presenca,
            "probabilidade_vida": leitura.probabilidade_vida,
        }
    except Exception as e:
        raise HTTPException(
            status_code=500, detail=f"Erro ao salvar no banco de dados: {str(e)}"
        )


@app.get("/leituras", response_model=list[LeituraResponse])
def obter_leituras():
    try:
        conn = sqlite3.connect(DB_NAME)
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
        cursor.execute("""
            SELECT id, timestamp, temperatura_c, umidade_pct, luminosidade, presenca, probabilidade_vida 
            FROM leituras 
            ORDER BY id DESC 
            LIMIT 100
        """)
        rows = cursor.fetchall()
        conn.close()

        return [dict(row) for row in rows]
    except Exception as e:
        raise HTTPException(
            status_code=500, detail=f"Erro ao consultar o banco de dados: {str(e)}"
        )
