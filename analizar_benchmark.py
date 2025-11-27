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
# Intentamos usar Pillow para rotar imágenes cuando el usuario quiera versiones "giradas"
try:
    from PIL import Image
except Exception:
    Image = None

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


def ordenar_por_tipo_y_longitud(df):
    """Ordena el DataFrame colocando primero las filas de tipo 'dna' (de menor a mayor longitud)
    y luego las de tipo 'protein' (de menor a mayor longitud).
    Devuelve una copia ordenada.
    """
    if df is None or len(df) == 0:
        return df
    tipo_order = {'dna': 0, 'protein': 1}
    df_copy = df.copy()
    df_copy['__tipo_ord'] = df_copy['tipo'].map(tipo_order).fillna(2)
    df_copy = df_copy.sort_values(['__tipo_ord', 'longitud'], ascending=[True, True])
    df_copy = df_copy.drop(columns=['__tipo_ord'])
    return df_copy


def _format_spanish_number(value, decimals=2):
    """Formatea números con separador de miles '.' y separador decimal ',' (estilo español).
    Por ejemplo: 352804.0 -> '352.804,00'
    """
    try:
        if decimals == 0:
            s = f"{value:,.0f}"
        else:
            s = f"{value:,.{decimals}f}"
        # s usa coma como separador de miles y punto como decimal en locale C (ej: '352,804.00')
        # Queremos '.' miles y ',' decimal -> intercambiamos
        s = s.replace(',', 'X').replace('.', ',').replace('X', '.')
        return s
    except Exception:
        return str(value)

def grafico_porcentajes_apilados(df, output_dir):
    """Gráfico de barras apiladas mostrando porcentajes por fase"""
    df_ordenado = ordenar_por_tipo_y_longitud(df)
    # Añadir columna en segundos para facilitar lectura humana
    df_ordenado = df_ordenado.copy()
    df_ordenado['total_s'] = df_ordenado['total_ms'] / 1000.0
    
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
    df_ordenado = ordenar_por_tipo_y_longitud(df)
    # Asegurar columna en segundos usada por los gráficos
    df_ordenado = df_ordenado.copy()
    if 'total_s' not in df_ordenado.columns:
        df_ordenado['total_s'] = df_ordenado['total_ms'] / 1000.0

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
    df_ordenado = ordenar_por_tipo_y_longitud(df)
    
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

