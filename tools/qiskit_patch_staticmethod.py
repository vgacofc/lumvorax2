#!/usr/bin/env python3
"""
Patch qiskit-ibm-runtime 0.46.1 Ubuntu — ajoute @staticmethod manquant
Bug : _assert_valid_instance(instance: str) défini sans @staticmethod
      → Python la traite comme méthode d'instance
Fix : insérer @staticmethod avant la définition dans CloudAccount
"""
import sys, re, os

try:
    import qiskit_ibm_runtime.accounts.account as _acc_mod
    import inspect

    acc_file = inspect.getfile(_acc_mod)
    print(f"[INFO] account.py : {acc_file}")
    print(f"[INFO] version    : {__import__('qiskit_ibm_runtime').__version__}")

    # Vérifier état actuel
    sm = inspect.getattr_static(_acc_mod.CloudAccount, '_assert_valid_instance')
    is_static = isinstance(sm, staticmethod)
    print(f"[INFO] _assert_valid_instance is @staticmethod : {is_static}")

    if is_static:
        print("[OK] Déjà @staticmethod — pas de patch nécessaire")
        # Tester la connexion directement
        token = os.environ.get('IBM_API_KEY', '')
        crn   = os.environ.get('IBM_CRN',
            'crn:v1:bluemix:public:quantum-computing:us-east:'
            'a/b893e3b64af743f9860d3ae175578d19:'
            '31cb02b4-5010-422a-a999-d6302e4d7b06::')
        from qiskit_ibm_runtime import QiskitRuntimeService
        svc = QiskitRuntimeService(channel='ibm_cloud', token=token, instance=crn)
        print(f"[OK] Connexion IBM Cloud réussie ! backends: {[b.name for b in svc.backends()[:5]]}")
        sys.exit(0)

    # Lire le fichier
    with open(acc_file, 'r') as f:
        src = f.read()

    print(f"[INFO] Taille fichier : {len(src)} chars")

    # Trouver toutes les occurrences de _assert_valid_instance dans CloudAccount
    # Pattern : lignes avec "def _assert_valid_instance" qui ne sont PAS précédées de @staticmethod
    lines = src.splitlines()
    patched = False
    new_lines = []
    i = 0
    while i < len(lines):
        line = lines[i]
        # Chercher "def _assert_valid_instance" sans @staticmethod sur la ligne précédente
        if ('def _assert_valid_instance' in line and
            i > 0 and
            '@staticmethod' not in lines[i-1]):
            # Insérer @staticmethod avant
            indent = len(line) - len(line.lstrip())
            new_lines.append(' ' * indent + '@staticmethod')
            new_lines.append(line)
            print(f"[PATCH] Ligne {i+1} patchée : ajout @staticmethod avant '{line.strip()}'")
            patched = True
        else:
            new_lines.append(line)
        i += 1

    if not patched:
        print("[WARN] Aucune ligne patchée — dump des lignes _assert_valid_instance:")
        for j, l in enumerate(lines):
            if '_assert_valid_instance' in l:
                ctx_start = max(0, j-3)
                ctx_end   = min(len(lines), j+5)
                print(f"  Lignes {ctx_start+1}-{ctx_end+1}:")
                for k in range(ctx_start, ctx_end):
                    marker = ">>>" if k == j else "   "
                    print(f"  {marker} {k+1:4d}: {lines[k]}")
        sys.exit(1)

    # Sauvegarder le fichier patché
    new_src = '\n'.join(new_lines)
    if not new_src.endswith('\n'):
        new_src += '\n'

    # Backup
    backup = acc_file + '.bak_c164'
    with open(backup, 'w') as f:
        f.write(src)
    print(f"[INFO] Backup : {backup}")

    # Écrire le patch
    with open(acc_file, 'w') as f:
        f.write(new_src)
    print(f"[PATCH] ✅ Fichier patché : {acc_file}")

    # Recharger le module et vérifier
    import importlib
    importlib.reload(_acc_mod)
    sm2 = inspect.getattr_static(_acc_mod.CloudAccount, '_assert_valid_instance')
    is_static2 = isinstance(sm2, staticmethod)
    print(f"[VERIFY] _assert_valid_instance is @staticmethod après patch : {is_static2}")

    if not is_static2:
        print("[ERR] Patch échoué — rollback")
        with open(acc_file, 'w') as f:
            f.write(src)
        sys.exit(1)

    # Tester la connexion IBM Cloud
    print("[TEST] Test connexion IBM Cloud post-patch...")
    token = os.environ.get('IBM_API_KEY', '')
    crn   = os.environ.get('IBM_CRN',
        'crn:v1:bluemix:public:quantum-computing:us-east:'
        'a/b893e3b64af743f9860d3ae175578d19:'
        '31cb02b4-5010-422a-a999-d6302e4d7b06::')
    print(f"[TEST] token len={len(token)} crn len={len(crn)}")

    # Recharger QiskitRuntimeService avec le module patché
    import qiskit_ibm_runtime.qiskit_runtime_service as _svc_mod
    importlib.reload(_svc_mod)
    QiskitRuntimeService = _svc_mod.QiskitRuntimeService

    svc = QiskitRuntimeService(channel='ibm_cloud', token=token, instance=crn)
    backends = [b.name for b in svc.backends()[:10]]
    print(f"[OK] ✅ Connexion IBM Cloud réussie !")
    print(f"[OK] Backends disponibles : {backends}")
    sys.exit(0)

except Exception as e:
    import traceback
    print(f"[ERR] {type(e).__name__}: {e}")
    traceback.print_exc()
    sys.exit(1)
