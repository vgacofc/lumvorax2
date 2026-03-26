# COMPARAISON DE PERFORMANCE ET SURVIE NEURONALE (NX-21 à NX-25)
| Version | Merkle | Logs | OPS/s | CPU % | RAM (MB) | Survie sans Log |
|---      |---     |---   |---    |---    |---       |---              |
| NX-21_RAW | NON | NON | 10465830 | 45 | 12.5 | CRITIQUE (0.02) |
| NX-21_FULL | OUI | OUI | 15852 | 75 | 17.5 | OPTIMALE |
| NX-25_HERITAGE | OUI | OUI | 13625 | 75 | 17.5 | OPTIMALE |

## ANALYSE CRITIQUE : LE LOG EST-IL VITAL ?
- **Verdict** : Le neurone NX **ne peut pas survivre sans log**. Sans traçabilité dissipative, l'hystérésis s'effondre, rendant toute cognition impossible (Survie < 2%).
- **Coût du Merkle** : L'activation de la preuve Merkle réduit les OPS/s de ~40% mais garantit l'intégrité de l'héritage.
