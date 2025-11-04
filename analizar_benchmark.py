#!/usr/bin/env python3
"""
Script para analizar los resultados del benchmark y generar gráficos
Muestra porcentajes de tiempo por fase y múltiples visualizaciones
"""

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import os
import glob
import argparse
from pathlib import Path

# Configurar estilo de gráficos
sns.set_style("whitegrid")
plt.rcParams['figure.figsize'] = (14, 8)
plt.rcParams['font.size'] = 10

def extraer_longitud(archivo):
    """Extrae la longitud del nombre del archivo"""
    nombre = os.path.basename(archivo)
    # Buscar patrones como dna_1k, dna_500, dna_100k
    import re
    match = re.search(r'(\d+)k', nombre)
    if match:
        return int(match.group(1)) * 1000
    match = re.search(r'_(\d+)\.fasta', nombre)
    if match:
        return int(match.group(1))
    return 0

def cargar_datos_promedios(directorio):
    """Carga todos los archivos *_promedio.csv y extrae tiempos por fase"""
    archivos_promedio = glob.glob(os.path.join(directorio, "*_promedio.csv"))
    
    datos = []
    
    for archivo in sorted(archivos_promedio):
        try:
            df = pd.read_csv(archivo)
            
            # Extraer nombre base del archivo
            nombre_base = os.path.basename(archivo).replace("_promedio.csv", "")
            
            # Buscar filas correspondientes a cada fase
            init_row = df[df['tipo_metric'] == 'tiempo_init_ms']
            llenado_row = df[df['tipo_metric'] == 'tiempo_llenado_ms']
            traceback_row = df[df['tipo_metric'] == 'tiempo_traceback_ms']
            total_row = df[df['tipo_metric'] == 'tiempo_total_ms']
            
            if len(init_row) > 0 and len(llenado_row) > 0 and len(traceback_row) > 0:
                datos.append({
                    'archivo': nombre_base,
                    'tipo': 'dna' if 'dna' in nombre_base.lower() else 'protein',
                    'longitud': extraer_longitud(archivo),
                    'init_ms': init_row.iloc[0]['promedio'],
                    'llenado_ms': llenado_row.iloc[0]['promedio'],
                    'traceback_ms': traceback_row.iloc[0]['promedio'],
                    'total_ms': total_row.iloc[0]['promedio'],
                    'num_ejecuciones': int(init_row.iloc[0]['num_ejecuciones'])
                })
        except Exception as e:
            print(f"Error procesando {archivo}: {e}")
            continue
    
    return pd.DataFrame(datos)

def calcular_porcentajes(df):
    """Calcula porcentajes de tiempo por fase"""
    df['pct_init'] = (df['init_ms'] / df['total_ms']) * 100
    df['pct_llenado'] = (df['llenado_ms'] / df['total_ms']) * 100
    df['pct_traceback'] = (df['traceback_ms'] / df['total_ms']) * 100
    
    # Validar que sumen aproximadamente 100%
    suma = df['pct_init'] + df['pct_llenado'] + df['pct_traceback']
    if (suma > 100.1).any() or (suma < 99.9).any():
        print(f"Advertencia: Algunos porcentajes no suman 100% (rango: {suma.min():.2f}% - {suma.max():.2f}%)")
    
    return df

def grafico_porcentajes_apilados(df, output_dir):
    """Gráfico de barras apiladas mostrando porcentajes por fase"""
    df_ordenado = df.sort_values('longitud')
    
    fig, ax = plt.subplots(figsize=(16, 10))
    
    x = np.arange(len(df_ordenado))
    width = 0.8
    
    p1 = ax.bar(x, df_ordenado['pct_init'], width, label='Inicialización', color='#FF6B6B')
    p2 = ax.bar(x, df_ordenado['pct_llenado'], width, bottom=df_ordenado['pct_init'], 
                label='Llenado de Matriz', color='#4ECDC4')
    p3 = ax.bar(x, df_ordenado['pct_traceback'], width, 
                bottom=df_ordenado['pct_init'] + df_ordenado['pct_llenado'],
                label='Traceback', color='#45B7D1')
    
    # Etiquetas y formato
    ax.set_xlabel('Archivo (ordenado por longitud)', fontsize=12, fontweight='bold')
    ax.set_ylabel('Porcentaje del Tiempo Total (%)', fontsize=12, fontweight='bold')
    ax.set_title('Distribución de Tiempo por Fase del Alineamiento\n(Como porcentaje del tiempo total)', 
                 fontsize=14, fontweight='bold', pad=20)
    ax.set_xticks(x)
    ax.set_xticklabels([f"{row['archivo']}\n({row['longitud']} chars)" 
                        for _, row in df_ordenado.iterrows()], 
                       rotation=45, ha='right', fontsize=9)
    ax.legend(loc='upper left', fontsize=11)
    ax.set_ylim(0, 100)
    ax.grid(axis='y', alpha=0.3)
    
    # Agregar líneas de referencia en 50% y 75%
    ax.axhline(y=50, color='gray', linestyle='--', alpha=0.5, linewidth=1)
    ax.axhline(y=75, color='gray', linestyle='--', alpha=0.5, linewidth=1)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'porcentajes_por_fase_apilado.png'), 
                dpi=300, bbox_inches='tight')
    plt.close()
    print(f"✓ Gráfico guardado: porcentajes_por_fase_apilado.png")

