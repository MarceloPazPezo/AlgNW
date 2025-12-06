#!/usr/bin/env python3
"""
Script completo para analizar los resultados del benchmark y generar gráficos
Muestra porcentajes de tiempo por fase y múltiples visualizaciones
Genera versiones IEEE (onecol/twocol) y PPT
"""

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import os
import argparse
import sys
from pathlib import Path
from matplotlib import transforms

# Intentamos usar Pillow para rotar imágenes cuando el usuario quiera versiones "giradas"
try:
    from PIL import Image
except Exception:
    Image = None

# Configurar estilo de gráficos
sns.set_style("whitegrid")
sns.set_palette("husl")

def extraer_longitud(archivo_fasta):
    """Extrae la longitud del nombre del archivo"""
    import re
    # Buscar patrones como dna_128, dna_1k, dna_16k
    match = re.search(r'(\d+)k', archivo_fasta)
    if match:
        return int(match.group(1)) * 1000
    match = re.search(r'_(\d+)\.fasta', archivo_fasta)
    if match:
        return int(match.group(1))
    return 0

def _format_spanish_number(value, decimals=2):
    """Formatea números con separador de miles '.' y separador decimal ',' (estilo español)"""
    try:
        if decimals == 0:
            s = f"{value:,.0f}"
        else:
            s = f"{value:,.{decimals}f}"
        s = s.replace(',', 'X').replace('.', ',').replace('X', '.')
        return s
    except Exception:
        return str(value)

def cargar_datos_csv(archivo_csv):
    """Carga datos desde el CSV promediado"""
    df = pd.read_csv(archivo_csv)
    
    # Filtrar solo secuenciales para análisis de fases
    df_secuencial = df[df['metodo'] == 'secuencial'].copy()
    
    if len(df_secuencial) == 0:
        print("Advertencia: No hay datos secuenciales")
        return None
    
    # Agregar columnas necesarias
    df_secuencial['longitud'] = df_secuencial['archivo_fasta'].apply(extraer_longitud)
    df_secuencial['archivo'] = df_secuencial['archivo_fasta'].apply(
        lambda x: os.path.basename(x).replace('.fasta', '')
    )
    df_secuencial['tipo'] = df_secuencial['archivo'].apply(
        lambda x: 'dna' if 'dna' in x.lower() else 'protein'
    )
    
    # Calcular porcentajes
    total = df_secuencial['tiempo_total_ms']
    df_secuencial['pct_init'] = (df_secuencial['tiempo_init_ms'] / total * 100)
    df_secuencial['pct_llenado'] = (df_secuencial['tiempo_llenado_ms'] / total * 100)
    df_secuencial['pct_traceback'] = (df_secuencial['tiempo_traceback_ms'] / total * 100)
    
    # Agregar alias para compatibilidad
    df_secuencial['total_ms'] = df_secuencial['tiempo_total_ms']
    df_secuencial['init_ms'] = df_secuencial['tiempo_init_ms']
    df_secuencial['llenado_ms'] = df_secuencial['tiempo_llenado_ms']
    df_secuencial['traceback_ms'] = df_secuencial['tiempo_traceback_ms']
    
    return df_secuencial.sort_values('longitud')

def ordenar_por_tipo_y_longitud(df):
    """Ordena el DataFrame colocando primero las filas de tipo 'dna' (de menor a mayor longitud)"""
    if df is None or len(df) == 0:
        return df
    tipo_order = {'dna': 0, 'protein': 1}
    df_copy = df.copy()
    df_copy['__tipo_ord'] = df_copy['tipo'].map(tipo_order).fillna(2)
    df_copy = df_copy.sort_values(['__tipo_ord', 'longitud'], ascending=[True, True])
    df_copy = df_copy.drop(columns=['__tipo_ord'])
    return df_copy

def grafico_porcentajes_apilados(df, output_dir, formato='ppt'):
    """Gráfico de barras apiladas mostrando porcentajes por fase"""
    df_ordenado = ordenar_por_tipo_y_longitud(df)
    
    # Configurar tamaño según formato
    if formato == 'ieee':
        fig, ax = plt.subplots(figsize=(3.5, 5.25))  # IEEE one-column vertical (4:6)
        fontsize = 10
    else:  # ppt
        fig, ax = plt.subplots(figsize=(16, 10))
        fontsize = 12
    
    archivos = [row['archivo'] for _, row in df_ordenado.iterrows()]
    
    if formato == 'ieee':
        # Barras horizontales apiladas para IEEE (vertical)
        y = np.arange(len(df_ordenado))
        width = 0.6
        
        p1 = ax.barh(y, df_ordenado['pct_init'], width, label='Fase 1', color='#FF6B6B')
        p2 = ax.barh(y, df_ordenado['pct_llenado'], width, left=df_ordenado['pct_init'], 
                    label='Fase 2', color='#4ECDC4')
        p3 = ax.barh(y, df_ordenado['pct_traceback'], width, 
                    left=df_ordenado['pct_init'] + df_ordenado['pct_llenado'],
                    label='Fase 3', color='#45B7D1')
        
        ax.set_yticks(y)
        ax.set_yticklabels([f"{row['archivo']}\n({row['longitud']} chars)" 
                            for _, row in df_ordenado.iterrows()], 
                           fontsize=fontsize-1)
        ax.invert_yaxis()  # Primero arriba
        ax.set_xlabel('Porcentaje del Tiempo Total (%)', fontsize=fontsize, fontweight='bold')
        ax.set_ylabel('Archivo (ordenado por longitud)', fontsize=fontsize, fontweight='bold')
        ax.set_title('Distribución de Tiempo por Fase del Alineamiento\n(Como porcentaje del tiempo total)', 
                     fontsize=fontsize+2, fontweight='bold', pad=20)
        ax.legend(loc='lower right', fontsize=fontsize-1)
        ax.set_xlim(0, 100)
        ax.grid(axis='x', alpha=0.3)
        
        ax.axvline(x=50, color='gray', linestyle='--', alpha=0.5, linewidth=1)
        ax.axvline(x=75, color='gray', linestyle='--', alpha=0.5, linewidth=1)
    else:
        # Barras verticales apiladas para PPT
        x = np.arange(len(df_ordenado))
        width = 0.8
        
        p1 = ax.bar(x, df_ordenado['pct_init'], width, label='Fase 1', color='#FF6B6B')
        p2 = ax.bar(x, df_ordenado['pct_llenado'], width, bottom=df_ordenado['pct_init'], 
                    label='Fase 2', color='#4ECDC4')
        p3 = ax.bar(x, df_ordenado['pct_traceback'], width, 
                    bottom=df_ordenado['pct_init'] + df_ordenado['pct_llenado'],
                    label='Fase 3', color='#45B7D1')
        
        ax.set_xlabel('Archivo (ordenado por longitud)', fontsize=fontsize, fontweight='bold')
        ax.set_ylabel('Porcentaje del Tiempo Total (%)', fontsize=fontsize, fontweight='bold')
        ax.set_title('Distribución de Tiempo por Fase del Alineamiento\n(Como porcentaje del tiempo total)', 
                     fontsize=fontsize+2, fontweight='bold', pad=20)
        ax.set_xticks(x)
        ax.set_xticklabels([f"{row['archivo']}\n({row['longitud']} chars)" 
                            for _, row in df_ordenado.iterrows()], 
                           rotation=45, ha='right', fontsize=fontsize-1)
        ax.legend(loc='upper left', fontsize=fontsize-1)
        ax.set_ylim(0, 100)
        ax.grid(axis='y', alpha=0.3)
        
        ax.axhline(y=50, color='gray', linestyle='--', alpha=0.5, linewidth=1)
        ax.axhline(y=75, color='gray', linestyle='--', alpha=0.5, linewidth=1)
    
    plt.tight_layout()
    filename = f'porcentajes_por_fase_apilado_{formato}.png'
    plt.savefig(os.path.join(output_dir, filename), dpi=300, bbox_inches='tight')
    
    # Versión IEEE two-column vertical (3:6)
    if formato == 'ieee':
        fig.set_size_inches(7.16, 14.32)  # IEEE two-column vertical (3:6)
        plt.savefig(os.path.join(output_dir, 'porcentajes_por_fase_apilado_ieee_twocol.png'), 
                   dpi=300, bbox_inches='tight')
    
    plt.close()
    print(f"[OK] Gráfico guardado: {filename}")

def grafico_tiempos_absolutos(df, output_dir, formato='ppt'):
    """Gráfico de líneas mostrando tiempos absolutos por fase vs longitud"""
    df_ordenado = ordenar_por_tipo_y_longitud(df)
    
    if formato == 'ieee':
        fig, ax = plt.subplots(figsize=(3.5, 5.25))  # IEEE one-column vertical (4:6)
        fontsize = 10
        markersize = 5
        linewidth = 1.5
    else:  # ppt
        fig, ax = plt.subplots(figsize=(14, 8))
        fontsize = 12
        markersize = 8
        linewidth = 2
    
    ax.plot(df_ordenado['longitud'], df_ordenado['tiempo_init_ms'], 
            marker='o', label='Fase 1', linewidth=linewidth, 
            markersize=markersize, color='#FF6B6B')
    ax.plot(df_ordenado['longitud'], df_ordenado['tiempo_llenado_ms'], 
            marker='s', label='Fase 2', linewidth=linewidth, 
            markersize=markersize, color='#4ECDC4')
    ax.plot(df_ordenado['longitud'], df_ordenado['tiempo_traceback_ms'], 
            marker='^', label='Fase 3', linewidth=linewidth, 
            markersize=markersize, color='#45B7D1')
    ax.plot(df_ordenado['longitud'], df_ordenado['tiempo_total_ms'], 
            marker='D', label='Total', linewidth=linewidth+1, 
            markersize=markersize+2, color='#2C3E50', linestyle='--')
    
    ax.set_xlabel('Longitud de Secuencias (caracteres)', fontsize=fontsize, fontweight='bold')
    ax.set_ylabel('Tiempo (ms)', fontsize=fontsize, fontweight='bold')
    ax.set_title('Tiempos Absolutos por Fase vs Longitud de Secuencias', 
                 fontsize=fontsize+2, fontweight='bold', pad=20)
    ax.legend(loc='upper left', fontsize=fontsize-1)
    ax.grid(True, alpha=0.3)
    ax.set_xscale('log')
    ax.set_yscale('log')
    
    plt.tight_layout()
    filename = f'tiempos_absolutos_por_fase_{formato}.png'
    plt.savefig(os.path.join(output_dir, filename), dpi=300, bbox_inches='tight')
    
    if formato == 'ieee':
        fig.set_size_inches(7.16, 14.32)  # IEEE two-column vertical (3:6)
        plt.savefig(os.path.join(output_dir, 'tiempos_absolutos_por_fase_ieee_twocol.png'), 
                   dpi=300, bbox_inches='tight')
    
    plt.close()
    print(f"[OK] Gráfico guardado: {filename}")