def grafico_tiempo_total_por_longitud(df, output_dir):
    """Grafica el tiempo total (ms) en función de la longitud de entrada.
    Se muestran puntos y líneas separadas por tipo (dna/protein). Escala log-log.
    Genera versiones para publicación (onecol / twocol).
    """
    df_ordenado = ordenar_por_tipo_y_longitud(df)

    fig, ax = plt.subplots(figsize=(14, 8))

    for tipo, marker, color in [('dna', 'o', '#2A9D8F'), ('protein', 's', '#E76F51')]:
        subset = df_ordenado[df_ordenado['tipo'] == tipo]
        if len(subset) == 0:
            continue
        # Robustecer: si no existe 'total_s', calcular a partir de 'total_ms'
        if 'total_s' in subset.columns:
            y_vals = subset['total_s']
        else:
            y_vals = subset['total_ms'] / 1000.0
        ax.plot(subset['longitud'], y_vals, marker=marker, linestyle='-',
                linewidth=2, markersize=6, label=tipo.capitalize(), color=color)

    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('Longitud de Secuencias (caracteres)', fontsize=12, fontweight='bold')
    ax.set_ylabel('Tiempo Total (s)', fontsize=12, fontweight='bold')
    ax.set_title('Tiempo Total vs Longitud de Entrada', fontsize=14, fontweight='bold')
    ax.grid(True, which='both', alpha=0.3)
    ax.legend(fontsize=11)

    plt.tight_layout()
    mainfile = os.path.join(output_dir, 'tiempo_total_vs_longitud.png')
    fig.savefig(mainfile, dpi=300, bbox_inches='tight')
    try:
        fig.set_size_inches((3.5, 2.4))
        fig.savefig(os.path.join(output_dir, 'tiempo_total_vs_longitud_ieee_onecol.png'), dpi=300, bbox_inches='tight')
        fig.set_size_inches((7.16, 3.6))
        fig.savefig(os.path.join(output_dir, 'tiempo_total_vs_longitud_ieee_twocol.png'), dpi=300, bbox_inches='tight')
    except Exception:
        pass
    plt.close()
    print('✓ Gráfico guardado: tiempo_total_vs_longitud.png')

    # Además generar versión lineal (ejes lineales) y anotar el último punto (mayor longitud)
    try:
        fig_lin, ax_lin = plt.subplots(figsize=(14, 8))
        for tipo, marker, color in [('dna', 'o', '#2A9D8F'), ('protein', 's', '#E76F51')]:
            subset = df_ordenado[df_ordenado['tipo'] == tipo]
            if len(subset) == 0:
                continue
            if 'total_s' in subset.columns:
                y_vals = subset['total_s']
            else:
                y_vals = subset['total_ms'] / 1000.0
            ax_lin.plot(subset['longitud'], y_vals, marker=marker, linestyle='-',
                        linewidth=2, markersize=6, label=tipo.capitalize(), color=color)

        ax_lin.set_xscale('linear')
        ax_lin.set_yscale('linear')
        ax_lin.set_xlabel('Longitud de Secuencias (caracteres)', fontsize=12, fontweight='bold')
        ax_lin.set_ylabel('Tiempo Total (s)', fontsize=12, fontweight='bold')
        ax_lin.set_title('Tiempo Total vs Longitud (escala lineal)', fontsize=14, fontweight='bold')
        ax_lin.grid(True, alpha=0.3)
        ax_lin.legend(fontsize=11)

        # Seleccionar último punto del DataFrame ordenado (mayor longitud global) y anotarlo
        if len(df_ordenado) > 0:
            last = df_ordenado.iloc[-1]
            lx = last['longitud']
            ly_s = last['total_s'] if 'total_s' in last.index else last['total_ms'] / 1000.0
            # Formato en español y en segundos
            label_last = f"Último: {last['archivo']} = {_format_spanish_number(ly_s, decimals=2)} s"
            ax_lin.scatter([lx], [ly_s], s=150, facecolors='none', edgecolors='black', linewidths=1.5)
            # Anotar con flecha
            ax_lin.annotate(label_last, xy=(lx, ly_s), xytext=(0.6, 0.2), textcoords='axes fraction',
                            arrowprops=dict(arrowstyle='->', color='black'), fontsize=10,
                            bbox=dict(boxstyle='round,pad=0.3', fc='yellow', alpha=0.3))

        # Además: si existe una entrada llamada 'dna_150k', marcarla y anotarla en segundos
        try:
            dna_row = df_ordenado[df_ordenado['archivo'] == 'dna_150k']
            if len(dna_row) == 1:
                dna_r = dna_row.iloc[0]
                dx = dna_r['longitud']
                dy_s = dna_r['total_s'] if 'total_s' in dna_r.index else dna_r['total_ms'] / 1000.0
                # Formato en segundos, redondeado a entero y con notación española
                sec_label = f"{_format_spanish_number(round(dy_s), decimals=0)} segundos"
                label_dna = f"dna_150k = {sec_label}"
                # Dibujar un marcador y una anotación clara (usar color distinto)
                ax_lin.scatter([dx], [dy_s], s=180, facecolors='none', edgecolors='#264653', linewidths=1.8)
                ax_lin.annotate(label_dna, xy=(dx, dy_s), xytext=(0.02, 0.85), textcoords='axes fraction',
                                arrowprops=dict(arrowstyle='->', color='#264653'), fontsize=10,
                                bbox=dict(boxstyle='round,pad=0.3', fc='lightblue', alpha=0.4))
        except Exception:
            # No romper si por alguna razón la fila no existe o faltan columnas
            pass

        plt.tight_layout()
        linfile = os.path.join(output_dir, 'tiempo_total_vs_longitud_linear.png')
        fig_lin.savefig(linfile, dpi=300, bbox_inches='tight')
        try:
            fig_lin.set_size_inches((3.5, 2.4))
            fig_lin.savefig(os.path.join(output_dir, 'tiempo_total_vs_longitud_linear_ieee_onecol.png'), dpi=300, bbox_inches='tight')
            fig_lin.set_size_inches((7.16, 3.6))
            fig_lin.savefig(os.path.join(output_dir, 'tiempo_total_vs_longitud_linear_ieee_twocol.png'), dpi=300, bbox_inches='tight')
        except Exception:
            pass
        plt.close()
        print(f"✓ Gráfico guardado: {os.path.basename(linfile)}")
    except Exception:
        pass

