from fpdf import FPDF

class PDF(FPDF):
    def header(self):
        self.set_font('Arial', 'B', 16)
        self.set_text_color(33, 37, 41)
        self.cell(0, 15, 'VGACTEC - STATION F PITCH DECK (V4 VESUVIUS)', 0, 1, 'C')
        self.ln(5)

    def chapter_title(self, title):
        self.set_font('Arial', 'B', 14)
        self.set_fill_color(200, 220, 255)
        self.cell(0, 10, title, 0, 1, 'L', 1)
        self.ln(4)

    def chapter_body(self, body):
        self.set_font('Arial', '', 11)
        self.multi_cell(0, 8, body)
        self.ln()

pdf = PDF()
pdf.add_page()

sections = [
    ("1. LA VISION", "Démocratiser le calcul de haute précision avec une certification totale."),
    ("2. LE PROBLÈME", "L'opacité des systèmes HPC actuels empêche leur adoption dans les secteurs ultra-critiques."),
    ("3. LA SOLUTION : LUMVORAX V4", "Un noyau hybride (C++/Python) certifié à 360° avec traçabilité forensique native."),
    ("4. LA TECHNOLOGIE", "- Noyau liblumvorax.so (GLIBC 2.2.5)\n- 3600 tests de validation réussis\n- Modules IA & Imagerie intégrés (PyTorch, OpenCV)"),
    ("5. L'AVANTAGE VGACTEC", "Unconventional Founders : Nous livrons des systèmes certifiés 'Vesuvius' là où d'autres livrent des promesses."),
    ("6. TRACTION & MATURITÉ", "Certification Définitive obtenue le 21/02/2026. Prêt pour déploiement industriel."),
    ("7. L'ÉQUIPE", "Experts autodidactes en bas-niveau et mathématiques appliquées.")
]

for title, body in sections:
    pdf.chapter_title(title)
    pdf.chapter_body(body)

pdf.output('/home/runner/workspace/VGACTEC_PITCH_DECK_STATION_F.pdf', 'F')
