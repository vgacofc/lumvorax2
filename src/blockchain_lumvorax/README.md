# `src/blockchain_lumvorax/` — Blockchain décentralisée sur LUM DBMS

Cycle C95. Voir `src/advanced_calculations/bitcoin_quantum_mining/CHAT/CAHIER_DES_CHARGES_C95_MAITRE.md` §4 pour la spécification complète.

## Vision
LUMVORAX possède déjà tous les briques bas-niveau d'une blockchain :
- Format `.lum` magic LUMQ v2 + gzip + SHA256 → équivalent fonctionnel d'un bloc
- WAL hash-chained (`transaction_wal_extension.c`) → équivalent chaîne
- MVCC (`lum_mvcc.c`) → versioning de la chaîne
- SHA-256 du minier BTC → réutilisable pour Merkle + PoW
- NX48 dual-neuron → consensus PoW intelligent

Il manque la couche P2P + wallet + RPC.

## État C95
- ✅ `blockchain_lumvorax.h` — API publique (header bloc, transactions, Merkle, consensus, wallet, chaîne)
- ✅ `block_header.c` — calcul leading_zeros (compatible BTC)
- ⏳ `merkle_tree.c`
- ⏳ `consensus_pow_lum.c` (réutilise minier NX48)
- ⏳ `consensus_pos_neural.c` (réseau neural axe A)
- ⏳ `wallet_ed25519.c`
- ⏳ `p2p_gossip.c`
- ⏳ `chain_validator.c`
- ⏳ `rpc_jsonrpc.c`

## Roadmap
| Phase | Livrable | Durée |
|-------|----------|-------|
| B1 | Bloc + Merkle + chaîne mono-nœud | 1 sem |
| B2 | Wallet Ed25519 + sigs | 3 j |
| B3 | P2P 2 nœuds | 1 sem |
| B4 | PoW NX48 multi-nœuds | 1 sem |
| B5 | RPC JSON-RPC subset Bitcoin Core | 3 j |
| B6 | PoS neural (axe A) | 2 sem |