def grafico_porcentajes_separados(df, output_dir):
    """Genera PNGs separados para los porcentajes por fase.
    - gráfico agrupado (barras lado a lado)
    - gráficos individuales por fase (barras ordenadas por longitud)
    """
    df_ordenado = ordenar_por_tipo_y_longitud(df)
    archivos = [row['archivo'] for _, row in df_ordenado.iterrows()]
    x = np.arange(len(df_ordenado))
    width = 0.25

    # Gráfico agrupado (por archivo: init / llenado / traceback)
    fig, ax = plt.subplots(figsize=(16, 8))
    ax.bar(x - width, df_ordenado['pct_init'], width, label='Inicialización', color='#FF6B6B')
    ax.bar(x, df_ordenado['pct_llenado'], width, label='Llenado de Matriz', color='#4ECDC4')
    ax.bar(x + width, df_ordenado['pct_traceback'], width, label='Traceback', color='#45B7D1')
    ax.set_xlabel('Archivo (ordenado por longitud)')
    ax.set_ylabel('Porcentaje (%)')
    ax.set_title('Porcentajes por Fase (agrupado)')
    ax.set_xticks(x)
    ax.set_xticklabels(archivos, rotation=45, ha='right', fontsize=9)
    ax.legend()
    ax.grid(True, alpha=0.3, axis='y')
    plt.tight_layout()
    outpath = os.path.join(output_dir, 'porcentajes_por_fase_separado.png')
    fig.savefig(outpath, dpi=300, bbox_inches='tight')
    # Guardar versiones con proporciones útiles para paper IEEE
    # One-column IEEE ~ 3.5 in width, Two-column IEEE ~ 7.16 in width
    try:
        fig.set_size_inches((3.5, 2.4))
        fig.savefig(os.path.join(output_dir, 'porcentajes_por_fase_separado_ieee_onecol.png'), dpi=300, bbox_inches='tight')
        fig.set_size_inches((7.16, 3.6))
        fig.savefig(os.path.join(output_dir, 'porcentajes_por_fase_separado_ieee_twocol.png'), dpi=300, bbox_inches='tight')
    except Exception:
        # En caso de que set_size_inches no funcione por algún backend, continuar sin fallar
        pass
    # Generar versión nativa vertical (tall) con barras horizontales para que el título quede en el borde corto
    try:
        fig_h, ax_h = plt.subplots(figsize=(3.5, 7.16))
        y = np.arange(len(df_ordenado))
        h = 0.25
        ax_h.barh(y - h, df_ordenado['pct_init'], height=h, label='Inicialización', color='#FF6B6B')
        ax_h.barh(y, df_ordenado['pct_llenado'], height=h, label='Llenado de Matriz', color='#4ECDC4')
        ax_h.barh(y + h, df_ordenado['pct_traceback'], height=h, label='Traceback', color='#45B7D1')
        ax_h.set_yticks(y)
        ax_h.set_yticklabels(archivos, fontsize=8)
        ax_h.invert_yaxis()
        ax_h.set_xlabel('Porcentaje (%)')
        ax_h.set_title('Porcentajes por Fase (agrupado)')
        ax_h.legend()
        ax_h.grid(True, axis='x', alpha=0.3)
        plt.tight_layout()
        fig_h.savefig(os.path.join(output_dir, 'porcentajes_por_fase_separado_ieee_twocol_vert.png'), dpi=300, bbox_inches='tight')
        plt.close(fig_h)
    except Exception:
        pass
    plt.close()
    print(f"✓ Gráfico guardado: porcentajes_por_fase_separado.png")

    # Gráficos individuales por fase (barras)
    fases = [
        ('Inicialización', 'pct_init', '#FF6B6B', 'porcentaje_init_por_archivo.png'),
        ('Llenado de Matriz', 'pct_llenado', '#4ECDC4', 'porcentaje_llenado_por_archivo.png'),
        ('Traceback', 'pct_traceback', '#45B7D1', 'porcentaje_traceback_por_archivo.png')
    ]

    for titulo, columna, color, filename in fases:
        fig, ax = plt.subplots(figsize=(14, 6))
        ax.bar(archivos, df_ordenado[columna], color=color)
        ax.set_xlabel('Archivo (ordenado por longitud)')
        ax.set_ylabel('Porcentaje (%)')
        ax.set_title(f'{titulo} - Porcentaje por Archivo')
        # Asegurar que el número de ticks coincide con las etiquetas para evitar warnings
        ax.set_xticks(range(len(archivos)))
        ax.set_xticklabels(archivos, rotation=45, ha='right', fontsize=9)
        ax.axhline(y=df_ordenado[columna].mean(), color='red', linestyle='--',
                   label=f'Promedio: {df_ordenado[columna].mean():.1f}%')
        ax.legend()
        ax.grid(True, alpha=0.3, axis='y')
        plt.tight_layout()
        outfile = os.path.join(output_dir, filename)
        fig.savefig(outfile, dpi=300, bbox_inches='tight')
        # Guardar versiones IEEE (tamaños compactos adecuados para una columna y dos columnas)
        try:
            # One-column compact
            fig.set_size_inches((3.5, 2.2))
            fig.savefig(os.path.join(output_dir, filename.replace('.png', '_ieee_onecol.png')), dpi=300, bbox_inches='tight')
            # Two-column horizontal
            fig.set_size_inches((7.16, 2.8))
            fig.savefig(os.path.join(output_dir, filename.replace('.png', '_ieee_twocol.png')), dpi=300, bbox_inches='tight')
            # Two-column vertical (tall) - en vez de rotar la imagen, generar una versión nativa horizontal (barh)
            try:
                fig_h, ax_h = plt.subplots(figsize=(3.5, 7.16))
                y = np.arange(len(df_ordenado))
                ax_h.barh(y, df_ordenado[columna], color=color)
                ax_h.set_yticks(y)
                ax_h.set_yticklabels(archivos, fontsize=8)
                ax_h.invert_yaxis()
                ax_h.set_xlabel('Porcentaje (%)')
                ax_h.set_title(f'{titulo} - Porcentaje por Archivo')
                ax_h.axvline(x=df_ordenado[columna].mean(), color='red', linestyle='--',
                              label=f'Promedio: {df_ordenado[columna].mean():.1f}%')
                ax_h.legend()
                ax_h.grid(True, axis='x', alpha=0.3)
                plt.tight_layout()
                fig_h.savefig(os.path.join(output_dir, filename.replace('.png', '_ieee_twocol_vert.png')), dpi=300, bbox_inches='tight')
                plt.close(fig_h)
            except Exception:
                pass
        except Exception:
            pass
        plt.close()
        print(f"✓ Gráfico guardado: {filename}")

