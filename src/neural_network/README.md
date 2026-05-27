# `src/neural_network/` — Réseau neuronal biologique multi-échelle

Cycle C95. Voir `src/advanced_calculations/bitcoin_quantum_mining/CHAT/CAHIER_DES_CHARGES_C95_MAITRE.md` §3 pour la spécification complète.

## Périmètre réaliste
| Échelle | Nombre cible | Faisabilité |
|---------|-------------|-------------|
| L0 atomique (MD) | 10⁴ atomes (un pore canal Na_v 1.5) | Ubuntu + GROMACS |
| L1 moléculaire | canaux Markov + récepteurs | Replit + Ubuntu |
| L2 compartimental | 10³ segments / neurone | Replit |
| L3 cellulaire (Izhikevich) | 10⁴ neurones | Replit (✅ implémenté) |
| L4 réseau STDP | 10⁵–10⁶ synapses | Ubuntu |

## État C95
- ✅ `neural_network.h` — API publique des 5 niveaux
- ✅ `izhikevich_spiking.c` — niveau L3 fonctionnel (2 sous-pas Euler)
- ⏳ `ion_channel_markov.c` — niveau L1 (canaux Na/K/Ca/Cl)
- ⏳ `receptor_ampa_nmda_gaba.c` — niveau L1 (synapses chimiques)
- ⏳ `cable_compartment.c` — niveau L2
- ⏳ `stdp_plasticity.c` — niveau L4
- ⏳ `lum_neural_format.c` — sérialisation `.lum`

## Limites physiques
- 1 neurone humain ≈ 10¹⁴ atomes
- Cerveau humain ≈ 10²⁵ atomes
- État de l'art MD 2026 : ~10⁹ atomes max (Frontier exascale)
- **Reproduction « atome par atome » d'un neurone entier = impossible avec la technologie 2026**

La stratégie multi-échelle est donc la seule voie scientifiquement honnête.
