#!/usr/bin/env python3
"""Comparar tiempos secuenciales vs paralelos"""

import pandas as pd
import sys

archivo = sys.argv[1] if len(sys.argv) > 1 else 'resultados/resultados_grandes.csv'

df = pd.read_csv(archivo)
df['archivo_fasta'] = df['archivo_fasta'].str.replace('\\', '/')

print("=" * 80)
print("COMPARACIÓN SECUENCIAL vs PARALELO")
print("=" * 80)
print()

# Separar secuencial y paralelo
df_sec = df[df['metodo'] == 'secuencial'].copy()
df_par = df[df['metodo'] != 'secuencial'].copy()

if len(df_sec) == 0:
    print("⚠️  No se encontraron resultados secuenciales")
    print("   Ejecuta primero el secuencial o usa un archivo que lo contenga")
    sys.exit(1)

print("1. TIEMPOS SECUENCIALES (baseline):")
print("-" * 80)
for long in sorted(df_sec['longitud_A'].unique()):
    subset = df_sec[df_sec['longitud_A'] == long]
    tiempo = subset['tiempo_total_ms'].mean()
    std = subset['tiempo_total_ms'].std()
    print(f"  Longitud {long}: {tiempo:.2f} ± {std:.2f} ms (n={len(subset)})")
print()

print("2. TIEMPOS PARALELOS (mejor schedule por archivo):")
print("-" * 80)

for archivo_fasta in df_par['archivo_fasta'].unique():
    subset_par = df_par[df_par['archivo_fasta'] == archivo_fasta]
    
    # Obtener longitud (puede estar incorrecta, usar la del secuencial)
    long_par = subset_par['longitud_A'].iloc[0]
    
    # Buscar secuencial correspondiente
    subset_sec = df_sec[df_sec['archivo_fasta'] == archivo_fasta]
    if len(subset_sec) == 0:
        # Intentar por longitud
        subset_sec = df_sec[df_sec['longitud_A'] == long_par]
    
    if len(subset_sec) == 0:
        print(f"  ⚠️  {archivo_fasta}: No se encontró secuencial correspondiente")
        continue
    
    tiempo_sec = subset_sec['tiempo_total_ms'].mean()
    long_sec = subset_sec['longitud_A'].iloc[0]
    
    # Mejor schedule
    mejor_schedule = subset_par.groupby('schedule')['tiempo_total_ms'].mean().idxmin()
    mejor_tiempo = subset_par.groupby('schedule')['tiempo_total_ms'].mean().min()
    mejor_std = subset_par[subset_par['schedule'] == mejor_schedule]['tiempo_total_ms'].std()
    
    speedup = tiempo_sec / mejor_tiempo
    eficiencia = speedup / subset_par['threads'].iloc[0] * 100
    
    print(f"\n  {archivo_fasta}:")
    print(f"    Longitud: {long_sec}")
    print(f"    Secuencial: {tiempo_sec:.2f} ms")
    print(f"    Paralelo ({mejor_schedule}): {mejor_tiempo:.2f} ± {mejor_std:.2f} ms")
    print(f"    Speedup: {speedup:.3f}x")
    print(f"    Eficiencia: {eficiencia:.1f}%")
    
    if mejor_tiempo > tiempo_sec:
        print(f"    ⚠️  PROBLEMA: Paralelo es {mejor_tiempo/tiempo_sec:.2f}x MÁS LENTO")
    else:
        print(f"    ✓ Paralelo es {tiempo_sec/mejor_tiempo:.2f}x MÁS RÁPIDO")
print()

print("3. TOP 5 SCHEDULES POR ARCHIVO:")
print("-" * 80)
for archivo_fasta in df_par['archivo_fasta'].unique():
    subset_par = df_par[df_par['archivo_fasta'] == archivo_fasta]
    top5 = subset_par.groupby('schedule')['tiempo_total_ms'].mean().nsmallest(5)
    
    print(f"\n  {archivo_fasta}:")
    for i, (schedule, tiempo) in enumerate(top5.items(), 1):
        print(f"    {i}. {schedule}: {tiempo:.2f} ms")
print()

print("4. ANÁLISIS DE OVERHEAD:")
print("-" * 80)
for archivo_fasta in df_par['archivo_fasta'].unique():
    subset_par = df_par[df_par['archivo_fasta'] == archivo_fasta]
    subset_sec = df_sec[df_sec['archivo_fasta'] == archivo_fasta]
    
    if len(subset_sec) == 0:
        continue
    
    mejor_schedule = subset_par.groupby('schedule')['tiempo_total_ms'].mean().idxmin()
    mejor_subset = subset_par[subset_par['schedule'] == mejor_schedule]
    
    print(f"\n  {archivo_fasta} ({mejor_schedule}):")
    print(f"    Init: {mejor_subset['tiempo_init_ms'].mean():.2f} ms")
    print(f"    Llenado: {mejor_subset['tiempo_llenado_ms'].mean():.2f} ms")
    print(f"    Traceback: {mejor_subset['tiempo_traceback_ms'].mean():.2f} ms")
    
    sec_llenado = subset_sec['tiempo_llenado_ms'].mean()
    par_llenado = mejor_subset['tiempo_llenado_ms'].mean()
    print(f"    Llenado secuencial: {sec_llenado:.2f} ms")
    print(f"    Speedup en llenado: {sec_llenado/par_llenado:.3f}x")