def grafico_comparativo_fases(df, output_dir, formato='ppt'):
    """Gráfico comparativo mostrando cómo crece cada fase con el tamaño"""
    df_ordenado = ordenar_por_tipo_y_longitud(df)
    
    if formato == 'ieee':
        fig, axes = plt.subplots(2, 2, figsize=(7.16, 14.32))  # IEEE two-column vertical (3:6)
        fontsize = 9
    else:  # ppt
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        fontsize = 10
    
    fig.suptitle('Análisis Detallado de Tiempos por Fase', 
                 fontsize=fontsize+4, fontweight='bold', y=0.995)
    
    # Subplot 1: Tiempos absolutos
    ax1 = axes[0, 0]
    ax1.plot(df_ordenado['longitud'], df_ordenado['tiempo_init_ms'], 
             marker='o', label='Fase 1', color='#FF6B6B', linewidth=1.5, markersize=4)
    ax1.plot(df_ordenado['longitud'], df_ordenado['tiempo_llenado_ms'], 
             marker='s', label='Fase 2', color='#4ECDC4', linewidth=1.5, markersize=4)
    ax1.plot(df_ordenado['longitud'], df_ordenado['tiempo_traceback_ms'], 
             marker='^', label='Fase 3', color='#45B7D1', linewidth=1.5, markersize=4)
    ax1.set_xlabel('Longitud', fontsize=fontsize)
    ax1.set_ylabel('Tiempo (ms)', fontsize=fontsize)
    ax1.set_title('Tiempos Absolutos', fontsize=fontsize+1)
    ax1.legend(fontsize=fontsize-1)
    ax1.grid(True, alpha=0.3)
    ax1.set_xscale('log')
    ax1.set_yscale('log')
    
    # Subplot 2: Porcentajes
    ax2 = axes[0, 1]
    x = np.arange(len(df_ordenado))
    width = 0.6
    ax2.bar(x - width/3, df_ordenado['pct_init'], width/3, label='Fase 1 %', color='#FF6B6B')
    ax2.bar(x, df_ordenado['pct_llenado'], width/3, label='Fase 2 %', color='#4ECDC4')
    ax2.bar(x + width/3, df_ordenado['pct_traceback'], width/3, label='Fase 3 %', color='#45B7D1')
    ax2.set_xlabel('Archivo', fontsize=fontsize)
    ax2.set_ylabel('Porcentaje (%)', fontsize=fontsize)
    ax2.set_title('Porcentajes por Fase', fontsize=fontsize+1)
    ax2.set_xticks(x)
    ax2.set_xticklabels([row['archivo'] for _, row in df_ordenado.iterrows()], 
                        rotation=45, ha='right', fontsize=fontsize-2)
    ax2.legend(fontsize=fontsize-1)
    ax2.grid(True, alpha=0.3, axis='y')
    
    # Subplot 3: Porcentaje de llenado
    ax3 = axes[1, 0]
    ax3.bar(range(len(df_ordenado)), df_ordenado['pct_llenado'], color='#4ECDC4')
    ax3.set_xlabel('Archivo', fontsize=fontsize)
    ax3.set_ylabel('Porcentaje (%)', fontsize=fontsize)
    ax3.set_title('Porcentaje de Tiempo en Fase 2', fontsize=fontsize+1)
    ax3.set_xticks(range(len(df_ordenado)))
    ax3.set_xticklabels([row['archivo'] for _, row in df_ordenado.iterrows()], 
                        rotation=45, ha='right', fontsize=fontsize-2)
    mean_val = df_ordenado['pct_llenado'].mean()
    ax3.axhline(y=mean_val, color='red', linestyle='--', 
                label=f'Promedio: {mean_val:.1f}%', linewidth=1)
    ax3.legend(fontsize=fontsize-1)
    ax3.grid(True, alpha=0.3, axis='y')
    
    # Subplot 4: Tiempo total vs longitud
    ax4 = axes[1, 1]
    scatter = ax4.scatter(df_ordenado['longitud'], df_ordenado['tiempo_total_ms'], 
                         s=100 if formato == 'ppt' else 50, alpha=0.6, 
                         c=df_ordenado['pct_llenado'], cmap='viridis', 
                         edgecolors='black', linewidth=1)
    ax4.set_xlabel('Longitud (caracteres)', fontsize=fontsize)
    ax4.set_ylabel('Tiempo Total (ms)', fontsize=fontsize)
    ax4.set_title('Tiempo Total vs Longitud\n(Color = % Fase 2)', fontsize=fontsize+1)
    ax4.set_xscale('log')
    ax4.set_yscale('log')
    plt.colorbar(scatter, ax=ax4, label='% Fase 2')
    ax4.grid(True, alpha=0.3)
    
    plt.tight_layout()
    filename = f'analisis_comparativo_fases_{formato}.png'
    plt.savefig(os.path.join(output_dir, filename), dpi=300, bbox_inches='tight')
    
    if formato == 'ieee':
        fig.set_size_inches(3.5, 5.25)  # IEEE one-column vertical (4:6)
        plt.savefig(os.path.join(output_dir, 'analisis_comparativo_fases_ieee_onecol.png'), 
                   dpi=300, bbox_inches='tight')
    
    plt.close()
    print(f"[OK] Gráfico guardado: {filename}")

def grafico_tiempo_total_por_longitud(df, output_dir, formato='ppt'):
    """Grafica el tiempo total (ms) en función de la longitud de entrada"""
    df_ordenado = ordenar_por_tipo_y_longitud(df)
    
    if formato == 'ieee':
        fig, ax = plt.subplots(figsize=(3.5, 5.25))  # IEEE one-column vertical (4:6)
        fontsize = 10
        markersize = 5
        linewidth = 1.5
    else:  # ppt
        fig, ax = plt.subplots(figsize=(14, 8))
        fontsize = 12
        markersize = 6
        linewidth = 2
    
    for tipo, marker, color in [('dna', 'o', '#2A9D8F'), ('protein', 's', '#E76F51')]:
        subset = df_ordenado[df_ordenado['tipo'] == tipo]
        if len(subset) == 0:
            continue
        y_vals = subset['tiempo_total_ms'] / 1000.0  # Convertir a segundos
        ax.plot(subset['longitud'], y_vals, marker=marker, linestyle='-',
                linewidth=linewidth, markersize=markersize, label=tipo.capitalize(), color=color)
    
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('Longitud de Secuencias (caracteres)', fontsize=fontsize, fontweight='bold')
    ax.set_ylabel('Tiempo Total (s)', fontsize=fontsize, fontweight='bold')
    ax.set_title('Tiempo Total vs Longitud de Entrada', fontsize=fontsize+2, fontweight='bold')
    ax.grid(True, which='both', alpha=0.3)
    ax.legend(fontsize=fontsize-1)
    
    plt.tight_layout()
    filename = f'tiempo_total_vs_longitud_{formato}.png'
    plt.savefig(os.path.join(output_dir, filename), dpi=300, bbox_inches='tight')
    
    if formato == 'ieee':
        fig.set_size_inches(7.16, 14.32)  # IEEE two-column vertical (3:6)
        plt.savefig(os.path.join(output_dir, 'tiempo_total_vs_longitud_ieee_twocol.png'), 
                   dpi=300, bbox_inches='tight')
    
    plt.close()
    print(f"[OK] Gráfico guardado: {filename}")

def grafico_porcentajes_separados(df, output_dir, formato='ppt'):
    """Genera PNGs separados para los porcentajes por fase"""
    df_ordenado = ordenar_por_tipo_y_longitud(df)
    archivos = [row['archivo'] for _, row in df_ordenado.iterrows()]
    
    if formato == 'ieee':
        fig, ax = plt.subplots(figsize=(3.5, 5.25))  # IEEE one-column vertical (4:6)
        fontsize = 9
    else:  # ppt
        fig, ax = plt.subplots(figsize=(16, 8))
        fontsize = 10
    
    if formato == 'ieee':
        # Barras horizontales agrupadas para IEEE (vertical)
        y = np.arange(len(df_ordenado))
        height = 0.25
        
        ax.barh(y - height, df_ordenado['pct_init'], height, label='Fase 1', color='#FF6B6B')
        ax.barh(y, df_ordenado['pct_llenado'], height, label='Fase 2', color='#4ECDC4')
        ax.barh(y + height, df_ordenado['pct_traceback'], height, label='Fase 3', color='#45B7D1')
        
        ax.set_yticks(y)
        ax.set_yticklabels(archivos, fontsize=fontsize-1)
        ax.invert_yaxis()  # Primero arriba
        ax.set_xlabel('Porcentaje (%)', fontsize=fontsize)
        ax.set_ylabel('Archivo (ordenado por longitud)', fontsize=fontsize)
        ax.set_title('Porcentajes por Fase (agrupado)', fontsize=fontsize+2)
        ax.legend(fontsize=fontsize-1)
        ax.grid(True, alpha=0.3, axis='x')
    else:
        # Barras verticales agrupadas para PPT
        x = np.arange(len(df_ordenado))
        width = 0.25
        
        ax.bar(x - width, df_ordenado['pct_init'], width, label='Fase 1', color='#FF6B6B')
        ax.bar(x, df_ordenado['pct_llenado'], width, label='Fase 2', color='#4ECDC4')
        ax.bar(x + width, df_ordenado['pct_traceback'], width, label='Fase 3', color='#45B7D1')
        ax.set_xlabel('Archivo (ordenado por longitud)', fontsize=fontsize)
        ax.set_ylabel('Porcentaje (%)', fontsize=fontsize)
        ax.set_title('Porcentajes por Fase (agrupado)', fontsize=fontsize+2)
        ax.set_xticks(x)
        ax.set_xticklabels(archivos, rotation=45, ha='right', fontsize=fontsize-1)
        ax.legend(fontsize=fontsize-1)
        ax.grid(True, alpha=0.3, axis='y')
    
    plt.tight_layout()
    filename = f'porcentajes_por_fase_separado_{formato}.png'
    plt.savefig(os.path.join(output_dir, filename), dpi=300, bbox_inches='tight')
    
    if formato == 'ieee':
        fig.set_size_inches(7.16, 14.32)  # IEEE two-column vertical (3:6)
        plt.savefig(os.path.join(output_dir, 'porcentajes_por_fase_separado_ieee_twocol.png'), 
                   dpi=300, bbox_inches='tight')
    
    plt.close()
    print(f"[OK] Gráfico guardado: {filename}")

