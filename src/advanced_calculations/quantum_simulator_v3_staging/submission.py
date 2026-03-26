import os
import subprocess

def run_command(cmd):
    print(f"Executing: {cmd}")
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    if stdout: print(stdout.decode())
    if stderr: print(stderr.decode())
    return process.returncode

print("=== KAGGLE AUTONOMOUS VALIDATION V3 ===")
# 1. Compilation
res = run_command("gcc -I. -O3 -mavx2 quantum_simulator.c quantum_simulator_fusion_v3.c memory_tracker.c -o quantum_v3_bin -lm -lpthread")
if res == 0:
    print("✅ COMPILATION_SUCCESS")
    # 2. Exécution du benchmark
    run_command("./quantum_v3_bin --benchmark --qubits 24")
else:
    print("❌ COMPILATION_FAILED")
