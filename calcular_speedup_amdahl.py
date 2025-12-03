#!/usr/bin/env python3
"""
Script educativo para calcular speedup y ley de Amdahl
Explica paso a paso cómo se calculan estos valores
"""

import pandas as pd
import sys
import numpy as np
import matplotlib.pyplot as plt

def calcular_speedup(tiempo_secuencial, tiempo_paralelo):
    """
    Calcula el speedup simple
    
    Args:
        tiempo_secuencial: Tiempo en ms del algoritmo secuencial
        tiempo_paralelo: Tiempo en ms del algoritmo paralelo
    
    Returns:
        speedup: Factor de mejora
    """
    if tiempo_paralelo == 0:
        return float('inf')
    return tiempo_secuencial / tiempo_paralelo

def calcular_ley_amdahl(p, n):
    """
    Calcula el speedup teórico según la ley de Amdahl
    
    Args:
        p: Fracción paralelizable (0 a 1). Ej: 0.71 = 71%
        n: Número de threads/processors
    
    Returns:
        speedup: Speedup teórico máximo
    """
    if n == 0:
        return 1.0
    return 1.0 / ((1.0 - p) + (p / n))

def calcular_fraccion_paralelizable(df_secuencial):
    """
    Calcula qué fracción del tiempo es paralelizable
    basándose en los tiempos de las fases
    
    Args:
        df_secuencial: DataFrame con resultados secuenciales
    
    Returns:
        p: Fracción paralelizable (0 a 1)
    """
    # Fase 2 (llenado) es la paralelizable
    tiempo_total = df_secuencial['tiempo_total_ms'].mean()
    tiempo_llenado = df_secuencial['tiempo_llenado_ms'].mean()
    
    p = tiempo_llenado / tiempo_total
    return p

