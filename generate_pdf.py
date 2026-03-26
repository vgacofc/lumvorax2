from fpdf import FPDF

class PDF(FPDF):
    def header(self):
        self.set_font('Arial', 'B', 15)
        self.cell(80)
        self.cell(30, 10, 'VGACTEC - PITCH DECK', 0, 0, 'C')
        self.ln(20)

pdf = PDF()
pdf.add_page()
pdf.set_font('Arial', '', 12)

content = """
VGACTEC - PITCH DECK : STATION F (Founders Program)

1. La Vision
Démocratiser la puissance de calcul brute avec une sécurité mathématique absolue.

2. Le Problème
Le calcul haute performance actuel est soit trop complexe, soit trop peu sécurisé pour les applications critiques (IA, Cryptographie, Simulation).

3. La Solution : LUM/VORAX
Un noyau de calcul ultra-optimisé (AVX2/SIMD) couplé à une couche de preuve formelle garantissant l'absence de failles critiques.

4. Technologie (L'Audit)
- 45+ modules intégrés.
- Architecture Lock-free & Slab allocation.
- Convergence accélérée via métriques de Lyapunov.

5. Pourquoi VGACTEC ?
Nous sommes des 'unconventional founders' ayant développé une pile technologique complète (NX Series) capable de rivaliser avec les infrastructures industrielles, sans les diplômes académiques classiques mais avec des preuves formelles irréfruables.

6. Traction
- Version V32 stable.
- Preuves formelles validées à 100% sur le Pure Core.
- Prêt pour une intégration à grande échelle.

7. L'Équipe
Experts autodidactes en bas-niveau, mathématiques appliquées et optimisation système.
"""

for line in content.split('\n'):
    pdf.multi_cell(0, 10, line)

pdf.output('VGACTEC_PITCH_DECK.pdf', 'F')