def grafico_llenado_detalle(df, output_dir, formato='ppt'):
    """Genera imágenes enfocadas en el porcentaje de tiempo en llenado de matriz"""
    df_ordenado = ordenar_por_tipo_y_longitud(df)
    
    archivos = [row['archivo'] for _, row in df_ordenado.iterrows()]
    mean_val = df_ordenado['pct_llenado'].mean()
    
    if formato == 'ieee':
        # Solo generar versión twocol para IEEE, no la one-column
        # Versión two-column con formato 3:6 y barras agrupadas (similar a porcentajes separados)
        # pero destacando la Fase 2
        fig_twocol, ax_twocol = plt.subplots(figsize=(7.16, 11.0))  # IEEE two-column vertical (reducido)
        fontsize_twocol = 18  # Fuente más grande para mejor legibilidad
        
        # Reducir espaciado entre barras: usar factor de compresión
        espaciado_factor = 0.7  # Factor para reducir espacio entre barras
        y = np.arange(len(df_ordenado)) * espaciado_factor
        height = 0.15  # Altura de las barras agrupadas (reducida proporcionalmente)
        
        # Usar los mismos colores del gráfico tiempo_vs_longitud
        # Barras horizontales agrupadas - Fase 2 más ancha para destacarla
        bars1 = ax_twocol.barh(y - height*1.1, df_ordenado['pct_init'], height, 
                               label='Fase 1', color='#E74C3C', edgecolor='white', linewidth=0.5)  # Rojo
        bars2 = ax_twocol.barh(y, df_ordenado['pct_llenado'], height*1.4,  # Fase 2 más ancha
                               label='Fase 2', color='#3498DB', edgecolor='white', linewidth=1)  # Azul
        bars3 = ax_twocol.barh(y + height*1.1, df_ordenado['pct_traceback'], height, 
                               label='Fase 3', color='#2C3E50', edgecolor='white', linewidth=0.5)  # Negro
        
        ax_twocol.set_yticks(y)
        ax_twocol.set_yticklabels(archivos, fontsize=fontsize_twocol-1)  # Fuente más grande
        ax_twocol.invert_yaxis()  # Primero arriba
        
        # Poner valores solo de la Fase 2 (la más importante) al final de la barra
        # Usar y[i] en lugar de i para que se alinee con las barras comprimidas
        for i, val2 in enumerate(df_ordenado['pct_llenado']):
            ax_twocol.text(val2 + 2, y[i], f'{val2:.1f}%',
                         fontsize=fontsize_twocol-2, fontweight='bold',  # Fuente más grande
                         color='black', va='center', ha='left',
                         bbox=dict(boxstyle='round,pad=0.3', facecolor='white', 
                                 edgecolor='black', alpha=0.9, linewidth=0.8))
        
        # Línea de promedio solo para Fase 2 (sin label para no incluirla en la leyenda)
        mean_val = df_ordenado['pct_llenado'].mean()
        ax_twocol.axvline(x=mean_val, color='red', linestyle='--', linewidth=2.5)
        
        # Agregar texto sobre la línea roja con el promedio en la parte superior (horizontal)
        # Ajustar posición Y considerando el factor de compresión
        y_promedio = -0.7 * espaciado_factor
        ax_twocol.text(mean_val, y_promedio, f'Promedio Fase 2: {mean_val:.2f}%',
                      fontsize=fontsize_twocol-2, fontweight='bold', color='red',
                      ha='center', va='top', rotation=0,
                      bbox=dict(boxstyle='round,pad=0.3', facecolor='white', edgecolor='red', alpha=0.8))
        
        ax_twocol.set_xlabel('Porcentaje del tiempo total (%)', fontsize=fontsize_twocol+1, fontweight='bold')
        ax_twocol.set_ylabel('Longitud', fontsize=fontsize_twocol+1, fontweight='bold')
        ax_twocol.set_xlim(0, 100)
        
        # Colocar leyenda debajo del gráfico
        ax_twocol.legend(fontsize=fontsize_twocol-2, loc='upper center', 
                        bbox_to_anchor=(0.4, -0.075), framealpha=0.95, 
                        edgecolor='black', frameon=True, ncol=4, 
                        columnspacing=0.6, handlelength=1.0, handletextpad=0.4)
        ax_twocol.grid(True, alpha=0.3, axis='x', linewidth=1.5)
        
        # Aumentar tamaño de ticks
        ax_twocol.tick_params(axis='x', labelsize=fontsize_twocol-1, width=1.5, length=6)
        ax_twocol.tick_params(axis='y', labelsize=fontsize_twocol-1, width=1.5, length=6)
        
        # Aumentar grosor de los ejes
        for spine in ax_twocol.spines.values():
            spine.set_linewidth(1.5)
        
        # Ajustar subplots para que el gráfico ocupe más espacio (menos padding)
        # Dejar espacio abajo para la leyenda
        plt.tight_layout(pad=1.0, rect=[0, 0.12, 1, 0.98])
        plt.savefig(os.path.join(output_dir, 'porcentaje_llenado_por_archivo_promedio_ieee_twocol.png'), 
                   dpi=300, bbox_inches='tight')
        plt.close(fig_twocol)
        print(f"[OK] Gráfico guardado: porcentaje_llenado_por_archivo_promedio_ieee_twocol.png")
    else:  # ppt
        # Para PPT, generar gráfico normal
        fig, ax = plt.subplots(figsize=(14, 6))
        fontsize = 10
        
        # Barras verticales para PPT
        ax.bar(archivos, df_ordenado['pct_llenado'], color='#4ECDC4')
        ax.set_xlabel('Archivo (ordenado por longitud)', fontsize=fontsize)
        ax.set_ylabel('% Fase 2', fontsize=fontsize)
        ax.set_title('Porcentaje de Tiempo en Fase 2 por Archivo', fontsize=fontsize+2)
        ax.set_xticks(range(len(archivos)))
        ax.set_xticklabels(archivos, rotation=45, ha='right', fontsize=fontsize-1)
        ax.axhline(y=mean_val, color='red', linestyle='--', linewidth=1.5,
                   label=f'Promedio: {mean_val:.2f}%')
        ax.legend(fontsize=fontsize-1)
        ax.grid(True, alpha=0.3, axis='y')
        
        plt.tight_layout()
        filename = f'porcentaje_llenado_por_archivo_promedio_{formato}.png'
        plt.savefig(os.path.join(output_dir, filename), dpi=300, bbox_inches='tight')
        plt.close()
        print(f"[OK] Gráfico guardado: {filename}")

def grafico_torta_por_archivo(df, archivo_seleccionado, output_dir, formato='ppt'):
    """Gráfico de torta para un archivo específico"""
    if archivo_seleccionado not in df['archivo'].values:
        print(f"Advertencia: {archivo_seleccionado} no encontrado. Usando el primero disponible.")
        archivo_seleccionado = df['archivo'].iloc[0]
    
    row = df[df['archivo'] == archivo_seleccionado].iloc[0]
    
    if formato == 'ieee':
        fig, ax = plt.subplots(figsize=(3.5, 5.25))  # IEEE one-column vertical (4:6)
        fontsize = 10
    else:  # ppt
        fig, ax = plt.subplots(figsize=(10, 8))
        fontsize = 12
    
    sizes = [row['pct_init'], row['pct_llenado'], row['pct_traceback']]
    labels = [
        f"Fase 1\n{row['pct_init']:.2f}%",
        f"Fase 2\n{row['pct_llenado']:.2f}%",
        f"Fase 3\n{row['pct_traceback']:.2f}%"
    ]
    colors = ['#FF6B6B', '#4ECDC4', '#45B7D1']
    explode = (0.05, 0.1, 0.05)
    
    wedges, texts, autotexts = ax.pie(sizes, explode=explode, labels=labels, colors=colors,
                                      autopct='%1.1f%%', shadow=True, startangle=90,
                                      textprops={'fontsize': fontsize, 'fontweight': 'bold'})
    
    ax.set_title(f'Distribución de Tiempo por Fase\n{row["archivo"]} ({row["longitud"]} caracteres)\n'
                 f'Tiempo Total: {row["total_ms"]:.2f} ms', 
                 fontsize=fontsize+2, fontweight='bold', pad=20)
    
    plt.tight_layout()
    filename = f'torta_{row["archivo"].replace(".fasta", "")}_{formato}.png'
    plt.savefig(os.path.join(output_dir, filename), dpi=300, bbox_inches='tight')
    plt.close()
    print(f"[OK] Gráfico guardado: {filename}")

def cargar_datos_completos(archivo_csv):
    """Carga todos los datos (secuenciales y paralelos) desde el CSV"""
    df = pd.read_csv(archivo_csv)
    
    # Agregar columnas necesarias
    df['longitud'] = df['archivo_fasta'].apply(extraer_longitud)
    df['archivo'] = df['archivo_fasta'].apply(
        lambda x: os.path.basename(x).replace('.fasta', '')
    )
    
    return df