def grafico_llenado_detalle(df, output_dir):
    """Genera imágenes enfocadas en el porcentaje de tiempo en llenado de matriz:
    - barra por archivo con línea de promedio
    - distribución (histograma + boxplot)
    """
    df_ordenado = ordenar_por_tipo_y_longitud(df)

    # Barra con promedio (más enfocada que la anterior)
    fig, ax = plt.subplots(figsize=(14, 6))
    archivos = [row['archivo'] for _, row in df_ordenado.iterrows()]
    ax.bar(archivos, df_ordenado['pct_llenado'], color='#4ECDC4')
    ax.set_xlabel('Archivo (ordenado por longitud)')
    ax.set_ylabel('% Llenado')
    ax.set_title('Porcentaje de Tiempo en Llenado de Matriz por Archivo')
    # Evitar warning asegurando ticks antes de asignar etiquetas
    ax.set_xticks(range(len(archivos)))
    ax.set_xticklabels(archivos, rotation=45, ha='right', fontsize=9)
    mean_val = df_ordenado['pct_llenado'].mean()
    ax.axhline(y=mean_val, color='red', linestyle='--', linewidth=1.5,
               label=f'Promedio: {mean_val:.2f}%')
    ax.legend()
    ax.grid(True, alpha=0.3, axis='y')
    plt.tight_layout()
    mainfile = os.path.join(output_dir, 'porcentaje_llenado_por_archivo_promedio.png')
    fig.savefig(mainfile, dpi=300, bbox_inches='tight')
    try:
        fig.set_size_inches((3.5, 2.2))
        fig.savefig(os.path.join(output_dir, 'porcentaje_llenado_por_archivo_promedio_ieee_onecol.png'), dpi=300, bbox_inches='tight')
        fig.set_size_inches((7.16, 2.8))
        fig.savefig(os.path.join(output_dir, 'porcentaje_llenado_por_archivo_promedio_ieee_twocol.png'), dpi=300, bbox_inches='tight')
        # Variante vertical para two-column (tall)
        fig.set_size_inches((3.5, 7.16))
        fig.savefig(os.path.join(output_dir, 'porcentaje_llenado_por_archivo_promedio_ieee_twocol_vert.png'), dpi=300, bbox_inches='tight')
        # Rotaciones disponibles si Pillow está instalado
        if Image is not None:
            try:
                path_vert = os.path.join(output_dir, 'porcentaje_llenado_por_archivo_promedio_ieee_twocol_vert.png')
                img = Image.open(path_vert)
                img.rotate(90, expand=True).save(os.path.join(output_dir, 'porcentaje_llenado_por_archivo_promedio_ieee_twocol_vert_rot_ccw.png'))
                img.rotate(-90, expand=True).save(os.path.join(output_dir, 'porcentaje_llenado_por_archivo_promedio_ieee_twocol_vert_rot_cw.png'))
            except Exception:
                pass
    except Exception:
        pass
    plt.close()
    print(f"✓ Gráfico guardado: porcentaje_llenado_por_archivo_promedio.png")

    # Distribución: histograma + boxplot
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6), gridspec_kw={'width_ratios': [3, 1]})
    sns.histplot(df_ordenado['pct_llenado'], bins=10, kde=True, ax=ax1, color='#4ECDC4')
    ax1.set_title('Distribución del % Llenado (histograma)')
    ax1.set_xlabel('% Llenado')
    ax1.grid(True, alpha=0.3)

    sns.boxplot(y=df_ordenado['pct_llenado'], ax=ax2, color='#4ECDC4')
    ax2.set_title('Boxplot % Llenado')
    ax2.set_ylabel('')
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    distrib_file = os.path.join(output_dir, 'porcentaje_llenado_distribucion.png')
    plt.savefig(distrib_file, dpi=300, bbox_inches='tight')
    try:
        # Guardar versión compacta (one-column) adecuada para figura en paper
        fig.set_size_inches((3.5, 2.4))
        plt.savefig(os.path.join(output_dir, 'porcentaje_llenado_distribucion_ieee_onecol.png'), dpi=300, bbox_inches='tight')
        # Añadir variante vertical two-column si interesa (tall)
        fig.set_size_inches((3.5, 7.16))
        plt.savefig(os.path.join(output_dir, 'porcentaje_llenado_distribucion_ieee_twocol_vert.png'), dpi=300, bbox_inches='tight')
        # Rotaciones (CCW/CW) para la variante vertical si Pillow está disponible
        if Image is not None:
            try:
                path_vert = os.path.join(output_dir, 'porcentaje_llenado_distribucion_ieee_twocol_vert.png')
                img = Image.open(path_vert)
                img.rotate(90, expand=True).save(os.path.join(output_dir, 'porcentaje_llenado_distribucion_ieee_twocol_vert_rot_ccw.png'))
                img.rotate(-90, expand=True).save(os.path.join(output_dir, 'porcentaje_llenado_distribucion_ieee_twocol_vert_rot_cw.png'))
            except Exception:
                pass
    except Exception:
        pass
    plt.close()
    print(f"✓ Gráfico guardado: porcentaje_llenado_distribucion.png")

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
        for _, row in ordenar_por_tipo_y_longitud(df).iterrows():
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
    grafico_tiempo_total_por_longitud(df, args.output)
    # Gráficos adicionales y PNGs separados para análisis comparativo
    grafico_porcentajes_separados(df, args.output)
    grafico_llenado_detalle(df, args.output)
    
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