def analizar_resultados(archivo_csv):
    """
    Analiza resultados y calcula speedup y ley de Amdahl
    """
    print("=" * 80)
    print("CALCULO DE SPEEDUP Y LEY DE AMDahl - GUIA EDUCATIVA")
    print("=" * 80)
    print()
    
    # Cargar datos
    df = pd.read_csv(archivo_csv)
    
    # Separar secuencial y paralelo
    df_sec = df[df['metodo'] == 'secuencial'].copy()
    df_par = df[df['metodo'] != 'secuencial'].copy()
    
    if len(df_sec) == 0:
        print("ERROR: No hay datos secuenciales")
        return
    
    if len(df_par) == 0:
        print("ERROR: No hay datos paralelos")
        return
    
    # ============================================================
    # PASO 1: Calcular fracción paralelizable (p)
    # ============================================================
    print("PASO 1: CALCULAR FRACCION PARALELIZABLE (p)")
    print("-" * 80)
    print()
    
    tiempo_total_sec = df_sec['tiempo_total_ms'].mean()
    tiempo_init_sec = df_sec['tiempo_init_ms'].mean()
    tiempo_llenado_sec = df_sec['tiempo_llenado_ms'].mean()
    tiempo_traceback_sec = df_sec['tiempo_traceback_ms'].mean()
    
    print(f"Tiempos secuenciales (promedio):")
    print(f"  Fase 1 (Init):      {tiempo_init_sec:.2f} ms ({tiempo_init_sec/tiempo_total_sec*100:.1f}%)")
    print(f"  Fase 2 (Llenado):  {tiempo_llenado_sec:.2f} ms ({tiempo_llenado_sec/tiempo_total_sec*100:.1f}%)")
    print(f"  Fase 3 (Traceback): {tiempo_traceback_sec:.2f} ms ({tiempo_traceback_sec/tiempo_total_sec*100:.1f}%)")
    print(f"  TOTAL:              {tiempo_total_sec:.2f} ms (100%)")
    print()
    
    # Solo la Fase 2 es paralelizable
    p = tiempo_llenado_sec / tiempo_total_sec
    print(f"Fraccion paralelizable (p) = Tiempo_Fase2 / Tiempo_Total")
    print(f"  p = {tiempo_llenado_sec:.2f} / {tiempo_total_sec:.2f}")
    print(f"  p = {p:.4f} = {p*100:.2f}%")
    print()
    print(f"Interpretacion:")
    print(f"  - {p*100:.1f}% del tiempo es paralelizable (Fase 2)")
    print(f"  - {100-p*100:.1f}% del tiempo NO es paralelizable (Fase 1 + Fase 3)")
    print()
    
    # ============================================================
    # PASO 2: Calcular speedup real
    # ============================================================
    print("=" * 80)
    print("PASO 2: CALCULAR SPEEDUP REAL")
    print("-" * 80)
    print()
    
    # Agrupar por threads y encontrar mejor schedule
    for threads in sorted(df_par['threads'].unique()):
        df_par_threads = df_par[df_par['threads'] == threads].copy()
        
        # Mejor tiempo (mejor schedule)
        mejor_tiempo = df_par_threads['tiempo_total_ms'].mean()
        mejor_schedule = df_par_threads.groupby('schedule')['tiempo_total_ms'].mean().idxmin()
        
        speedup_real = calcular_speedup(tiempo_total_sec, mejor_tiempo)
        eficiencia = (speedup_real / threads) * 100
        
        print(f"Con {threads} threads (mejor schedule: {mejor_schedule}):")
        print(f"  Tiempo secuencial: {tiempo_total_sec:.2f} ms")
        print(f"  Tiempo paralelo:   {mejor_tiempo:.2f} ms")
        print(f"  Speedup = {tiempo_total_sec:.2f} / {mejor_tiempo:.2f} = {speedup_real:.3f}x")
        print(f"  Eficiencia = {speedup_real:.3f} / {threads} = {eficiencia:.1f}%")
        print()
    
    # ============================================================
    # PASO 3: Calcular speedup teórico (Ley de Amdahl)
    # ============================================================
    print("=" * 80)
    print("PASO 3: CALCULAR SPEEDUP TEORICO (LEY DE AMDahl)")
    print("-" * 80)
    print()
    
    print(f"Formula de Amdahl:")
    print(f"  Speedup = 1 / ((1 - p) + (p / n))")
    print(f"  Donde:")
    print(f"    p = fraccion paralelizable = {p:.4f}")
    print(f"    n = numero de threads")
    print()
    
    for threads in sorted(df_par['threads'].unique()):
        speedup_teorico = calcular_ley_amdahl(p, threads)
        
        # Speedup real para comparar
        df_par_threads = df_par[df_par['threads'] == threads].copy()
        mejor_tiempo = df_par_threads.groupby('schedule')['tiempo_total_ms'].mean().min()
        speedup_real = calcular_speedup(tiempo_total_sec, mejor_tiempo)
        
        print(f"Con {threads} threads:")
        print(f"  Speedup teorico (Amdahl): {speedup_teorico:.3f}x")
        print(f"  Speedup real:              {speedup_real:.3f}x")
        print(f"  Diferencia:                {speedup_real - speedup_teorico:+.3f}x")
        print(f"  Eficiencia vs teorico:     {speedup_real / speedup_teorico * 100:.1f}%")
        print()
    
    # ============================================================
    # PASO 4: Límite teórico máximo
    # ============================================================
    print("=" * 80)
    print("PASO 4: LIMITE TEORICO MAXIMO")
    print("-" * 80)
    print()
    
    # Con infinitos threads, el speedup máximo es:
    speedup_max = 1 / (1 - p)
    
    print(f"Si usaras infinitos threads, el speedup maximo seria:")
    print(f"  Speedup_max = 1 / (1 - p)")
    print(f"  Speedup_max = 1 / (1 - {p:.4f})")
    print(f"  Speedup_max = 1 / {1-p:.4f}")
    print(f"  Speedup_max = {speedup_max:.3f}x")
    print()
    print(f"Interpretacion:")
    print(f"  - NUNCA podras superar {speedup_max:.2f}x de speedup")
    print(f"  - Esto es porque {100-p*100:.1f}% del codigo NO es paralelizable")
    print(f"  - Esta es la LEY DE AMDahl: las partes secuenciales limitan el speedup")
    print()
    
    # ============================================================
    # PASO 5: Gráfico comparativo
    # ============================================================
    print("=" * 80)
    print("PASO 5: GENERANDO GRAFICO COMPARATIVO")
    print("-" * 80)
    print()
    
    threads_vals = sorted(df_par['threads'].unique())
    speedup_real_vals = []
    speedup_teorico_vals = []
    
    for n in threads_vals:
        # Real
        df_par_n = df_par[df_par['threads'] == n]
        mejor_tiempo = df_par_n.groupby('schedule')['tiempo_total_ms'].mean().min()
        speedup_real_vals.append(calcular_speedup(tiempo_total_sec, mejor_tiempo))
        
        # Teórico
        speedup_teorico_vals.append(calcular_ley_amdahl(p, n))
    
    # Crear gráfico
    fig, ax = plt.subplots(figsize=(10, 6))
    
    ax.plot(threads_vals, speedup_real_vals, 'o-', linewidth=2, markersize=8, 
            label='Speedup Real', color='#2A9D8F')
    ax.plot(threads_vals, speedup_teorico_vals, 's--', linewidth=2, markersize=8,
            label=f'Ley de Amdahl (p={p:.2%})', color='#E74C3C')
    ax.plot([1, max(threads_vals)], [1, max(threads_vals)], 'k:', linewidth=1.5,
            label='Speedup Ideal', alpha=0.5)
    ax.axhline(y=speedup_max, color='gray', linestyle='--', linewidth=1.5,
               label=f'Límite máximo ({speedup_max:.2f}x)', alpha=0.7)
    
    ax.set_xlabel('Número de Threads', fontsize=12, fontweight='bold')
    ax.set_ylabel('Speedup', fontsize=12, fontweight='bold')
    ax.set_title('Speedup Real vs Teórico (Ley de Amdahl)', fontsize=14, fontweight='bold')
    ax.legend(fontsize=10)
    ax.grid(True, alpha=0.3)
    ax.set_xlim(left=0.5, right=max(threads_vals) + 0.5)
    ax.set_ylim(bottom=0)
    
    plt.tight_layout()
    plt.savefig('speedup_amdahl_educativo.png', dpi=300, bbox_inches='tight')
    print("[OK] Grafico guardado: speedup_amdahl_educativo.png")
    print()
    
    # ============================================================
    # RESUMEN
    # ============================================================
    print("=" * 80)
    print("RESUMEN")
    print("=" * 80)
    print()
    print(f"1. Fraccion paralelizable: {p:.2%}")
    print(f"2. Speedup maximo teorico: {speedup_max:.2f}x")
    print(f"3. Speedup real ({max(threads_vals)} threads): {speedup_real_vals[-1]:.2f}x")
    print(f"4. Eficiencia: {speedup_real_vals[-1] / speedup_teorico_vals[-1] * 100:.1f}% del teorico")
    print()
    print("CONCLUSION:")
    if speedup_real_vals[-1] / speedup_teorico_vals[-1] > 0.8:
        print("  [OK] Excelente rendimiento! Estas cerca del limite teorico de Amdahl")
    elif speedup_real_vals[-1] / speedup_teorico_vals[-1] > 0.6:
        print("  [OK] Buen rendimiento, pero hay espacio para mejorar")
    else:
        print("  [!] Rendimiento bajo. Revisa overhead, conflictos de cache, etc.")
    print()

if __name__ == '__main__':
    archivo = sys.argv[1] if len(sys.argv) > 1 else 'resultados/resultados_grandes_4threads.csv'
    analizar_resultados(archivo)

