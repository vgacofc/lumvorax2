import os
import sys
import json
import time
from datetime import datetime

def audit_kaggle_environment():
    log_file = f"logs/forensic/audit_v10_{int(time.time())}.log"
    os.makedirs("logs/forensic", exist_ok=True)
    
    with open(log_file, "w") as f:
        f.write(f"--- AUDIT RÉEL NX47 - VERSION 10 - {datetime.now().isoformat()} ---\n")
        f.write("REMPLACEMENT DU CODE SIMULÉ PAR UNE STRUCTURE D'AUDIT STRICTE.\n\n")
        
        # 1. Liste des datasets
        f.write("1. STRUCTURE /kaggle/input/ :\n")
        input_dir = "/kaggle/input"
        if os.path.exists(input_dir):
            for root, dirs, files in os.walk(input_dir):
                level = root.replace(input_dir, '').count(os.sep)
                indent = ' ' * 4 * (level)
                f.write(f"{indent}[{os.path.basename(root)}/]\n")
                subindent = ' ' * 4 * (level + 1)
                for file in files[:10]: # Limite pour le log
                    f.write(f"{subindent}{file}\n")
        else:
            f.write("ERREUR : /kaggle/input non trouvé. Environnement hors-Kaggle détecté.\n")
            print("CRITICAL: /kaggle/input not found. This script must run in a Kaggle environment.")
            # sys.exit(1) # Commenté pour permettre l'exécution Replit mais logué comme échec
            
        f.write("\n--- FIN DE L'AUDIT INITIAL ---\n")
    
    print(f"Log d'audit généré : {log_file}")

if __name__ == "__main__":
    audit_kaggle_environment()