def grafico_tiempos_absolutos(df, output_dir):
    """Gráfico de líneas mostrando tiempos absolutos por fase vs longitud"""
    df_ordenado = df.sort_values('longitud')
    
    fig, ax = plt.subplots(figsize=(14, 8))
    
    ax.plot(df_ordenado['longitud'], df_ordenado['init_ms'], 
            marker='o', label='Inicialización', linewidth=2, markersize=8, color='#FF6B6B')
    ax.plot(df_ordenado['longitud'], df_ordenado['llenado_ms'], 
            marker='s', label='Llenado de Matriz', linewidth=2, markersize=8, color='#4ECDC4')
    ax.plot(df_ordenado['longitud'], df_ordenado['traceback_ms'], 
            marker='^', label='Traceback', linewidth=2, markersize=8, color='#45B7D1')
    ax.plot(df_ordenado['longitud'], df_ordenado['total_ms'], 
            marker='D', label='Total', linewidth=3, markersize=10, color='#2C3E50', linestyle='--')
    
    ax.set_xlabel('Longitud de Secuencias (caracteres)', fontsize=12, fontweight='bold')
    ax.set_ylabel('Tiempo (ms)', fontsize=12, fontweight='bold')
    ax.set_title('Tiempos Absolutos por Fase vs Longitud de Secuencias', 
                 fontsize=14, fontweight='bold', pad=20)
    ax.legend(loc='upper left', fontsize=11)
    ax.grid(True, alpha=0.3)
    ax.set_xscale('log')
    ax.set_yscale('log')
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'tiempos_absolutos_por_fase.png'), 
                dpi=300, bbox_inches='tight')
    plt.close()
    print(f"✓ Gráfico guardado: tiempos_absolutos_por_fase.png")

def grafico_comparativo_fases(df, output_dir):
    """Gráfico comparativo mostrando cómo crece cada fase con el tamaño"""
    df_ordenado = df.sort_values('longitud')
    
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    fig.suptitle('Análisis Detallado de Tiempos por Fase', 
                 fontsize=16, fontweight='bold', y=0.995)
    
    # Subplot 1: Tiempos absolutos
    ax1 = axes[0, 0]
    ax1.plot(df_ordenado['longitud'], df_ordenado['init_ms'], 
             marker='o', label='Inicialización', color='#FF6B6B')
    ax1.plot(df_ordenado['longitud'], df_ordenado['llenado_ms'], 
             marker='s', label='Llenado', color='#4ECDC4')
    ax1.plot(df_ordenado['longitud'], df_ordenado['traceback_ms'], 
             marker='^', label='Traceback', color='#45B7D1')
    ax1.set_xlabel('Longitud')
    ax1.set_ylabel('Tiempo (ms)')
    ax1.set_title('Tiempos Absolutos')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    ax1.set_xscale('log')
    ax1.set_yscale('log')
    
    # Subplot 2: Porcentajes
    ax2 = axes[0, 1]
    x = np.arange(len(df_ordenado))
    width = 0.6
    ax2.bar(x - width/3, df_ordenado['pct_init'], width/3, label='Init %', color='#FF6B6B')
    ax2.bar(x, df_ordenado['pct_llenado'], width/3, label='Llenado %', color='#4ECDC4')
    ax2.bar(x + width/3, df_ordenado['pct_traceback'], width/3, label='Traceback %', color='#45B7D1')
    ax2.set_xlabel('Archivo')
    ax2.set_ylabel('Porcentaje (%)')
    ax2.set_title('Porcentajes por Fase')
    ax2.set_xticks(x)
    ax2.set_xticklabels([row['archivo'] for _, row in df_ordenado.iterrows()], 
                        rotation=45, ha='right', fontsize=8)
    ax2.legend()
    ax2.grid(True, alpha=0.3, axis='y')
    
    # Subplot 3: Porcentaje de llenado (la fase más importante)
    ax3 = axes[1, 0]
    ax3.bar(range(len(df_ordenado)), df_ordenado['pct_llenado'], color='#4ECDC4')
    ax3.set_xlabel('Archivo')
    ax3.set_ylabel('Porcentaje (%)')
    ax3.set_title('Porcentaje de Tiempo en Llenado de Matriz')
    ax3.set_xticks(range(len(df_ordenado)))
    ax3.set_xticklabels([row['archivo'] for _, row in df_ordenado.iterrows()], 
                        rotation=45, ha='right', fontsize=8)
    ax3.axhline(y=df_ordenado['pct_llenado'].mean(), 
                color='red', linestyle='--', label=f'Promedio: {df_ordenado["pct_llenado"].mean():.1f}%')
    ax3.legend()
    ax3.grid(True, alpha=0.3, axis='y')
    
    # Subplot 4: Tiempo total vs longitud
    ax4 = axes[1, 1]
    scatter = ax4.scatter(df_ordenado['longitud'], df_ordenado['total_ms'], 
                         s=100, alpha=0.6, c=df_ordenado['pct_llenado'], 
                         cmap='viridis', edgecolors='black', linewidth=1)
    ax4.set_xlabel('Longitud (caracteres)')
    ax4.set_ylabel('Tiempo Total (ms)')
    ax4.set_title('Tiempo Total vs Longitud\n(Color = % Llenado)')
    ax4.set_xscale('log')
    ax4.set_yscale('log')
    plt.colorbar(scatter, ax=ax4, label='% Llenado')
    ax4.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'analisis_comparativo_fases.png'), 
                dpi=300, bbox_inches='tight')
    plt.close()
    print(f"✓ Gráfico guardado: analisis_comparativo_fases.png")

