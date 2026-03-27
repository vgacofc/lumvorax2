# Auto-generated from notebook_validation_lumvorax_dependances_360_20260220.ipynb
# Copy/paste friendly for a single Kaggle notebook code cell.

# %% [markdown cell 1]
# # Notebook validation 360 LUMVORAX dépendances
# Valide modules C/C++, inventaire fonctions, compilation smoke, conversion TIFF->LUMV1, et journaux forensic.

# %% [code cell 2]
from pathlib import Path
import json, subprocess, tempfile, os
ROOT=Path('/workspace/Lumvorax')
print(ROOT)

# %% [code cell 3]
# 1) Inventaire modules/fonctions
cmd=['python3', str(ROOT/'RAPPORT-VESUVIUS/validation_lumvorax/build_lumvorax_module_inventory.py')]
print(subprocess.check_output(cmd,text=True))

# %% [code cell 4]
# 2) Smoke compile modules C critiques
modules=[
 'src/lum/lum_core.c',
 'src/file_formats/lum_native_universal_format.c',
 'src/file_formats/lum_native_file_handler.c',
 'src/file_formats/lum_secure_serialization.c',
 'src/vorax/vorax_3d_volume.c',
 'src/vorax/vorax_operations.c',
]
for m in modules:
    cmd=['gcc','-fsyntax-only',str(ROOT/m),'-I',str(ROOT/'src')]
    r=subprocess.run(cmd,capture_output=True,text=True)
    print(m,'OK' if r.returncode==0 else 'FAIL')
    if r.returncode!=0:
        print(r.stderr[:600])

# %% [code cell 5]
# 3) Conversion TIFF -> LUMV1 + logs par tranche
import numpy as np, tifffile
with tempfile.TemporaryDirectory() as td:
    td=Path(td)
    tif=td/'sample.tif'
    arr=(np.random.rand(8,32,32)>0.9).astype('uint8')
    tifffile.imwrite(tif,arr,compression='LZW')
    lum=td/'sample.lum'
    log=td/'sample_log.json'
    cmd=['python3',str(ROOT/'RAPPORT-VESUVIUS/validation_lumvorax/convert_competitor_tiff_to_lum.py'),'--input',str(tif),'--output',str(lum),'--log',str(log)]
    print(subprocess.check_output(cmd,text=True))
    rep=json.loads(log.read_text())
    print('shape',rep['shape'],'slices',len(rep['slice_logs']),'global_nonzero_pct',rep['global_nonzero_pct'])

# %% [code cell 6]
# 4) Vérification existence tests Python/C listés
for p in sorted((ROOT/'tests').glob('test_*.py')):
    print(p.name)