def calcular_mejor_peor_schedule(df_completo):
    """Calcula el mejor y peor schedule para cada método basándose en el promedio general
    (todas las longitudes, todos los threads) para tener consistencia entre gráficos
    
    Returns:
        dict: {'antidiagonal': {'mejor': 'schedule', 'peor': 'schedule'}, 
               'bloques': {'mejor': 'schedule', 'peor': 'schedule'}}
    """
    # Filtrar solo datos paralelos
    df_paralelo = df_completo[df_completo['metodo'] != 'secuencial'].copy()
    
    if len(df_paralelo) == 0:
        return {'antidiagonal': {'mejor': 'static', 'peor': 'dynamic'}, 
                'bloques': {'mejor': 'dynamic', 'peor': 'static,1'}}
    
    # Obtener datos secuenciales para calcular speedup
    df_secuencial = df_completo[df_completo['metodo'] == 'secuencial'].copy()
    df_secuencial_merge = df_secuencial[['archivo_fasta', 'tiempo_total_ms']].rename(
        columns={'tiempo_total_ms': 'tiempo_secuencial_ms'}
    )
    
    # Calcular speedup
    df_speedup = df_paralelo.merge(df_secuencial_merge, on='archivo_fasta', how='left')
    df_speedup['speedup'] = df_speedup['tiempo_secuencial_ms'] / df_speedup['tiempo_total_ms']
    
    resultado = {}
    
    for metodo in ['antidiagonal', 'bloques']:
        df_metodo = df_speedup[df_speedup['metodo'] == metodo].copy()
        
        if len(df_metodo) == 0:
            # Valores por defecto si no hay datos
            if metodo == 'antidiagonal':
                resultado[metodo] = {'mejor': 'static', 'peor': 'dynamic'}
            else:
                resultado[metodo] = {'mejor': 'dynamic', 'peor': 'static,1'}
            continue
        
        # Promediar speedup por schedule (sobre todas las longitudes y threads)
        df_schedule_avg = df_metodo.groupby('schedule', as_index=False)['speedup'].mean()
        
        # Mejor schedule: el que tiene mayor speedup promedio
        schedule_best = df_schedule_avg.loc[df_schedule_avg['speedup'].idxmax(), 'schedule']
        # Peor schedule: el que tiene menor speedup promedio
        schedule_worst = df_schedule_avg.loc[df_schedule_avg['speedup'].idxmin(), 'schedule']
        
        resultado[metodo] = {'mejor': schedule_best, 'peor': schedule_worst}
    
    # Mostrar qué schedules se están usando para verificación
    print(f"  Schedules mejor/peor calculados:")
    for metodo, schedules in resultado.items():
        print(f"    {metodo}: mejor={schedules['mejor']}, peor={schedules['peor']}")
    
    return resultado

def calcular_speedup(df_completo, usar_mejor_schedule=True):
    """Calcula el speedup comparando métodos paralelos con el secuencial
    
    Args:
        df_completo: DataFrame con todos los datos
        usar_mejor_schedule: Si True, selecciona el mejor schedule para cada configuración
    """
    # Obtener datos secuenciales
    df_secuencial = df_completo[df_completo['metodo'] == 'secuencial'].copy()
    
    if len(df_secuencial) == 0:
        print("Advertencia: No hay datos secuenciales")
        return None
    
    # Obtener datos paralelos
    df_paralelo = df_completo[df_completo['metodo'] != 'secuencial'].copy()
    
    if len(df_paralelo) == 0:
        print("Advertencia: No hay datos paralelos")
        return None
    
    # Hacer merge para tener tiempos secuenciales
    df_secuencial_merge = df_secuencial[['archivo_fasta', 'tiempo_total_ms']].rename(
        columns={'tiempo_total_ms': 'tiempo_secuencial_ms'}
    )
    
    df_speedup = df_paralelo.merge(df_secuencial_merge, on='archivo_fasta', how='left')
    
    # Calcular speedup
    df_speedup['speedup'] = df_speedup['tiempo_secuencial_ms'] / df_speedup['tiempo_total_ms']
    
    # Si se solicita, seleccionar el mejor schedule para cada configuración
    if usar_mejor_schedule:
        # Para cada combinación de método, threads, archivo_fasta, seleccionar el schedule con mayor speedup
        # Esto asegura que usamos el mejor schedule para cada método específico
        df_speedup = df_speedup.loc[
            df_speedup.groupby(['metodo', 'threads', 'archivo_fasta'])['speedup'].idxmax()
        ].copy()
        
        # Mostrar qué schedules se están usando
        print(f"  Schedules seleccionados por método y hilos:")
        for metodo in df_speedup['metodo'].unique():
            for threads in sorted(df_speedup['threads'].unique()):
                subset = df_speedup[(df_speedup['metodo'] == metodo) & (df_speedup['threads'] == threads)]
                if len(subset) > 0:
                    schedules_used = subset['schedule'].value_counts()
                    print(f"    {metodo} ({threads} hilos): {dict(schedules_used)}")
    
    return df_speedup

def calcular_ley_amdahl(p, n):
    """
    Calcula el speedup teórico según la ley de Amdahl
    p: fracción paralelizable (0 a 1)
    n: número de threads/processors
    """
    if n == 0:
        return 1.0
    return 1.0 / ((1.0 - p) + (p / n))