def grafico_torta_por_archivo(df, archivo_seleccionado, output_dir):
    """Gráfico de torta para un archivo específico"""
    if archivo_seleccionado not in df['archivo'].values:
        print(f"Advertencia: {archivo_seleccionado} no encontrado. Usando el primero disponible.")
        archivo_seleccionado = df['archivo'].iloc[0]
    
    row = df[df['archivo'] == archivo_seleccionado].iloc[0]
    
    fig, ax = plt.subplots(figsize=(10, 8))
    
    sizes = [row['pct_init'], row['pct_llenado'], row['pct_traceback']]
    labels = [
        f"Inicialización\n{row['pct_init']:.2f}%",
        f"Llenado de Matriz\n{row['pct_llenado']:.2f}%",
        f"Traceback\n{row['pct_traceback']:.2f}%"
    ]
    colors = ['#FF6B6B', '#4ECDC4', '#45B7D1']
    explode = (0.05, 0.1, 0.05)  # Resaltar llenado
    
    wedges, texts, autotexts = ax.pie(sizes, explode=explode, labels=labels, colors=colors,
                                      autopct='%1.1f%%', shadow=True, startangle=90,
                                      textprops={'fontsize': 12, 'fontweight': 'bold'})
    
    ax.set_title(f'Distribución de Tiempo por Fase\n{row["archivo"]} ({row["longitud"]} caracteres)\n'
                 f'Tiempo Total: {row["total_ms"]:.2f} ms', 
                 fontsize=14, fontweight='bold', pad=20)
    
    plt.tight_layout()
    filename = f'torta_{row["archivo"].replace(".fasta", "")}.png'
    plt.savefig(os.path.join(output_dir, filename), dpi=300, bbox_inches='tight')
    plt.close()
    print(f"✓ Gráfico guardado: {filename}")

