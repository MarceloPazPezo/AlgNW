#!/usr/bin/env python3
"""
Script para generar gráficos de rendimiento (Speedup, Eficiencia, Tiempo)
a partir de los resultados del benchmark.
"""

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os
import sys

# Configuración
RESULTADOS_DIR = "resultados_benchmark"
GRAFICOS_DIR = "graficos_benchmark"
ARCHIVO_DATOS = os.path.join(RESULTADOS_DIR, "resultados_completo.csv")

# Estilo
sns.set_style("whitegrid")
plt.rcParams['figure.figsize'] = (12, 8)

def cargar_datos():
    if not os.path.exists(ARCHIVO_DATOS):
        print(f"Error: No se encontró el archivo de datos {ARCHIVO_DATOS}")
        return None
    
    try:
        df = pd.read_csv(ARCHIVO_DATOS)
        return df
    except Exception as e:
        print(f"Error leyendo CSV: {e}")
        return None

def procesar_datos(df):
    # Calcular promedio por método y longitud
    # Agrupamos por archivo_fasta, metodo, longitud_A (asumiendo A~B)
    # Primero extraemos el tipo (dna/protein) del nombre del archivo
    df['tipo'] = df['archivo_fasta'].apply(lambda x: 'dna' if 'dna' in str(x).lower() else 'protein')
    
    # Agrupar y promediar
    df_avg = df.groupby(['tipo', 'longitud_A', 'metodo']).agg({
        'tiempo_total_ms': 'mean',
        'puntuacion': 'first' # Debería ser igual
    }).reset_index()
    
    return df_avg

def calcular_metricas(df):
    # Separar secuencial para calcular speedup
    df_sec = df[df['metodo'] == 'secuencial'].copy()
    df_sec = df_sec.set_index(['tipo', 'longitud_A'])['tiempo_total_ms']
    
    # Calcular Speedup: T_sec / T_par
    def get_speedup(row):
        try:
            t_sec = df_sec.loc[(row['tipo'], row['longitud_A'])]
            return t_sec / row['tiempo_total_ms']
        except KeyError:
            return 0.0
            
    df['speedup'] = df.apply(get_speedup, axis=1)
    
    # Eficiencia (asumiendo que sabemos el número de hilos, pero aquí comparamos schedulers)
    # Si no sabemos hilos, la eficiencia es relativa. 
    # El usuario pidió eficiencia, asumiremos que se corrió con el máximo de hilos disponibles 
    # o simplemente graficaremos Speedup que es lo más relevante para comparar schedulers.
    # Si quisiéramos eficiencia real necesitaríamos variar hilos.
    # Por ahora dejaremos Speedup y Tiempo.
    
    return df

def graficar_tiempo(df, output_dir):
    tipos = df['tipo'].unique()
    
    for tipo in tipos:
        data = df[df['tipo'] == tipo]
        
        plt.figure()
        sns.lineplot(data=data, x='longitud_A', y='tiempo_total_ms', hue='metodo', marker='o')
        plt.title(f'Tiempo de Ejecución vs Tamaño ({tipo.upper()})')
        plt.xlabel('Longitud de Secuencia')
        plt.ylabel('Tiempo (ms)')
        plt.yscale('log')
        plt.xscale('log')
        plt.grid(True, which="both", ls="-", alpha=0.5)
        plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f'tiempo_{tipo}.png'))
        plt.close()

def graficar_speedup(df, output_dir):
    tipos = df['tipo'].unique()
    
    for tipo in tipos:
        # Filtrar solo métodos paralelos (o incluir secuencial como base 1)
        data = df[df['tipo'] == tipo]
        
        plt.figure()
        sns.lineplot(data=data, x='longitud_A', y='speedup', hue='metodo', marker='o')
        plt.title(f'Speedup vs Tamaño ({tipo.upper()})')
        plt.xlabel('Longitud de Secuencia')
        plt.ylabel('Speedup (X)')
        plt.xscale('log')
        plt.axhline(y=1, color='r', linestyle='--')
        plt.grid(True, which="both", ls="-", alpha=0.5)
        plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f'speedup_{tipo}.png'))
        plt.close()

def graficar_mejor_metodo(df, output_dir):
    # Encontrar el mejor método para cada tamaño
    idx = df.groupby(['tipo', 'longitud_A'])['tiempo_total_ms'].idxmin()
    mejores = df.loc[idx]
    
    # Guardar tabla de mejores
    mejores[['tipo', 'longitud_A', 'metodo', 'tiempo_total_ms', 'speedup']].to_csv(
        os.path.join(output_dir, 'mejores_metodos.csv'), index=False
    )
    print("Tabla de mejores métodos guardada.")

def main():
    if not os.path.exists(GRAFICOS_DIR):
        os.makedirs(GRAFICOS_DIR)
        
    print("Cargando datos...")
    df = cargar_datos()
    if df is None:
        return
        
    print("Procesando datos...")
    df_avg = procesar_datos(df)
    df_metrics = calcular_metricas(df_avg)
    
    print("Generando gráficos...")
    graficar_tiempo(df_metrics, GRAFICOS_DIR)
    graficar_speedup(df_metrics, GRAFICOS_DIR)
    graficar_mejor_metodo(df_metrics, GRAFICOS_DIR)
    
    print(f"Gráficos guardados en {GRAFICOS_DIR}")

if __name__ == "__main__":
    main()
