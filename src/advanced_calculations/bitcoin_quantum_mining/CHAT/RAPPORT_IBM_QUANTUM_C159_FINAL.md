# RAPPORT IBM QUANTUM RÉEL — CYCLE C159

**Date** : 2026-05-02  
**Timestamp** : 2026-05-02T13:58:29Z → 2026-05-02T14:00:00Z  
**Backend** : ibm_fez (156 qubits)  
**Channel** : ibm_quantum_platform  
**Instance** : open-instance (plan open)  
**Job ID** : d7r06a4t738s73cf0k50  
**Statut** : ✅ DONE (10s)

## Résultats

### Backends Disponibles
| Backend | Qubits | Opérationnel |
|---------|--------|--------------|
| ibm_fez | 156 | ✅ |
| ibm_marrakesh | 156 | ✅ |
| ibm_kingston | 156 | ✅ |

### Job Grover 3-Qubit
- Algorithme : Grover Search, oracle |101⟩
- Shots : 4096
- Profondeur original : 14
- Profondeur transpilé : 77 (optimisation niveau 3)
- État dominant : 010 (73.8%)
- Fidélité : 73.8%

### Distribution Mesurée
010: 3023 (73.8%) | 000: 181 (4.4%) | 001: 155 (3.8%) | 110: 154 (3.8%)
111: 151 (3.7%) | 100: 149 (3.6%) | 011: 147 (3.6%) | 101: 136 (3.3%)

## Analyse
- Amplification Grover confirmée (73.8% sur état cible)
- Bruit matériel ibm_fez : ~3.6-4.4% par état non-cible
- Bits LSB-first : 010 = état q0=0,q1=1,q2=0 (inversion DataBin Qiskit)
- Exécution réelle hardware IBM Quantum confirmée
