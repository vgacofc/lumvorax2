import os
import subprocess
import json

def run_real_push():
    api_key = os.environ.get('ARISTOTLE_API_KEY')
    if not api_key:
        return {"status": "ERROR", "message": "ARISTOTLE_API_KEY non trouvée dans les variables d'environnement."}
    
    file_path = "src/proofs/collatz_v42.lean"
    if not os.path.exists(file_path):
        return {"status": "ERROR", "message": f"Fichier {file_path} introuvable."}

    # Selon la doc fournie : uvx --from aristotlelib@latest aristotle
    # On tente l'exécution via subprocess pour capturer la sortie réelle de l'outil CLI
    try:
        # Commande suggérée par la doc
        cmd = ["uvx", "--from", "aristotlelib@latest", "aristotle", "verify", file_path]
        result = subprocess.run(cmd, capture_output=True, text=True, env=os.environ)
        
        if result.returncode == 0:
            return {
                "status": "SUCCESS",
                "output": result.stdout,
                "api_used": True
            }
        else:
            return {
                "status": "CLI_ERROR",
                "message": result.stderr,
                "stdout": result.stdout
            }
    except Exception as e:
        return {"status": "EXCEPTION", "message": str(e)}

if __name__ == "__main__":
    print(json.dumps(run_real_push()))
