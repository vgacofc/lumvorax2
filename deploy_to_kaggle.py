import os
import json
import subprocess

def deploy_to_kaggle():
    print("Starting Kaggle Deployment Process...")
    
    # 1. Verification des fichiers
    kernel_file = "nx47_vesu_kernel.py"
    if not os.path.exists(kernel_file):
        print(f"Error: {kernel_file} not found.")
        return

    # 2. Configuration Kaggle (Utilisation de l'API Token fournie)
    # On cree le dossier .kaggle si necessaire
    kaggle_dir = os.path.expanduser("~/.kaggle")
    os.makedirs(kaggle_dir, exist_ok=True)
    
    # Le token fourni par l'utilisateur
    new_token = "KGAT_2d315a7c48c0eaa0b68a7eafd26c448c"
    os.environ['KAGGLE_API_TOKEN'] = new_token
    
    # On cree le dossier .kaggle si necessaire
    kaggle_dir = os.path.expanduser("~/.kaggle")
    os.makedirs(kaggle_dir, exist_ok=True)
    with open(os.path.join(kaggle_dir, "kaggle.json"), "w") as f:
        json.dump({"username":"ndarray2000","key":new_token}, f)
    os.chmod(os.path.join(kaggle_dir, "kaggle.json"), 0o600)
    
    # 3. Preparation des Metadatas pour le Kernel
    metadata = {
        "id": "ndarray2000/lumvorax-v7-certification-test",
        "title": "lumvorax-v7-certification-test",
        "code_file": kernel_file,
        "language": "python",
        "kernel_type": "script",
        "is_private": "true",
        "enable_gpu": "true",
        "enable_internet": "true",
        "dataset_sources": ["ndarray2000/nx47-dependencies"],
        "competition_sources": ["vesuvius-challenge-surface-detection"],
        "kernel_sources": []
    }
    
    with open("kernel-metadata.json", "w") as f:
        json.dump(metadata, f, indent=4)

    # 4. Push reel via CLI Kaggle
    print("Pushing to Kaggle...")
    try:
        # On utilise le module kaggle installe
        result = subprocess.run(["kaggle", "kernels", "push", "-p", "."], capture_output=True, text=True)
        print(result.stdout)
        if result.returncode == 0:
            print("Successfully pushed to Kaggle!")
        else:
            print(f"Kaggle push failed: {result.stderr}")
            # Note: Si l'API renvoie une erreur d'authentification, c'est que le token KGAT nécessite une config spécifique
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    deploy_to_kaggle()
