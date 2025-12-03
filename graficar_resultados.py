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
    # Buscar patrones como dna_128, dna_1k, dna_32k
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
    print(f"✓ Gráfico guardado: {filename}")

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
    print(f"✓ Gráfico guardado: {filename}")

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
    print(f"✓ Gráfico guardado: {filename}")

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
    print(f"✓ Gráfico guardado: {filename}")

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
    print(f"✓ Gráfico guardado: {filename}")

def grafico_llenado_detalle(df, output_dir, formato='ppt'):
    """Genera imágenes enfocadas en el porcentaje de tiempo en llenado de matriz"""
    df_ordenado = ordenar_por_tipo_y_longitud(df)
    
    if formato == 'ieee':
        # IEEE: formato vertical con barras horizontales
        fig, ax = plt.subplots(figsize=(3.5, 5.25))  # IEEE one-column vertical (4:6)
        fontsize = 10
    else:  # ppt
        fig, ax = plt.subplots(figsize=(14, 6))
        fontsize = 10
    
    archivos = [row['archivo'] for _, row in df_ordenado.iterrows()]
    mean_val = df_ordenado['pct_llenado'].mean()
    
    if formato == 'ieee':
        # Barras horizontales para IEEE (vertical)
        y = np.arange(len(df_ordenado))
        ax.barh(y, df_ordenado['pct_llenado'], color='#4ECDC4')
        ax.set_yticks(y)
        ax.set_yticklabels(archivos, fontsize=fontsize-1)
        ax.invert_yaxis()  # Primero arriba
        ax.set_xlabel('% Fase 2', fontsize=fontsize, fontweight='bold')
        ax.set_ylabel('Archivo (ordenado por longitud)', fontsize=fontsize, fontweight='bold')
        ax.set_title('Porcentaje de Tiempo en Fase 2 por Archivo', 
                    fontsize=fontsize+2, fontweight='bold')
        ax.axvline(x=mean_val, color='red', linestyle='--', linewidth=1.5,
                   label=f'Promedio: {mean_val:.2f}%')
        ax.legend(fontsize=fontsize-1)
        ax.grid(True, alpha=0.3, axis='x')
    else:
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
    
    if formato == 'ieee':
        # Versión two-column con formato 3:6 y barras agrupadas (similar a porcentajes separados)
        # pero destacando la Fase 2
        fig_twocol, ax_twocol = plt.subplots(figsize=(7.16, 14.32))  # IEEE two-column vertical (3:6)
        fontsize_twocol = 18  # Fuente más grande para mejor legibilidad
        
        y = np.arange(len(df_ordenado))
        height = 0.22  # Altura de las barras agrupadas
        
        # Usar colores por defecto de matplotlib (C0, C1, C2)
        # Barras horizontales agrupadas - Fase 2 más ancha para destacarla
        bars1 = ax_twocol.barh(y - height*1.1, df_ordenado['pct_init'], height, 
                               label='Fase 1', color='C0', edgecolor='white', linewidth=0.5)
        bars2 = ax_twocol.barh(y, df_ordenado['pct_llenado'], height*1.4,  # Fase 2 más ancha
                               label='Fase 2', color='C2', edgecolor='white', linewidth=1)
        bars3 = ax_twocol.barh(y + height*1.1, df_ordenado['pct_traceback'], height, 
                               label='Fase 3', color='C1', edgecolor='white', linewidth=0.5)
        
        ax_twocol.set_yticks(y)
        ax_twocol.set_yticklabels(archivos, fontsize=fontsize_twocol-1)  # Fuente más grande
        ax_twocol.invert_yaxis()  # Primero arriba
        
        # Poner valores solo de la Fase 2 (la más importante) al final de la barra
        for i, val2 in enumerate(df_ordenado['pct_llenado']):
            ax_twocol.text(val2 + 2, i, f'{val2:.1f}%',
                         fontsize=fontsize_twocol-2, fontweight='bold',  # Fuente más grande
                         color='black', va='center', ha='left',
                         bbox=dict(boxstyle='round,pad=0.3', facecolor='white', 
                                 edgecolor='black', alpha=0.9, linewidth=0.8))
        
        # Línea de promedio solo para Fase 2 (sin label para no incluirla en la leyenda)
        mean_val = df_ordenado['pct_llenado'].mean()
        ax_twocol.axvline(x=mean_val, color='red', linestyle='--', linewidth=2.5)
        
        # Agregar texto sobre la línea roja con el promedio en la parte superior (horizontal)
        # Como el eje Y está invertido, usar un valor negativo más grande para posicionar más arriba
        ax_twocol.text(mean_val, -0.7, f'Promedio Fase 2: {mean_val:.2f}%',
                      fontsize=fontsize_twocol-2, fontweight='bold', color='red',
                      ha='center', va='top', rotation=0,
                      bbox=dict(boxstyle='round,pad=0.3', facecolor='white', edgecolor='red', alpha=0.8))
        
        ax_twocol.set_xlabel('Porcentaje del tiempo total (%)', fontsize=fontsize_twocol+1, fontweight='bold')
        ax_twocol.set_ylabel('Longitud', fontsize=fontsize_twocol+1, fontweight='bold')
        ax_twocol.set_xlim(0, 100)
        
        # Colocar leyenda entre dna_32k y el eje horizontal (más arriba)
        ax_twocol.legend(fontsize=fontsize_twocol-2, loc='lower center', 
                        bbox_to_anchor=(0.5, -0.00), framealpha=0.95, 
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
        plt.tight_layout(pad=1.0, rect=[0, 0.02, 1, 0.98])
        plt.savefig(os.path.join(output_dir, 'porcentaje_llenado_por_archivo_promedio_ieee_twocol.png'), 
                   dpi=300, bbox_inches='tight')
        plt.close(fig_twocol)
    
    plt.close()
    print(f"✓ Gráfico guardado: {filename}")

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
    print(f"✓ Gráfico guardado: {filename}")

def cargar_datos_completos(archivo_csv):
    """Carga todos los datos (secuenciales y paralelos) desde el CSV"""
    df = pd.read_csv(archivo_csv)
    
    # Agregar columnas necesarias
    df['longitud'] = df['archivo_fasta'].apply(extraer_longitud)
    df['archivo'] = df['archivo_fasta'].apply(
        lambda x: os.path.basename(x).replace('.fasta', '')
    )
    
    return df

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
        print(f"  Schedules seleccionados por método y threads:")
        for metodo in df_speedup['metodo'].unique():
            for threads in sorted(df_speedup['threads'].unique()):
                subset = df_speedup[(df_speedup['metodo'] == metodo) & (df_speedup['threads'] == threads)]
                if len(subset) > 0:
                    schedules_used = subset['schedule'].value_counts()
                    print(f"    {metodo} ({threads} threads): {dict(schedules_used)}")
    
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
    # Calcular speedup usando el mejor schedule para cada configuración
    df_speedup = calcular_speedup(df_completo, usar_mejor_schedule=True)
    
    if df_speedup is None or len(df_speedup) == 0:
        print("Advertencia: No se pudo calcular speedup")
        return
    
    # Calcular porcentaje de Fase 2 (paralelizable) desde datos secuenciales
    pct_fase2 = df_secuencial['pct_llenado'].mean() / 100.0  # Convertir a fracción (0-1)
    
    # Filtrar tamaños donde el paralelismo realmente ayuda
    # Excluir tamaños muy pequeños (overhead domina) y muy grandes (problemas de escalabilidad)
    # Usar tamaños medianos-grandes donde el speedup es más representativo
    df_speedup_filtrado = df_speedup[
        (df_speedup['longitud_A'] >= 1024) & 
        (df_speedup['longitud_A'] <= 16384)
    ].copy()
    
    if len(df_speedup_filtrado) == 0:
        print("Advertencia: No hay datos después del filtrado")
        df_speedup_filtrado = df_speedup
    
    # Agrupar por threads, promediando speedup (usando datos filtrados)
    df_threads = df_speedup_filtrado.groupby('threads', as_index=False)['speedup'].mean()
    df_threads = df_threads.sort_values('threads')
    
    # También calcular speedup con todos los datos para comparación
    df_threads_todos = df_speedup.groupby('threads', as_index=False)['speedup'].mean()
    df_threads_todos = df_threads_todos.sort_values('threads')
    
    if formato == 'ieee':
        # IEEE two-column vertical (3:6)
        fig, ax = plt.subplots(figsize=(7.16, 14.32))
        fontsize = 18
        markersize = 10
        linewidth = 2.5
    else:  # ppt
        fig, ax = plt.subplots(figsize=(14, 8))
        fontsize = 12
        markersize = 8
        linewidth = 2
    
    # Graficar speedup real (promedio) - datos filtrados
    ax.plot(df_threads['threads'], df_threads['speedup'], 
            marker='o', linewidth=linewidth, markersize=markersize,
            label='Speedup observado (tamaños 1k-16k)', color='#2A9D8F', zorder=3)
    
    # Graficar speedup con todos los datos (línea punteada para comparación)
    ax.plot(df_threads_todos['threads'], df_threads_todos['speedup'], 
            marker='s', linewidth=linewidth*0.7, markersize=markersize*0.7,
            linestyle='--', alpha=0.6,
            label='Speedup observado (todos los tamaños)', color='#95A5A6', zorder=2)
    
    # Calcular y graficar ley de Amdahl
    threads_max = int(df_threads['threads'].max())
    threads_amdahl = np.arange(1, threads_max + 1)
    speedup_amdahl = [calcular_ley_amdahl(pct_fase2, n) for n in threads_amdahl]
    
    ax.plot(threads_amdahl, speedup_amdahl, 
            'r--', linewidth=linewidth, alpha=0.8,
            label=f'Ley de Amdahl (p={pct_fase2:.2%})', zorder=2)
    
    # Línea de referencia speedup = 1
    ax.axhline(y=1, color='gray', linestyle=':', linewidth=1.5, 
               alpha=0.5, label='Speedup = 1', zorder=1)
    
    # Línea ideal (speedup = número de threads) - solo hasta donde tenga sentido
    ax.plot([1, min(threads_max, 8)], [1, min(threads_max, 8)], 
            'k--', linewidth=1.5, alpha=0.4, 
            label='Speedup ideal', zorder=1)
    
    ax.set_xlabel('Número de threads', fontsize=fontsize+1, fontweight='bold')
    ax.set_ylabel('Speedup', fontsize=fontsize+1, fontweight='bold')
    ax.set_title('Speedup vs Número de Threads\n(Comparación con implementación secuencial)', 
                 fontsize=fontsize+2, fontweight='bold', pad=20)
    
    ax.legend(fontsize=fontsize-2, loc='upper left', framealpha=0.95, 
              edgecolor='black', frameon=True)
    ax.grid(True, alpha=0.3, linewidth=1.5)
    ax.set_xlim(left=0.5, right=threads_max + 0.5)
    ax.set_ylim(bottom=0)
    
    # Aumentar tamaño de ticks
    ax.tick_params(axis='both', labelsize=fontsize-1, width=1.5, length=6)
    
    # Aumentar grosor de los ejes
    for spine in ax.spines.values():
        spine.set_linewidth(1.5)
    
    plt.tight_layout()
    
    if formato == 'ieee':
        filename = 'speedup_vs_threads_ieee_twocol.png'
    else:
        filename = 'speedup_vs_threads_ppt.png'
    
    plt.savefig(os.path.join(output_dir, filename), dpi=300, bbox_inches='tight')
    plt.close()
    
    print(f"✓ Gráfico guardado: {filename}")
    print(f"  Fracción paralelizable (Fase 2): {pct_fase2:.2%}")
    print(f"  Speedup observado (filtrado): {df_threads['speedup'].values}")
    print(f"  Speedup observado (todos): {df_threads_todos['speedup'].values}")
    print(f"  Speedup teórico Amdahl (8 threads): {calcular_ley_amdahl(pct_fase2, 8):.3f}")
    print(f"  Eficiencia vs Amdahl: {(df_threads[df_threads['threads']==8]['speedup'].values[0] / calcular_ley_amdahl(pct_fase2, 8) * 100):.1f}%")

def grafico_tiempo_vs_tamaño(df_completo, output_dir, formato='ieee'):
    """Genera gráfico comparando tiempo vs tamaño para diferentes métodos y schedules"""
    # Filtrar solo datos de DNA
    df_dna = df_completo[df_completo['archivo'].str.contains('dna', case=False, na=False)].copy()
    
    if len(df_dna) == 0:
        print("Advertencia: No hay datos de DNA para graficar")
        return
    
    # Ordenar por longitud
    df_dna = df_dna.sort_values('longitud')
    
    if formato == 'ieee':
        # IEEE two-column vertical (3:6)
        fig, ax = plt.subplots(figsize=(7.16, 14.32))
        fontsize = 18
        markersize = 10
        linewidth = 2.5
    else:  # ppt
        fig, ax = plt.subplots(figsize=(14, 8))
        fontsize = 12
        markersize = 8
        linewidth = 2
    
    # Colores y marcadores para diferentes métodos
    colores_metodos = {
        'secuencial': '#2C3E50',
        'antidiagonal': '#E74C3C',
        'bloques': '#3498DB'
    }
    
    marcadores_metodos = {
        'secuencial': 'o',
        'antidiagonal': 's',
        'bloques': '^'
    }
    
    # Graficar secuencial
    df_secuencial = df_dna[df_dna['metodo'] == 'secuencial'].copy()
    if len(df_secuencial) > 0:
        # Agrupar por longitud (por si hay múltiples repeticiones)
        df_sec_agg = df_secuencial.groupby('longitud', as_index=False)['tiempo_total_ms'].mean()
        ax.plot(df_sec_agg['longitud'], df_sec_agg['tiempo_total_ms'] / 1000.0,  # Convertir a segundos
                marker=marcadores_metodos['secuencial'], linewidth=linewidth, 
                markersize=markersize, label='Secuencial', 
                color=colores_metodos['secuencial'], linestyle='-', zorder=5)
    
    # Graficar métodos paralelos: antidiagonal y bloques
    for metodo in ['antidiagonal', 'bloques']:
        df_metodo = df_dna[df_dna['metodo'] == metodo].copy()
        if len(df_metodo) == 0:
            continue
        
        # Para cada método, mostrar el mejor tiempo por longitud (mejor = menor tiempo)
        # Agrupar por longitud, método y schedule, y tomar el mínimo
        df_metodo_agg = df_metodo.groupby(['longitud', 'schedule'], as_index=False)['tiempo_total_ms'].mean()
        df_metodo_best = df_metodo_agg.loc[df_metodo_agg.groupby('longitud')['tiempo_total_ms'].idxmin()]
        df_metodo_best = df_metodo_best.sort_values('longitud')
        
        # Graficar mejor configuración por método
        ax.plot(df_metodo_best['longitud'], df_metodo_best['tiempo_total_ms'] / 1000.0,  # Convertir a segundos
                marker=marcadores_metodos[metodo], linewidth=linewidth, 
                markersize=markersize, label=f'{metodo.capitalize()} (mejor)', 
                color=colores_metodos[metodo], linestyle='-', zorder=4)
        
        # También mostrar diferentes schedules para cada método (con líneas más delgadas)
        schedules_unicos = sorted(df_metodo['schedule'].unique())
        # Colores para schedules: usar colores complementarios
        colores_schedules = ['#95A5A6', '#7F8C8D', '#34495E', '#2C3E50', '#1ABC9C', '#16A085']
        
        for idx, schedule in enumerate(schedules_unicos):
            df_schedule = df_metodo[df_metodo['schedule'] == schedule].copy()
            df_schedule_agg = df_schedule.groupby('longitud', as_index=False)['tiempo_total_ms'].mean()
            df_schedule_agg = df_schedule_agg.sort_values('longitud')
            
            if len(df_schedule_agg) > 0:
                color_schedule = colores_schedules[idx % len(colores_schedules)]
                ax.plot(df_schedule_agg['longitud'], df_schedule_agg['tiempo_total_ms'] / 1000.0,
                        marker='.', linewidth=1.0, markersize=4, 
                        label=f'{metodo.capitalize()} - {schedule}',
                        color=color_schedule, linestyle='--', alpha=0.5, zorder=2)
    
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('Longitud de secuencias', fontsize=fontsize+1, fontweight='bold')
    ax.set_ylabel('Tiempo total (s)', fontsize=fontsize+1, fontweight='bold')
    ax.set_title('Tiempo de Ejecución vs Tamaño de Entrada\n(Comparación de métodos y schedules)', 
                 fontsize=fontsize+2, fontweight='bold', pad=20)
    
    ax.legend(fontsize=fontsize-3, loc='upper left', framealpha=0.95, 
              edgecolor='black', frameon=True, ncol=1)
    ax.grid(True, alpha=0.3, linewidth=1.5, which='both')
    
    # Aumentar tamaño de ticks
    ax.tick_params(axis='both', labelsize=fontsize-1, width=1.5, length=6)
    
    # Aumentar grosor de los ejes
    for spine in ax.spines.values():
        spine.set_linewidth(1.5)
    
    plt.tight_layout()
    
    if formato == 'ieee':
        filename = 'tiempo_vs_tamaño_ieee_twocol.png'
    else:
        filename = 'tiempo_vs_tamaño_ppt.png'
    
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
    
    print(f"✓ Resumen de texto guardado: resumen_analisis.txt")

def main():
    parser = argparse.ArgumentParser(
        description='Analiza resultados de benchmark y genera gráficos en formato IEEE y PPT'
    )
    parser.add_argument('-i', '--input', 
                       default='resultados/resultados_completo_promedio.csv',
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
    
    print(f"✓ Cargados {len(df)} archivos de resultados secuenciales\n")
    
    # Cargar datos completos (para gráfico de speedup)
    df_completo = cargar_datos_completos(args.input)
    print(f"✓ Cargados datos completos (secuenciales + paralelos)\n")
    
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
    print("\n2. Generando gráfico de speedup vs threads...")
    grafico_speedup_vs_threads(df_completo, df, args.output, formato)
    
    # 3. Gráfico de tiempo vs tamaño comparando métodos
    print("\n3. Generando gráfico de tiempo vs tamaño...")
    grafico_tiempo_vs_tamaño(df_completo, args.output, formato)
    
    print()
    print("=" * 80)
    print(f"✓ Análisis completado. Gráficos guardados en: {args.output}/")
    print("=" * 80)

if __name__ == '__main__':
    main()