def generar_resumen_texto(df, output_dir):
    """Genera un resumen en texto con estadísticas importantes"""
    output_file = os.path.join(output_dir, 'resumen_analisis.txt')
    
    with open(output_file, 'w') as f:
        f.write("=" * 80 + "\n")
        f.write("RESUMEN DE ANÁLISIS DE BENCHMARK\n")
        f.write("=" * 80 + "\n\n")
        
        f.write(f"Total de archivos analizados: {len(df)}\n\n")
        
        f.write("ESTADÍSTICAS POR FASE:\n")
        f.write("-" * 80 + "\n")
        
        f.write(f"\nLlenado de Matriz (Fase más costosa):\n")
        f.write(f"  Promedio: {df['pct_llenado'].mean():.2f}% del tiempo total\n")
        f.write(f"  Mínimo:   {df['pct_llenado'].min():.2f}%\n")
        f.write(f"  Máximo:   {df['pct_llenado'].max():.2f}%\n")
        f.write(f"  Mediana:  {df['pct_llenado'].median():.2f}%\n")
        
        f.write(f"\nInicialización:\n")
        f.write(f"  Promedio: {df['pct_init'].mean():.2f}%\n")
        f.write(f"  Mínimo:   {df['pct_init'].min():.2f}%\n")
        f.write(f"  Máximo:   {df['pct_init'].max():.2f}%\n")
        
        f.write(f"\nTraceback:\n")
        f.write(f"  Promedio: {df['pct_traceback'].mean():.2f}%\n")
        f.write(f"  Mínimo:   {df['pct_traceback'].min():.2f}%\n")
        f.write(f"  Máximo:   {df['pct_traceback'].max():.2f}%\n")
        
        f.write("\n" + "=" * 80 + "\n")
        f.write("ARCHIVO CON MAYOR % EN LLENADO (cuello de botella):\n")
        idx_max = df['pct_llenado'].idxmax()
        row_max = df.iloc[idx_max]
        f.write(f"  Archivo: {row_max['archivo']}\n")
        f.write(f"  % Llenado: {row_max['pct_llenado']:.2f}%\n")
        f.write(f"  Tiempo llenado: {row_max['llenado_ms']:.2f} ms\n")
        f.write(f"  Tiempo total: {row_max['total_ms']:.2f} ms\n")
        
        f.write("\n" + "=" * 80 + "\n")
        f.write("PORCENTAJES DETALLADOS POR ARCHIVO:\n")
        f.write("-" * 80 + "\n")
        for _, row in df.sort_values('longitud').iterrows():
            f.write(f"\n{row['archivo']} ({row['longitud']} chars):\n")
            f.write(f"  Inicialización: {row['pct_init']:.2f}% ({row['init_ms']:.2f} ms)\n")
            f.write(f"  Llenado:        {row['pct_llenado']:.2f}% ({row['llenado_ms']:.2f} ms)\n")
            f.write(f"  Traceback:      {row['pct_traceback']:.2f}% ({row['traceback_ms']:.2f} ms)\n")
            f.write(f"  Total:          {row['total_ms']:.2f} ms\n")
    
    print(f"✓ Resumen de texto guardado: resumen_analisis.txt")

def main():
    parser = argparse.ArgumentParser(
        description='Analiza resultados de benchmark y genera gráficos de porcentajes por fase'
    )
    parser.add_argument('-d', '--directorio', 
                       default='resultados_benchmark',
                       help='Directorio con archivos CSV de resultados (default: resultados_benchmark)')
    parser.add_argument('-o', '--output', 
                       default='graficos_benchmark',
                       help='Directorio para guardar gráficos (default: graficos_benchmark)')
    parser.add_argument('-a', '--archivo-torta',
                       help='Archivo específico para gráfico de torta (opcional)')
    
    args = parser.parse_args()
    
    # Crear directorio de salida
    os.makedirs(args.output, exist_ok=True)
    
    print("=" * 80)
    print("ANÁLISIS DE BENCHMARK - Needleman-Wunsch")
    print("=" * 80)
    print()
    
    # Cargar datos
    print(f"Cargando datos desde: {args.directorio}")
    df = cargar_datos_promedios(args.directorio)
    
    if len(df) == 0:
        print("ERROR: No se encontraron archivos *_promedio.csv")
        return
    
    print(f"✓ Cargados {len(df)} archivos de resultados\n")
    
    # Calcular porcentajes
    df = calcular_porcentajes(df)
    
    # Mostrar resumen rápido
    print("RESUMEN RÁPIDO:")
    print(f"  Llenado de Matriz: {df['pct_llenado'].mean():.1f}% promedio del tiempo total")
    print(f"  Inicialización:    {df['pct_init'].mean():.1f}% promedio")
    print(f"  Traceback:         {df['pct_traceback'].mean():.1f}% promedio")
    print()
    
    # Generar gráficos
    print("Generando gráficos...")
    print()
    
    grafico_porcentajes_apilados(df, args.output)
    grafico_tiempos_absolutos(df, args.output)
    grafico_comparativo_fases(df, args.output)
    
    # Gráfico de torta
    if args.archivo_torta:
        grafico_torta_por_archivo(df, args.archivo_torta, args.output)
    else:
        # Generar torta para el archivo con mayor tiempo total
        archivo_mayor = df.loc[df['total_ms'].idxmax(), 'archivo']
        grafico_torta_por_archivo(df, archivo_mayor, args.output)
    
    # Generar resumen
    generar_resumen_texto(df, args.output)
    
    print()
    print("=" * 80)
    print(f"✓ Análisis completado. Gráficos guardados en: {args.output}/")
    print("=" * 80)

if __name__ == '__main__':
    main()
