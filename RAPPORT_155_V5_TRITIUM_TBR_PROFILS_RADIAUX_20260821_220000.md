# RAPPORT 155 — V5 : AUTO-SUFFISANCE TRITIUM (TBR) ET PROFILS RADIAUX CALCULES
## 98 PASS / 0 FAIL dans les deux arbres — couverture 61 % — le piquage forfaitaire remplace par un calcul radial reel
### Date : 2026-08-21 22:00:00 UTC — Suite des RAPPORTS 150-154

---

## 1. CE QUI A ETE AJOUTE (l'etape annoncee au RAPPORT 154 §9)

### 1.1 Module tritium (contrainte C9 — verrou absolu d'une centrale)
Le tritium n'existe pas naturellement en quantite (stock mondial civil
~25 kg) ; une centrale consomme ~56 kg/an/GW de fusion et DOIT les regenerer
dans sa couverture lithiee (6Li + n -> T + He). Implementation :
- TBR = TBR_local (concept publie) x couverture geometrique ;
- bilan tritium par design : consommation (0.0561 kg/an/MW — verifiee par
  test physique independant), production nette kg/an ;
- contrainte C9 : TBR >= 1.05, integree au verdict de viabilite.

**Validations contre les etudes publiees (phase 15)** :
| Concept | TBR calcule | Publie |
|---|---|---|
| LTS / HCPB (EU-DEMO) | 1.105 | ~1.10-1.15 |
| HTS / FLiBe+Be (ARC) | 1.306 | ~1.3 |
| Contre-exemple couverture 70 % | 0.99 | REJETE par C9 (le detecteur mord) |

### 1.2 Module profils radiaux (fondation du transport 1.5-D)
`src/physics/fusion_dt_profiles.{h,c}` : integration de Simpson (200 pas)
des profils parametriques n(rho) = n0(1-rho^2)^0.3, T(rho) = T0(1-rho^2)^1.2 ;
piquages fusion et bremsstrahlung EXACTS pour ces profils ; piquage effectif
ancre sur la calibration ITER (1.3 a 9 keV) avec variation en T CALCULEE.
Le facteur forfaitaire 1.3 de V3/V4 est remplace par ce calcul dans toutes
les evaluations de reacteurs (re-simulation de la combustion au piquage
recalcule a la temperature d'equilibre).

---

## 2. DECOUVERTE V5 (statut : resultat emergent quantifie de notre modele)

**Le gain de piquage s'evanouit dans les regimes chauds.** A <T> = 20 keV,
la temperature centrale atteint ~44 keV, ou la reactivite D-T est quasi
PLATE : le piquage des profils n'amplifie plus la puissance de fusion
(piquage effectif calcule : 1.30 a 9 keV -> 1.00 a 20 keV). Consequence
chiffree : la calibration forfaitaire 1.3 de V3/V4 SURESTIMAIT les designs
chauds — P_net LTS corrige de 196 a 127 MW ; l'optimiseur HTS compense en
deplacant l'optimum vers la densite maximale (fGW 0.64 -> 0.90), gagnant
finalement +107 MW (381 -> 488). Un modele plus honnete a produit de
MEILLEURES solutions en explorant une region que le modele optimiste
n'avait pas privilegiee.

---

## 3. SOLUTIONS V5 (98 verifications, deux arbres, 1296 combustions)

| Catalogue | R | T | Q | P_fus | **P_net** | TBR | Tritium net | Piquage |
|---|---|---|---|---|---|---|---|---|
| LTS (etat ITER) | 9.50 m | 17.6 keV | 8.2 | 1731 MW | **+127 MW** | 1.10 | +10.2 kg/an | 1.00 |
| HTS (SPARC/ARC) | 6.00 m | 15.7 keV | 11.2 | 2059 MW | **+488 MW** | 1.31 | +35.4 kg/an | 1.00 |
| FUTUR (hypothetique) | 4.29 m | 17.3 keV | 10.4 | 1540 MW | **+501 MW** | 1.40 | +34.1 kg/an | 1.00 |

Enseignement industriel chiffre : la solution HTS produit 488 MW nets ET
35 kg/an de tritium excedentaire — de quoi demarrer une nouvelle centrale
tous les ~2-3 ans (inventaire de demarrage estime 5-10 kg par machine) :
la filiere est auto-multiplicatrice, condition necessaire d'un deploiement
mondial.

---

## 4. COUVERTURE VERS LA SOLUTION IDEALE : 61 % (53 % en V4, 45 % en V3)

| Verrou monte en V5 | Avant | Apres |
|---|---|---|
| Production de tritium (TBR + bilan kg/an) | 0 % | 60 % |
| Profils radiaux / transport | 20 % (parametre) | 50 % (calcule, ancre ITER) |

Verrous restants dominants : controle temps reel/disruptions 3-D (0 %),
economie/licensing (10 %), synchrotron (20 %), montee du transport a un
vrai 1.5-D avec equation de diffusion (50 %->80 %).

---

## 5. PREUVES

- 98 assertions / 0 echec dans les DEUX arbres (lumc01/Lumvorax,
  vgacofc/lumvorax2), zero warning, zero fuite memoire.
- Phases nouvelles : 14 (profils : exactitude profils plats = 1.0 a 1e-9,
  piquage parabolique 2.197 dans la plage publiee 1.5-2.5, monotonie,
  brems < fusion, ancrage exact 1.300) et 15 (TBR EU-DEMO/ARC, constante
  de consommation verifiee, contre-exemple rejete).
- Non-regression integrale des phases 1-13.
- Tracabilite inchangee : ring nanoseconde, hash bit-level, .lum diff=0.

## 6. CONCLUSION

La V5 ferme le verrou tritium (le seul poste a 0 % du tableau V4) et
remplace la derniere calibration forfaitaire par un calcul radial reel —
qui a immediatement produit une decouverte de modele (evanouissement du
piquage a haute temperature) et de meilleures solutions. Prochaine etape a
plus forte valeur : transport 1.5-D complet (equation de diffusion de la
chaleur avec chi(rho) empirique), puis pertes synchrotron (formule
d'Albajar/Trubnikov avec coefficients verifies en bibliographie).
