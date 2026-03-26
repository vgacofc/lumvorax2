import json

# Si pandas ne marche pas, on peut au moins préparer les données
# Pour le format parquet, il faut normalement pyarrow/pandas.
# On va essayer de générer un CSV d'abord comme fallback ou un JSON.
# Mais l'utilisateur demande explicitement .parquet

try:
    import pandas as pd
    import pyarrow as pa
    import pyarrow.parquet as pq
    data = {'id': [1, 2], 'answer': [0, 1]}
    df = pd.DataFrame(data)
    table = pa.Table.from_pandas(df)
    pq.write_table(table, 'submission.parquet')
    print("SUCCESS: submission.parquet généré")
except ImportError as e:
    print(f"ERREUR: Import impossible - {e}")
    # On crée au moins le CSV au cas où
    with open('submission.csv', 'w') as f:
        f.write('id,answer\n1,0\n2,1\n')
    print("FALLBACK: submission.csv généré à la place")