def grafico_speedup_vs_threads(df_completo, df_secuencial, output_dir, formato='ieee'):
    """Genera gráfico de speedup vs número de threads con ley de Amdahl"""
    # Calcular mejor/peor schedule de forma consistente
    mejor_peor_schedules = calcular_mejor_peor_schedule(df_completo)
    
    # Calcular speedup SIN filtrar por mejor schedule, para poder calcular mejor/peor después
    df_speedup = calcular_speedup(df_completo, usar_mejor_schedule=False)
    
    if df_speedup is None or len(df_speedup) == 0:
        print("Advertencia: No se pudo calcular speedup")
        return
    
    # Calcular porcentaje de Fase 2 (paralelizable) promedio general desde datos secuenciales
    pct_fase2_general = df_secuencial['pct_llenado'].mean() / 100.0  # Convertir a fracción (0-1)
    
    # Definir solo el tamaño 16k
    tamanos_config = [
        {'nombre': '16k', 'min': 16000, 'max': 17000}
    ]
    
    # Filtrar y agrupar datos por tamaño, calculando también el pct paralelizable específico
    datos_por_tamano_metodo = {}  # Estructura: {tamano: {metodo: {'mejor': df, 'peor': df}}}
    pct_por_tamano = {}
    
    for tamano in tamanos_config:
        # Filtrar datos de speedup para este tamaño
        df_tamano = df_speedup[
            (df_speedup['longitud_A'] >= tamano['min']) & 
            (df_speedup['longitud_A'] <= tamano['max'])
        ].copy()
        
        if len(df_tamano) > 0:
            # Agrupar por método, threads y schedule para obtener speedup promedio
            # Si hay múltiples archivos del mismo tamaño, promediar entre ellos también
            df_metodo_threads_schedule = df_tamano.groupby(['metodo', 'threads', 'schedule'], as_index=False)['speedup'].mean()
            
            # Para cada método, obtener mejor y peor speedup por threads
            datos_por_tamano_metodo[tamano['nombre']] = {}
            
            for metodo in ['antidiagonal', 'bloques']:
                df_metodo = df_metodo_threads_schedule[df_metodo_threads_schedule['metodo'] == metodo].copy()
                
                if len(df_metodo) > 0:
                    # Para el gráfico de speedup, calcular mejor/peor por threads individual
                    # ya que cada número de threads puede tener un mejor/peor diferente
                    df_mejor_list = []
                    df_peor_list = []
                    
                    for threads in sorted(df_metodo['threads'].unique()):
                        df_threads = df_metodo[df_metodo['threads'] == threads].copy()
                        
                        # Mejor schedule para este número de threads
                        schedule_best_threads = df_threads.loc[df_threads['speedup'].idxmax(), 'schedule']
                        # Peor schedule para este número de threads
                        schedule_worst_threads = df_threads.loc[df_threads['speedup'].idxmin(), 'schedule']
                        
                        # Agregar a las listas
                        mejor_row = df_threads[df_threads['schedule'] == schedule_best_threads].iloc[0]
                        peor_row = df_threads[df_threads['schedule'] == schedule_worst_threads].iloc[0]
                        
                        df_mejor_list.append({
                            'threads': threads,
                            'speedup': mejor_row['speedup'],
                            'schedule': schedule_best_threads
                        })
                        df_peor_list.append({
                            'threads': threads,
                            'speedup': peor_row['speedup'],
                            'schedule': schedule_worst_threads
                        })
                    
                    df_mejor = pd.DataFrame(df_mejor_list).sort_values('threads')
                    df_peor = pd.DataFrame(df_peor_list).sort_values('threads')
                    
                    datos_por_tamano_metodo[tamano['nombre']][metodo] = {
                        'mejor': df_mejor[['threads', 'speedup', 'schedule']].copy(),
                        'peor': df_peor[['threads', 'speedup', 'schedule']].copy()
                    }
            
            # Calcular porcentaje paralelizable específico para este tamaño desde datos secuenciales
            # NOTA IMPORTANTE: El speedup medido puede superar la ley de Amdahl debido a un problema de medición:
            # - El código de Fase 1 es idéntico en ambas versiones (no se paraleliza)
            # - Aunque cada ejecución paralela intenta limpiar la caché antes de ejecutar, hay inconsistencia:
            #   * Rep 1 del paralelo: init ~285ms (similar al secuencial sin limpieza ~284ms)
            #   * Reps 2+ del paralelo: init ~53-63ms (5x más rápidas, sugiriendo que la limpieza no funciona)
            # - El secuencial se ejecuta una sola vez, mientras que el paralelo se promedia sobre múltiples
            #   ejecuciones (1ra ~285ms, resto ~53-63ms → promedio ~78ms)
            # - Esto infla artificialmente el speedup medido (2.88x) vs el teórico de Amdahl (2.53x)
            # - El problema: no se están comparando condiciones equivalentes (secuencial 1 ejecución vs paralelo promediado)
            df_sec_tamano = df_secuencial[
                (df_secuencial['longitud'] >= tamano['min']) & 
                (df_secuencial['longitud'] <= tamano['max'])
            ]
            
            if len(df_sec_tamano) > 0:
                pct_fase2_tamano = df_sec_tamano['pct_llenado'].mean() / 100.0
            else:
                # Si no hay datos secuenciales para este tamaño, usar el promedio general
                pct_fase2_tamano = pct_fase2_general
            
            pct_por_tamano[tamano['nombre']] = pct_fase2_tamano
    
    if formato == 'ieee':
        # IEEE two-column con altura aumentada para mejor visualización
        fig, ax = plt.subplots(figsize=(7.16, 9.5))
        fontsize = 18
        markersize = 10
        linewidth = 2.5
    else:  # ppt
        fig, ax = plt.subplots(figsize=(14, 8))
        fontsize = 12
        markersize = 8
        linewidth = 2
    
    # Colores para cada estrategia
    colores_antidiagonal = {'mejor': '#2A9D8F', 'peor': '#95A5A6'}  # Verde para mejor, gris para peor
    colores_bloques = {'mejor': '#3498DB', 'peor': '#BDC3C7'}  # Azul para mejor, gris claro para peor
    # Marcadores estandarizados: antidiagonal=triángulo, bloques=cuadrado
    marcadores_antidiagonal = {'mejor': '^', 'peor': '^'}  # Triángulo para antidiagonal
    marcadores_bloques = {'mejor': 's', 'peor': 's'}  # Cuadrado para bloques
    estilos = {'mejor': '-', 'peor': '--'}
    
    threads_max = 0
    
    # Graficar speedup para cada tamaño y estrategia
    for nombre_tamano in datos_por_tamano_metodo.keys():
        datos_metodos = datos_por_tamano_metodo[nombre_tamano]
        
        # Graficar antidiagonal (mejor y peor)
        if 'antidiagonal' in datos_metodos:
            datos_ant = datos_metodos['antidiagonal']
            if len(datos_ant['mejor']) > 0:
                threads_max = max(threads_max, int(datos_ant['mejor']['threads'].max()))
                # Mejor speedup - usar el schedule más frecuente
                schedule_mejor = datos_ant['mejor']['schedule'].mode()[0] if len(datos_ant['mejor']['schedule'].mode()) > 0 else datos_ant['mejor']['schedule'].iloc[0]
                ax.plot(datos_ant['mejor']['threads'], datos_ant['mejor']['speedup'], 
                        marker=marcadores_antidiagonal['mejor'], linewidth=linewidth, markersize=markersize,
                        linestyle=estilos['mejor'], color=colores_antidiagonal['mejor'],
                        label=f'Antidiagonal mejor ({schedule_mejor})', zorder=4)
                
            if len(datos_ant['peor']) > 0:
                schedule_peor = datos_ant['peor']['schedule'].mode()[0] if len(datos_ant['peor']['schedule'].mode()) > 0 else datos_ant['peor']['schedule'].iloc[0]
                ax.plot(datos_ant['peor']['threads'], datos_ant['peor']['speedup'], 
                        marker=marcadores_antidiagonal['peor'], linewidth=linewidth*0.8, markersize=markersize*0.8,
                        linestyle=estilos['peor'], color=colores_antidiagonal['peor'], alpha=0.7,
                        label=f'Antidiagonal peor ({schedule_peor})', zorder=2)
        
        # Graficar bloques (mejor y peor)
        if 'bloques' in datos_metodos:
            datos_bloq = datos_metodos['bloques']
            if len(datos_bloq['mejor']) > 0:
                threads_max = max(threads_max, int(datos_bloq['mejor']['threads'].max()))
                # Mejor speedup - preferir mostrar el schedule de 4 threads si existe, sino el más frecuente
                if 4 in datos_bloq['mejor']['threads'].values:
                    schedule_mejor = datos_bloq['mejor'][datos_bloq['mejor']['threads'] == 4]['schedule'].iloc[0]
                else:
                    schedule_mejor = datos_bloq['mejor']['schedule'].mode()[0] if len(datos_bloq['mejor']['schedule'].mode()) > 0 else datos_bloq['mejor']['schedule'].iloc[0]
                ax.plot(datos_bloq['mejor']['threads'], datos_bloq['mejor']['speedup'], 
                        marker=marcadores_bloques['mejor'], linewidth=linewidth, markersize=markersize,
                        linestyle=estilos['mejor'], color=colores_bloques['mejor'],
                        label=f'Bloques mejor ({schedule_mejor})', zorder=4)
                
            if len(datos_bloq['peor']) > 0:
                # Peor speedup - preferir mostrar el schedule de 4 threads si existe, sino el más frecuente
                if 4 in datos_bloq['peor']['threads'].values:
                    schedule_peor = datos_bloq['peor'][datos_bloq['peor']['threads'] == 4]['schedule'].iloc[0]
                else:
                    schedule_peor = datos_bloq['peor']['schedule'].mode()[0] if len(datos_bloq['peor']['schedule'].mode()) > 0 else datos_bloq['peor']['schedule'].iloc[0]
                ax.plot(datos_bloq['peor']['threads'], datos_bloq['peor']['speedup'], 
                        marker=marcadores_bloques['peor'], linewidth=linewidth*0.8, markersize=markersize*0.8,
                        linestyle=estilos['peor'], color=colores_bloques['peor'], alpha=0.7,
                        label=f'Bloques peor ({schedule_peor})', zorder=2)
        
        # Graficar ley de Amdahl específica para este tamaño
        # NOTA: La ley de Amdahl proporciona un límite teórico superior asumiendo que las partes secuenciales
        # (Fase 1 y 3) no cambian. El speedup medido puede superarlo debido a un problema de medición:
        # - El secuencial se ejecuta una sola vez y su tiempo incluye inicialización (init ~284ms)
        # - El paralelo se promedia sobre múltiples ejecuciones: la primera es lenta (init ~285ms, similar al
        #   secuencial), pero las siguientes son mucho más rápidas (init ~53-63ms) aunque intentan limpiar caché
        # - Esto sugiere que la limpieza de caché no funciona correctamente o hay efectos de estado que persisten
        # - El promedio del paralelo (~78ms init) es artificialmente bajo comparado con el secuencial (284ms)
        # Esto no representa una mejora real del algoritmo, sino un artefacto de cómo se promedian las mediciones.
        # Extender hasta un punto que represente "infinito" para mostrar el límite teórico
        if nombre_tamano in pct_por_tamano:
            pct_fase2_tamano = pct_por_tamano[nombre_tamano]
            if threads_max == 0:
                threads_max = 8  # Valor por defecto
            
            # Calcular límite teórico máximo
            speedup_max_infinito_amdahl = 1.0 / (1.0 - pct_fase2_tamano)
            
            # Extender la curva de Amdahl hasta un punto que represente "infinito"
            # Usar valores hasta threads_max normalmente, luego agregar puntos intermedios
            # y finalmente un punto que represente "infinito" (posición ~10-11 en el eje)
            threads_amdahl = np.arange(1, threads_max + 1)
            speedup_amdahl = [calcular_ley_amdahl(pct_fase2_tamano, n) for n in threads_amdahl]
            
            # Extender la curva de Amdahl usando valores grandes en escala logarítmica
            # Usar potencias de 2 para que se vean bien en la escala log
            threads_extendidos = np.concatenate([
                threads_amdahl,
                np.array([16, 32, 64, 128, 256, 512, 1024])  # Valores grandes para mostrar acercamiento al límite
            ])
            speedup_extendidos = [
                calcular_ley_amdahl(pct_fase2_tamano, n) for n in threads_extendidos
            ]
            
            # Usar color rojo para la ley de Amdahl (diferente del verde del límite)
            ax.plot(threads_amdahl, speedup_amdahl, 
                    linestyle='--', linewidth=linewidth*0.8, alpha=0.7, color='#E74C3C',
                    label=f'Ley de Amdahl ({nombre_tamano}, p={pct_fase2_tamano:.2%})', zorder=1)
            
            # Extender la línea hasta el punto de "infinito" (línea más tenue)
            ax.plot(threads_extendidos, speedup_extendidos, 
                    linestyle='--', linewidth=linewidth*0.6, alpha=0.4, color='#E74C3C',
                    zorder=1)
    
    if threads_max == 0:
        threads_max = 8  # Valor por defecto
    
    # Ya no necesitamos pos_infinito_x con escala logarítmica
    # La escala log maneja automáticamente el espacio
    
    # Calcular el máximo de los datos observados primero
    y_max_necesario = 3.5  # Valor inicial
    for nombre_tamano, datos_metodos in datos_por_tamano_metodo.items():
        for metodo, datos in datos_metodos.items():
            if 'mejor' in datos and len(datos['mejor']) > 0:
                y_max_necesario = max(y_max_necesario, datos['mejor']['speedup'].max() + 0.2)
            if 'peor' in datos and len(datos['peor']) > 0:
                y_max_necesario = max(y_max_necesario, datos['peor']['speedup'].max() + 0.2)
    
    # Calcular y mostrar speedup máximo teórico para 4 threads (límite de núcleos físicos)
    # y límite teórico máximo cuando n → ∞
    # Usar el porcentaje paralelizable del tamaño mostrado (16k)
    speedup_max_4threads = None
    speedup_max_infinito = None
    
    if len(datos_por_tamano_metodo) > 0:
        nombre_tamano = list(datos_por_tamano_metodo.keys())[0]
        if nombre_tamano in pct_por_tamano:
            pct_fase2_16k = pct_por_tamano[nombre_tamano]
            speedup_max_4threads = calcular_ley_amdahl(pct_fase2_16k, 4)
            
            # Calcular límite teórico máximo cuando n → ∞: 1/(1-p)
            speedup_max_infinito = 1.0 / (1.0 - pct_fase2_16k)
            
            # Calcular el máximo necesario: ley de Amdahl en threads_max + margen para anotación
            speedup_amdahl_max = calcular_ley_amdahl(pct_fase2_16k, threads_max)
            y_max_necesario = max(y_max_necesario, speedup_amdahl_max, speedup_max_4threads + 0.3, speedup_max_infinito + 0.3)
            
            # Agregar línea horizontal indicando el límite teórico para 4 threads (verde vibrante)
            ax.axhline(y=speedup_max_4threads, color='#32CD32', linestyle=':', linewidth=2.5, 
                        alpha=0.8, zorder=1)
            
            # Agregar línea horizontal indicando el límite teórico máximo (n → ∞) en color diferente
            # Extender la línea hasta el final del gráfico en escala logarítmica
            max_x_plot = 1024 * 1.5  # Valor máximo del eje X
            ax.plot([0.7, max_x_plot], [speedup_max_infinito, speedup_max_infinito], 
                    color='#9B59B6', linestyle='-.', linewidth=2.5, 
                    alpha=0.8, zorder=1)
            
            # Agregar anotación para límite de 4 threads (asegurarse de que quepa en el rango)
            y_texto_4threads = min(speedup_max_4threads + 0.15, y_max_necesario - 0.1)
            ax.text(128+64, y_texto_4threads,  # Usar 4 directamente en escala log
                   f'Límite teórico (4 hilos): {speedup_max_4threads:.2f}x',
                   fontsize=fontsize-3, color='#32CD32', fontweight='bold',
                   ha='center',  # Centrar horizontalmente
                   bbox=dict(boxstyle='round,pad=0.3', facecolor='white', 
                            edgecolor='#32CD32', alpha=0.8))
            
            # Agregar anotación para límite máximo teórico (n → ∞)
            y_texto_infinito = min(speedup_max_infinito + 0.25, y_max_necesario - 0.1)
            # Colocar la anotación en una posición que se vea bien en escala logarítmica
            pos_texto_infinito = 17.2  # Posición en escala log (entre 8 y 64)
            ax.text(pos_texto_infinito, y_texto_infinito, 
                   f'Límite teórico máximo (n→∞): {speedup_max_infinito:.2f}x',
                   fontsize=fontsize-3, color='#9B59B6', fontweight='bold',
                   ha='left',  # Alinear a la izquierda
                   bbox=dict(boxstyle='round,pad=0.3', facecolor='white', 
                            edgecolor='#9B59B6', alpha=1))
    
    ax.set_xlabel('Número de hilos', fontsize=fontsize+1, fontweight='bold')
    ax.set_ylabel('Speedup', fontsize=fontsize+1, fontweight='bold')
    # Sin título para formato IEEE paper
    
    # Colocar leyenda en la parte superior izquierda pero un poco más abajo
    # para que no tape el texto del límite máximo
    ax.legend(fontsize=fontsize-2, loc='upper left', framealpha=0.95, 
              edgecolor='black', frameon=True, bbox_to_anchor=(0.31, 0.23))
    ax.grid(True, alpha=0.3, linewidth=1.5)
    
    # Usar escala logarítmica en el eje X para suavizar la visualización
    # Similar al gráfico de referencia de la ley de Amdahl
    # Esto elimina el salto visual y hace que el crecimiento se vea más natural
    ax.set_xscale('log', base=2)  # Escala logarítmica base 2
    
    # Extender el eje X hasta un valor grande para mostrar cómo se acerca al límite
    # Usar valores que sean potencias de 2 para que se vean bien en escala log
    max_threads_log = 1024  # Extender hasta 1024 threads para mostrar el acercamiento al límite
    ax.set_xlim(left=1, right=max_threads_log * 1.5)
    
    # Configurar ticks del eje X en escala logarítmica
    # Mostrar potencias de 2: 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024
    ticks_x_log = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048]
    # Filtrar solo los que están dentro del rango visible (1-8 son los datos reales)
    ticks_x_log = [t for t in ticks_x_log if t <= max_threads_log]
    labels_x_log = [str(t) if t <= threads_max else '' for t in ticks_x_log]
    # Agregar símbolo de infinito al final si queremos mantenerlo
    # Pero en escala log, es mejor mostrar valores grandes
    ax.set_xticks(ticks_x_log)
    ax.set_xticklabels(labels_x_log)
    
    # Ajustar límite del eje Y a 10 para mejor visualización
    # Esto da más espacio para el límite teórico máximo (5.19x) y las anotaciones
    ax.set_ylim(bottom=0, top=6)
    
    # Aumentar tamaño de ticks
    ax.tick_params(axis='both', labelsize=fontsize-1, width=1.5, length=6)
    
    # Aumentar grosor de los ejes
    for spine in ax.spines.values():
        spine.set_linewidth(1.5)
    
    # Ajustar subplots para que el gráfico ocupe más espacio
    # Dar más espacio en la parte inferior para la leyenda y en la derecha para el texto del límite
    plt.tight_layout(pad=1.0, rect=[0, 0.05, 0.98, 0.98])  # Más espacio abajo y menos a la derecha
    
    if formato == 'ieee':
        filename = 'speedup_vs_threads_ieee_twocol.png'
    else:
        filename = 'speedup_vs_threads_ppt.png'
    
    plt.savefig(os.path.join(output_dir, filename), dpi=300, bbox_inches='tight')
    plt.close()
    
    print(f"[OK] Gráfico guardado: {filename}")
    print(f"  Fracción paralelizable promedio (Fase 2): {pct_fase2_general:.2%}")
    print(f"  Tamaños mostrados: {', '.join(datos_por_tamano_metodo.keys())}")
    for nombre_tamano, datos_metodos in datos_por_tamano_metodo.items():
        pct_tamano = pct_por_tamano.get(nombre_tamano, pct_fase2_general)
        print(f"  {nombre_tamano}:")
        print(f"    - Fracción paralelizable: {pct_tamano:.2%}")
        for metodo, datos in datos_metodos.items():
            if 'mejor' in datos and len(datos['mejor']) > 0:
                print(f"    - {metodo.capitalize()} mejor speedup: {datos['mejor']['speedup'].values}")
            if 'peor' in datos and len(datos['peor']) > 0:
                print(f"    - {metodo.capitalize()} peor speedup: {datos['peor']['speedup'].values}")
        print(f"    - Speedup teórico Amdahl (8 hilos): {calcular_ley_amdahl(pct_tamano, 8):.3f}")
        # Calcular y mostrar límite máximo teórico (n → ∞)
        speedup_max_teorico = 1.0 / (1.0 - pct_tamano)
        print(f"    - Límite teórico máximo (n→∞): {speedup_max_teorico:.3f}x")

