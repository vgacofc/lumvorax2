import os
import subprocess
import time

def run_all_nx():
    nx_versions = [
        ("NX-11", "src/nx_versions/nx11_logger_engine.py", "python"),
        ("NX-12", "src/nx_versions/run_nx12_execution.py", "python"),
        ("NX-21", "src/nx_versions/nx21_engine.cpp", "cpp"),
        ("NX-32", "src/nx_versions/nx32_ultimate_engine.cpp", "cpp"),
        ("NX-41", "src/nx_versions/nx41_lebesgue_engine.cpp", "cpp")
    ]
    
    global_progress = 0
    total_versions = 41
    
    for nx_id, path, lang in nx_versions:
        print(f"Executing {nx_id}...")
        log_dir = f"logs_AIMO3/nx/{nx_id}"
        os.makedirs(log_dir, exist_ok=True)
        
        if lang == "cpp":
            bin_name = f"./{nx_id}_bin"
            subprocess.run(["g++", "-O3", path, "-o", bin_name])
            with open(f"{log_dir}/execution_{int(time.time())}.log", "w") as f:
                subprocess.run([bin_name], stdout=f, stderr=subprocess.STDOUT)
        elif lang == "python":
            with open(f"{log_dir}/execution_{int(time.time())}.log", "w") as f:
                subprocess.run(["python", path], stdout=f, stderr=subprocess.STDOUT)
        
        global_progress += (1 / total_versions) * 100
        print(f"Progress: {nx_id} concluded. Global: {global_progress:.2f}%")

if __name__ == "__main__":
    run_all_nx()
