#!/usr/bin/env python3
"""
Script para promediar los resultados del benchmark agrupando por repeticiones.
Agrupa por: archivo_fasta, metodo, threads, schedule
"""

import pandas as pd
import sys
import argparse

def promediar_resultados(archivo_entrada, archivo_salida=None):
    """
    Promedia los resultados agrupando por archivo_fasta, metodo, threads, schedule.
    
    Args:
        archivo_entrada: Ruta al archivo CSV con todos los resultados
        archivo_salida: Ruta donde guardar el CSV promediado (opcional)
    """
    print(f"Leyendo datos de {archivo_entrada}...")
    # Leer con manejo robusto de campos con comas
    try:
        # Intentar leer normalmente primero
        df = pd.read_csv(archivo_entrada, quotechar='"', escapechar='\\')
    except Exception as e:
        print(f"Error al leer CSV: {e}")
        print("Intentando limpieza manual...")
        # Leer línea por línea y corregir
        import csv
        rows = []
        with open(archivo_entrada, 'r', encoding='utf-8') as f:
            reader = csv.reader(f)
            header = next(reader)
            expected_cols = len(header)
            schedule_idx = header.index('schedule') if 'schedule' in header else -1
            
            for i, row in enumerate(reader, start=2):
                if len(row) != expected_cols:
                    # Si tiene un campo extra, probablemente schedule tiene coma
                    if len(row) == expected_cols + 1 and schedule_idx >= 0:
                        # Combinar schedule y el siguiente campo si es un número
                        if schedule_idx < len(row) - 1:
                            schedule_val = row[schedule_idx]
                            next_val = row[schedule_idx + 1]
                            # Si el siguiente campo es un número, combinarlo con schedule
                            if schedule_val in ['static', 'dynamic', 'guided'] and (next_val.isdigit() or next_val == '1'):
                                row[schedule_idx] = f"{schedule_val},{next_val}"
                                row.pop(schedule_idx + 1)
                            # Si no, podría ser que longitud_A se duplicó
                            elif schedule_idx + 1 < len(row) and row[schedule_idx + 1] == row[schedule_idx + 2]:
                                row.pop(schedule_idx + 1)
                
                if len(row) == expected_cols:
                    rows.append(row)
                elif len(row) > 0:
                    print(f"Línea {i} ignorada: tiene {len(row)} campos (esperados {expected_cols})")
        
        if rows:
            df = pd.DataFrame(rows, columns=header)
        else:
            raise ValueError("No se pudieron leer datos válidos del archivo")
    
    print(f"Total de filas: {len(df)}")
    print(f"Columnas: {list(df.columns)}")
    
    # Columnas numéricas a promediar
    columnas_numericas = [
        'tiempo_init_ms', 'tiempo_llenado_ms', 'tiempo_traceback_ms',
        'tiempo_total_ms', 'puntuacion'
    ]
    
    # Convertir columnas numéricas a float (manejar errores)
    for col in columnas_numericas:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')
    
    # Convertir también columnas de agrupación que deberían ser numéricas
    for col in ['threads', 'longitud_A', 'longitud_B', 'match', 'mismatch', 'gap', 'repeticion']:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')
    
    # Columnas de agrupación (mantener valores únicos)
    columnas_grupo = [
        'archivo_fasta', 'metodo', 'threads', 'schedule',
        'longitud_A', 'longitud_B', 'match', 'mismatch', 'gap'
    ]
    
    # Verificar que las columnas existen
    columnas_faltantes = [c for c in columnas_grupo + columnas_numericas if c not in df.columns]
    if columnas_faltantes:
        print(f"Advertencia: Columnas faltantes: {columnas_faltantes}")
        columnas_grupo = [c for c in columnas_grupo if c in df.columns]
        columnas_numericas = [c for c in columnas_numericas if c in df.columns]
    
    print(f"\nAgrupando por: {columnas_grupo}")
    print(f"Promediando: {columnas_numericas}")
    
    # Agrupar y promediar
    df_promedio = df.groupby(columnas_grupo, as_index=False)[columnas_numericas].mean()
    
    # Agregar columna con número de repeticiones
    conteo = df.groupby(columnas_grupo, as_index=False).size()
    conteo.rename(columns={'size': 'num_repeticiones'}, inplace=True)
    df_promedio = df_promedio.merge(conteo, on=columnas_grupo)
    
    # Reordenar columnas para que num_repeticiones esté después de las columnas de grupo
    columnas_ordenadas = columnas_grupo + ['num_repeticiones'] + columnas_numericas
    df_promedio = df_promedio[columnas_ordenadas]
    
    print(f"\nFilas después de promediar: {len(df_promedio)}")
    print(f"Reducción: {len(df) - len(df_promedio)} filas eliminadas")
    
    # Guardar resultados
    if archivo_salida is None:
        archivo_salida = archivo_entrada.replace('.csv', '_promedio.csv')
    
    df_promedio.to_csv(archivo_salida, index=False)
    print(f"\nResultados promediados guardados en: {archivo_salida}")
    
    # Mostrar estadísticas
    print("\n=== ESTADÍSTICAS ===")
    print(f"Archivos únicos: {df_promedio['archivo_fasta'].nunique()}")
    print(f"Métodos únicos: {df_promedio['metodo'].unique()}")
    print(f"Configuraciones únicas: {df_promedio.groupby(['metodo', 'threads', 'schedule']).size().shape[0]}")
    print(f"\nNúmero de repeticiones por grupo:")
    print(df_promedio['num_repeticiones'].value_counts().sort_index())
    
    return df_promedio

def main():
    parser = argparse.ArgumentParser(
        description='Promedia resultados del benchmark agrupando por repeticiones'
    )
    parser.add_argument(
        '-i', '--input',
        default='resultados/resultados_completo.csv',
        help='Archivo CSV de entrada (default: resultados/resultados_completo.csv)'
    )
    parser.add_argument(
        '-o', '--output',
        default=None,
        help='Archivo CSV de salida (default: input_promedio.csv)'
    )
    
    args = parser.parse_args()
    
    try:
        promediar_resultados(args.input, args.output)
    except FileNotFoundError as e:
        print(f"Error: Archivo no encontrado: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    main()