def grafico_tiempo_vs_tamaño(df_completo, output_dir, formato='ieee'):
    """Genera gráfico comparando tiempo vs tamaño para diferentes métodos y schedules"""
    # Filtrar solo datos de DNA
    df_dna = df_completo[df_completo['archivo'].str.contains('dna', case=False, na=False)].copy()
    
    if len(df_dna) == 0:
        print("Advertencia: No hay datos de DNA para graficar")
        return
    
    # Excluir 32k para mejor visualización (probablemente problemas de memoria con SSD externo USB)
    df_dna = df_dna[df_dna['longitud'] <= 17000].copy()
    
    # Ordenar por longitud
    df_dna = df_dna.sort_values('longitud')
    
    if formato == 'ieee':
        # IEEE two-column con altura reducida
        fig, ax = plt.subplots(figsize=(7.16, 10.0))
        fontsize = 18
        markersize = 10
        linewidth = 2.5
    else:  # ppt
        fig, ax = plt.subplots(figsize=(14, 8))
        fontsize = 12
        markersize = 8
        linewidth = 2
    
    # Colores y marcadores
    color_secuencial = '#2C3E50'
    color_mejor = '#2A9D8F'  # Verde para los mejores
    color_peor = '#E74C3C'   # Rojo para los peores
    
    # Marcadores estandarizados: secuencial=círculo, antidiagonal=triángulo, bloques=cuadrado
    marcadores_metodos = {
        'secuencial': 'o',      # Círculo
        'antidiagonal': '^',     # Triángulo
        'bloques': 's'           # Cuadrado
    }
    
    # Graficar secuencial
    df_secuencial = df_dna[df_dna['metodo'] == 'secuencial'].copy()
    if len(df_secuencial) > 0:
        # Agrupar por longitud (por si hay múltiples repeticiones)
        df_sec_agg = df_secuencial.groupby('longitud', as_index=False)['tiempo_total_ms'].mean()
        ax.plot(df_sec_agg['longitud'], df_sec_agg['tiempo_total_ms'] / 1000.0,  # Convertir a segundos
                marker=marcadores_metodos['secuencial'], linewidth=linewidth, 
                markersize=markersize, label='Secuencial', 
                color=color_secuencial, linestyle='-', zorder=5)
    
    # Graficar métodos paralelos: antidiagonal y bloques
    # Usar solo 4 threads (representa el límite de núcleos físicos, 6-8 son similares)
    threads_fijo = 4
    
    for metodo in ['antidiagonal', 'bloques']:
        # Filtrar por método y 4 threads
        df_metodo = df_dna[(df_dna['metodo'] == metodo) & (df_dna['threads'] == threads_fijo)].copy()
        if len(df_metodo) == 0:
            continue
        
        # Calcular mejor/peor schedule ESPECÍFICO para 4 threads basándose en tiempo promedio
        # (no speedup, ya que este gráfico muestra tiempo)
        longitud_col = 'longitud' if 'longitud' in df_metodo.columns else 'longitud_A'
        df_metodo_agg = df_metodo.groupby(['schedule', longitud_col], as_index=False)['tiempo_total_ms'].mean()
        
        # Calcular tiempo promedio total por schedule (promediando sobre todas las longitudes)
        df_schedule_avg = df_metodo_agg.groupby('schedule', as_index=False)['tiempo_total_ms'].mean()
        schedule_best = df_schedule_avg.loc[df_schedule_avg['tiempo_total_ms'].idxmin(), 'schedule']
        schedule_worst = df_schedule_avg.loc[df_schedule_avg['tiempo_total_ms'].idxmax(), 'schedule']
        
        # Para bloques: según la tabla LaTeX, para 16k static es peor que static,1
        # Ajustar para ser consistente: si static,1 es peor en promedio pero static es peor en 16k,
        # usar static como peor para consistencia con la tabla
        if metodo == 'bloques':
            # Verificar para 16k específicamente
            df_16k = df_metodo_agg[(df_metodo_agg[longitud_col] >= 16000) & (df_metodo_agg[longitud_col] <= 17000)]
            if len(df_16k) > 0:
                df_16k_avg = df_16k.groupby('schedule', as_index=False)['tiempo_total_ms'].mean()
                schedule_worst_16k = df_16k_avg.loc[df_16k_avg['tiempo_total_ms'].idxmax(), 'schedule']
                # Si el peor en 16k es static y el peor general es static,1, usar static
                if schedule_worst_16k == 'static' and schedule_worst == 'static,1':
                    schedule_worst = 'static'
        
        # Obtener todos los schedules únicos
        schedules_unicos = sorted(df_metodo['schedule'].unique())
        
        # Graficar todos los schedules usando los datos ya promediados
        for schedule in schedules_unicos:
            df_schedule_agg = df_metodo_agg[df_metodo_agg['schedule'] == schedule].copy()
            df_schedule_agg = df_schedule_agg.sort_values(longitud_col)
            
            if len(df_schedule_agg) > 0:
                # Determinar color y estilo según si es mejor, peor, o intermedio
                if schedule == schedule_best:
                    # Mejor: verde, línea sólida, más gruesa
                    color = color_mejor
                    estilo = '-'
                    grosor = linewidth
                    tam_marcador = markersize
                    etiqueta = f'{metodo.capitalize()} (mejor: {schedule})'
                    zorder = 4
                elif schedule == schedule_worst:
                    # Peor: rojo, línea punteada, más gruesa
                    color = color_peor
                    estilo = '--'
                    grosor = linewidth
                    tam_marcador = markersize
                    etiqueta = f'{metodo.capitalize()} (peor: {schedule})'
                    zorder = 3
                else:
                    # Intermedio: gris, línea punteada, delgada
                    color = '#95A5A6'
                    estilo = '--'
                    grosor = 1.0
                    tam_marcador = 4
                    etiqueta = f'{metodo.capitalize()} - {schedule}'
                    zorder = 2
                
                ax.plot(df_schedule_agg[longitud_col], df_schedule_agg['tiempo_total_ms'] / 1000.0,
                        marker='.' if schedule != schedule_best and schedule != schedule_worst else marcadores_metodos[metodo],
                        linewidth=grosor, markersize=tam_marcador,
                        label=etiqueta, color=color, linestyle=estilo, alpha=0.7 if schedule != schedule_best and schedule != schedule_worst else 1.0,
                        zorder=zorder)
    
    # Usar escalas lineales en lugar de logarítmicas
    # ax.set_xscale('log')  # Comentado: usar escala lineal
    # ax.set_yscale('log')  # Comentado: usar escala lineal
    
    ax.set_xlabel('Longitud de secuencias', fontsize=fontsize+1, fontweight='bold')
    ax.set_ylabel('Tiempo total (s)', fontsize=fontsize+1, fontweight='bold')
    # Título descriptivo (comentado para formato IEEE): este gráfico compara todos los schedules con 4 hilos
    # ax.set_title('Comparación de schedules con 4 hilos', fontsize=fontsize, fontweight='bold', pad=10)
    
    # Ajustar límites de ejes para mejor visualización con escala lineal
    if len(df_dna) > 0:
        x_min = df_dna['longitud'].min()
        x_max = df_dna['longitud'].max()
        # Redondear a valores "redondos"
        x_min = max(0, (x_min // 500) * 500)  # Redondear hacia abajo a múltiplos de 500
        x_max = ((x_max // 500) + 1) * 500  # Redondear hacia arriba a múltiplos de 500
        ax.set_xlim(left=x_min, right=x_max)
        
        # Para el eje Y, limitar a 2.5 segundos para mejor visualización
        # (aunque algunos casos extremos se salgan del rango)
        y_max_limite = 2.5
        ax.set_ylim(bottom=0, top=y_max_limite)
        
        # Verificar si hay valores que se salen del rango y agregar anotación si es necesario
        df_paralelos_4 = df_dna[(df_dna['metodo'] != 'secuencial') & (df_dna['threads'] == threads_fijo)]
        if len(df_paralelos_4) > 0:
            tiempos_max = df_paralelos_4.groupby(['metodo', 'schedule'])['tiempo_total_ms'].max() / 1000.0
            valores_fuera_rango = tiempos_max[tiempos_max > y_max_limite]
            if len(valores_fuera_rango) > 0:
                # Agregar texto indicando que hay valores fuera del rango, algunos hasta 8.8s
                ax.text(0.98, 0.98, 'Nota: algunos valores\nexceden 2.5s\n(hasta 8.8s)',
                       transform=ax.transAxes, fontsize=fontsize-4,
                       verticalalignment='top', horizontalalignment='right',
                       bbox=dict(boxstyle='round', facecolor='yellow', alpha=0.7))
    
    ax.legend(fontsize=fontsize-3, loc='upper left', framealpha=0.95, 
              edgecolor='black', frameon=True, ncol=1)
    ax.grid(True, alpha=0.3, linewidth=1.5)
    
    # Aumentar tamaño de ticks
    ax.tick_params(axis='both', labelsize=fontsize-1, width=1.5, length=6)
    
    # Aumentar grosor de los ejes
    for spine in ax.spines.values():
        spine.set_linewidth(1.5)
    
    # Ajustar subplots para que el gráfico ocupe más espacio (menos padding)
    plt.tight_layout(pad=1.0, rect=[0, 0.02, 1, 0.98])
    
    if formato == 'ieee':
        filename = 'tiempo_vs_tamaño_ieee_twocol.png'
    else:
        filename = 'tiempo_vs_tamaño_ppt.png'
    
    plt.savefig(os.path.join(output_dir, filename), dpi=300, bbox_inches='tight')
    plt.close()
    
    print(f"[OK] Gráfico guardado: {filename}")

def grafico_tiempo_vs_longitud_por_threads(df_completo, output_dir, formato='ieee'):
    """Genera gráfico de tiempo vs longitud mostrando diferentes números de threads
    usando el mejor schedule para cada estrategia calculado de forma consistente"""
    
    # Calcular mejor/peor schedule de forma consistente
    mejor_peor_schedules = calcular_mejor_peor_schedule(df_completo)
    
    # Filtrar solo datos de DNA
    df_dna = df_completo[df_completo['archivo'].str.contains('dna', case=False, na=False)].copy()
    
    if len(df_dna) == 0:
        print("Advertencia: No hay datos de DNA para graficar")
        return
    
    # Excluir 32k si existe
    df_dna = df_dna[df_dna['longitud'] <= 17000].copy() if 'longitud' in df_dna.columns else df_dna[df_dna['longitud_A'] <= 17000].copy()
    
    # Ordenar por longitud
    df_dna = df_dna.sort_values('longitud' if 'longitud' in df_dna.columns else 'longitud_A')
    
    if formato == 'ieee':
        # IEEE two-column con altura reducida
        fig, ax = plt.subplots(figsize=(7.16, 10.0))
        fontsize = 18
        markersize = 10
        linewidth = 2.5
    else:  # ppt
        fig, ax = plt.subplots(figsize=(14, 8))
        fontsize = 12
        markersize = 8
        linewidth = 2
    
    # Graficar secuencial
    df_secuencial = df_dna[df_dna['metodo'] == 'secuencial'].copy()
    if len(df_secuencial) > 0:
        # Agrupar por longitud
        longitud_col = 'longitud' if 'longitud' in df_secuencial.columns else 'longitud_A'
        df_sec_agg = df_secuencial.groupby(longitud_col, as_index=False)['tiempo_total_ms'].mean()
        ax.plot(df_sec_agg[longitud_col], df_sec_agg['tiempo_total_ms'] / 1000.0,
                marker='o', linewidth=linewidth, markersize=markersize,
                label='Secuencial', color='#2C3E50', linestyle='-', zorder=10)
    
    # Calcular mejor schedule específico para 4 threads basándose en tiempo promedio
    # (no usar el de speedup, ya que este gráfico muestra tiempo)
    df_dna_4t = df_dna[df_dna['threads'] == 4].copy()
    mejor_schedule = {}
    
    for metodo in ['antidiagonal', 'bloques']:
        df_metodo_4t = df_dna_4t[df_dna_4t['metodo'] == metodo].copy()
        if len(df_metodo_4t) > 0:
            # Calcular tiempo promedio por schedule para 4 threads
            longitud_col = 'longitud' if 'longitud' in df_metodo_4t.columns else 'longitud_A'
            df_metodo_agg = df_metodo_4t.groupby(['schedule', longitud_col], as_index=False)['tiempo_total_ms'].mean()
            df_schedule_avg = df_metodo_agg.groupby('schedule', as_index=False)['tiempo_total_ms'].mean()
            mejor_schedule[metodo] = df_schedule_avg.loc[df_schedule_avg['tiempo_total_ms'].idxmin(), 'schedule']
        else:
            # Fallback a los valores calculados por speedup
            mejor_schedule[metodo] = mejor_peor_schedules[metodo]['mejor']
    
    # Colores: 2 y 4 threads (4 representa el límite de núcleos físicos)
    colores_threads = {
        2: '#E74C3C',   # Rojo - destacado porque aprovecha bien los recursos
        4: '#3498DB'    # Azul - representa el límite de 4 núcleos físicos (6-8 threads son similares)
    }
    
    # Marcadores estandarizados: antidiagonal=triángulo, bloques=cuadrado
    marcadores_metodos = {
        'antidiagonal': '^',  # Triángulo
        'bloques': 's'         # Cuadrado
    }
    
    # Graficar para cada método y número de threads
    for metodo in ['antidiagonal', 'bloques']:
        schedule_mejor = mejor_schedule[metodo]
        df_metodo = df_dna[(df_dna['metodo'] == metodo) & 
                          (df_dna['schedule'] == schedule_mejor)].copy()
        
        if len(df_metodo) == 0:
            continue
        
        # Para cada número de threads
        # Mostrar solo 2 y 4 threads (4 representa el límite de núcleos físicos, 6-8 son similares)
        threads_a_mostrar = [2, 4]
        
        for threads in sorted(df_metodo['threads'].unique()):
            if threads == 1:  # Saltar secuencial
                continue
            if threads not in threads_a_mostrar:  # Solo mostrar 2 y 4 threads
                continue
            
            df_threads = df_metodo[df_metodo['threads'] == threads].copy()
            
            # Agrupar por longitud, promediando tiempos
            longitud_col = 'longitud' if 'longitud' in df_threads.columns else 'longitud_A'
            df_threads_agg = df_threads.groupby(longitud_col, as_index=False)['tiempo_total_ms'].mean()
            df_threads_agg = df_threads_agg.sort_values(longitud_col)
            
            if len(df_threads_agg) > 0:
                color = colores_threads.get(threads, '#95A5A6')
                marcador = marcadores_metodos[metodo]
                
                # Etiqueta: para 4 hilos, mencionar que 6-8 son similares
                if threads == 4:
                    etiqueta = f'{metodo.capitalize()} ({schedule_mejor}, {threads} hilos, ~6-8 similar)'
                else:
                    etiqueta = f'{metodo.capitalize()} ({schedule_mejor}, {threads} hilos)'
                
                ax.plot(df_threads_agg[longitud_col], df_threads_agg['tiempo_total_ms'] / 1000.0,
                        marker=marcador, linewidth=linewidth, markersize=markersize,
                        label=etiqueta, color=color, linestyle='-', zorder=5)
    
    # Ajustar límites de ejes
    if len(df_dna) > 0:
        longitud_col = 'longitud' if 'longitud' in df_dna.columns else 'longitud_A'
        x_min = df_dna[longitud_col].min()
        x_max = df_dna[longitud_col].max()
        x_min = max(0, (x_min // 500) * 500)
        x_max = ((x_max // 500) + 1) * 500
        ax.set_xlim(left=x_min, right=x_max)
        
        # Para el eje Y: calcular solo con los datos que realmente se grafican
        y_max = 0
        
        # Secuencial
        if len(df_secuencial) > 0:
            y_max = max(y_max, (df_secuencial['tiempo_total_ms'] / 1000.0).max())
        
        # Antidiagonal con static
        df_antidiagonal = df_dna[(df_dna['metodo'] == 'antidiagonal') & 
                                 (df_dna['schedule'] == 'static')].copy()
        if len(df_antidiagonal) > 0:
            y_max = max(y_max, (df_antidiagonal['tiempo_total_ms'] / 1000.0).max())
        
        # Bloques con dynamic
        df_bloques = df_dna[(df_dna['metodo'] == 'bloques') & 
                           (df_dna['schedule'] == 'dynamic')].copy()
        if len(df_bloques) > 0:
            y_max = max(y_max, (df_bloques['tiempo_total_ms'] / 1000.0).max())
        
        if y_max > 0:
            # Agregar un pequeño margen (5%) y redondear hacia arriba a un valor "redondo"
            y_max_con_margen = y_max * 1.05
            # Redondear a múltiplos de 0.2 para mejor legibilidad
            y_max_rounded = np.ceil(y_max_con_margen * 5) / 5
            ax.set_ylim(bottom=0, top=y_max_rounded)
    
    ax.set_xlabel('Longitud de secuencias', fontsize=fontsize+1, fontweight='bold')
    ax.set_ylabel('Tiempo total (s)', fontsize=fontsize+1, fontweight='bold')
    # Título descriptivo (comentado para formato IEEE): este gráfico muestra escalabilidad con número de hilos
    # ax.set_title('Escalabilidad por número de hilos (mejor schedule)', fontsize=fontsize, fontweight='bold', pad=10)
    
    ax.legend(fontsize=fontsize-3, loc='upper left', framealpha=0.95, 
              edgecolor='black', frameon=True, ncol=1)
    ax.grid(True, alpha=0.3, linewidth=1.5)
    
    # Aumentar tamaño de ticks
    ax.tick_params(axis='both', labelsize=fontsize-1, width=1.5, length=6)
    
    # Aumentar grosor de los ejes
    for spine in ax.spines.values():
        spine.set_linewidth(1.5)
    
    # Ajustar subplots
    plt.tight_layout(pad=1.0, rect=[0, 0.02, 1, 0.98])
    
    if formato == 'ieee':
        filename = 'tiempo_vs_longitud_por_threads_ieee_twocol.png'
    else:
        filename = 'tiempo_vs_longitud_por_threads_ppt.png'
    
    plt.savefig(os.path.join(output_dir, filename), dpi=300, bbox_inches='tight')
    plt.close()
    
    print(f"[OK] Gráfico guardado: {filename}")

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
        
        f.write(f"\nFase 2 (Fase más costosa):\n")
        f.write(f"  Promedio: {df['pct_llenado'].mean():.2f}% del tiempo total\n")
        f.write(f"  Mínimo:   {df['pct_llenado'].min():.2f}%\n")
        f.write(f"  Máximo:   {df['pct_llenado'].max():.2f}%\n")
        f.write(f"  Mediana:  {df['pct_llenado'].median():.2f}%\n")
        
        f.write(f"\nFase 1:\n")
        f.write(f"  Promedio: {df['pct_init'].mean():.2f}%\n")
        f.write(f"  Mínimo:   {df['pct_init'].min():.2f}%\n")
        f.write(f"  Máximo:   {df['pct_init'].max():.2f}%\n")
        
        f.write(f"\nFase 3:\n")
        f.write(f"  Promedio: {df['pct_traceback'].mean():.2f}%\n")
        f.write(f"  Mínimo:   {df['pct_traceback'].min():.2f}%\n")
        f.write(f"  Máximo:   {df['pct_traceback'].max():.2f}%\n")
        
        f.write("\n" + "=" * 80 + "\n")
        f.write("ARCHIVO CON MAYOR % EN FASE 2 (cuello de botella):\n")
        idx_max = df['pct_llenado'].idxmax()
        row_max = df.loc[idx_max]
        f.write(f"  Archivo: {row_max['archivo']}\n")
        f.write(f"  % Fase 2: {row_max['pct_llenado']:.2f}%\n")
        f.write(f"  Tiempo Fase 2: {row_max['tiempo_llenado_ms']:.2f} ms\n")
        f.write(f"  Tiempo total: {row_max['tiempo_total_ms']:.2f} ms\n")
        
        f.write("\n" + "=" * 80 + "\n")
        f.write("PORCENTAJES DETALLADOS POR ARCHIVO:\n")
        f.write("-" * 80 + "\n")
        for _, row in ordenar_por_tipo_y_longitud(df).iterrows():
            f.write(f"\n{row['archivo']} ({row['longitud']} chars):\n")
            f.write(f"  Fase 1: {row['pct_init']:.2f}% ({row['tiempo_init_ms']:.2f} ms)\n")
            f.write(f"  Fase 2: {row['pct_llenado']:.2f}% ({row['tiempo_llenado_ms']:.2f} ms)\n")
            f.write(f"  Fase 3: {row['pct_traceback']:.2f}% ({row['tiempo_traceback_ms']:.2f} ms)\n")
            f.write(f"  Total:  {row['tiempo_total_ms']:.2f} ms\n")
    
    print(f"[OK] Resumen de texto guardado: resumen_analisis.txt")

def main():
    parser = argparse.ArgumentParser(
        description='Analiza resultados de benchmark y genera gráficos en formato IEEE y PPT'
    )
    parser.add_argument('-i', '--input', 
                       default='resultados/resultados_promedio.csv',
                       help='Archivo CSV de entrada con datos promediados')
    parser.add_argument('-o', '--output', 
                       default='graficos',
                       help='Directorio para guardar gráficos (default: graficos)')
    parser.add_argument('--formato',
                       choices=['ieee', 'ppt', 'ambos'],
                       default='ambos',
                       help='Formato de gráficos: ieee, ppt, o ambos')
    parser.add_argument('-a', '--archivo-torta',
                       help='Archivo específico para gráfico de torta (opcional)')
    
    args = parser.parse_args()
    
    # Crear directorio de salida
    os.makedirs(args.output, exist_ok=True)
    
    print("=" * 80)
    print("ANÁLISIS DE BENCHMARK - Needleman-Wunsch")
    print("=" * 80)
    print()
    
    # Cargar datos secuenciales (para gráficos de fases)
    print(f"Cargando datos desde: {args.input}")
    df = cargar_datos_csv(args.input)
    
    if df is None or len(df) == 0:
        print("ERROR: No se encontraron datos secuenciales")
        return
    
    print(f"[OK] Cargados {len(df)} archivos de resultados secuenciales\n")
    
    # Cargar datos completos (para gráfico de speedup)
    df_completo = cargar_datos_completos(args.input)
    print(f"[OK] Cargados datos completos (secuenciales + paralelos)\n")
    
    # Mostrar resumen rápido
    print("RESUMEN RÁPIDO:")
    print(f"  Fase 2: {df['pct_llenado'].mean():.1f}% promedio del tiempo total")
    print(f"  Fase 1: {df['pct_init'].mean():.1f}% promedio")
    print(f"  Fase 3: {df['pct_traceback'].mean():.1f}% promedio")
    print()
    
    # Generar solo los gráficos necesarios
    formato = 'ieee'  # Solo generamos en formato IEEE twocol
    
    print(f"\n=== Generando gráficos en formato {formato.upper()} ===")
    
    # 1. Gráfico de porcentaje de llenado por archivo (el que ya existe)
    print("\n1. Generando gráfico de porcentaje de llenado...")
    grafico_llenado_detalle(df, args.output, formato)
    
    # 2. Gráfico de speedup vs threads con ley de Amdahl
    print("\n2. Generando gráfico de speedup vs hilos...")
    grafico_speedup_vs_threads(df_completo, df, args.output, formato)
    
    # 3. Gráfico de tiempo vs tamaño comparando métodos
    print("\n3. Generando gráfico de tiempo vs tamaño...")
    grafico_tiempo_vs_tamaño(df_completo, args.output, formato)
    
    # 4. Gráfico de tiempo vs longitud por threads y estrategia
    print("\n4. Generando gráfico de tiempo vs longitud por hilos...")
    grafico_tiempo_vs_longitud_por_threads(df_completo, args.output, formato)
    
    print()
    print("=" * 80)
    print(f"[OK] Análisis completado. Gráficos guardados en: {args.output}/")
    print("=" * 80)

if __name__ == '__main__':
    main()

