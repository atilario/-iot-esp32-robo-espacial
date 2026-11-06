import streamlit as st
import sqlite3
import pandas as pd
from streamlit_autorefresh import st_autorefresh

# Configurações visuais da página
st.set_page_config(
    page_title="Monitoramento Industrial",
    page_icon="🏭",
    layout="wide"
)

# Atualiza a página automaticamente a cada 2000 ms (2 segundos)
st_autorefresh(interval=2000, limit=None, key="sensor_refresh")

st.title("🏭 Chão de Fábrica - Telemetria em Tempo Real")

# Função para buscar os dados do SQLite
def obter_dados():
    try:
        conn = sqlite3.connect("sensores.db")
        # Lê os últimos 60 registros
        df = pd.read_sql_query("SELECT * FROM leituras ORDER BY id DESC LIMIT 60", conn)
        conn.close()
        return df
    except Exception as e:
        st.error(f"Erro ao acessar o banco: {e}")
        return pd.DataFrame()

df = obter_dados()

if not df.empty:
    ultimo = df.iloc[0]

    # Linha de Indicadores / KPIs
    col1, col2, col3, col4, col5 = st.columns(5)
    
    col1.metric("Nível do Tanque", f"{ultimo['nivel']}%")
    col2.metric("Temperatura", f"{ultimo['temperatura']} °C")
    col3.metric("Umidade", f"{ultimo['umidade']}%")
    col4.metric("Luminosidade", f"{ultimo['luminosidade']} lux")
    col5.metric("Presença", "ON" if ultimo['presenca'] == 1 else "OFF")

    st.markdown("---")

    # Gráfico temporal
    st.subheader("Histórico de Leituras")
    
    # Inverte para ordem cronológica (antigo -> novo)
    df_grafico = df.iloc[::-1].copy()
    
    # Gráfico interativo com as grandezas analógicas
    st.line_chart(
        df_grafico.set_index("created_at")[["temperatura", "umidade", "nivel"]]
    )

    # Tabela com as últimas leituras
    with st.expander("Ver histórico tabular"):
        st.dataframe(df, use_container_width=True)
else:
    st.info("Aguardando o recebimento dos primeiros dados via ESP-NOW...")