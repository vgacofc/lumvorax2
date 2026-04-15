# LumVorax C45 — Bitcoin Core RPC Setup via Ubuntu CLI Agent

## Architecture C45

```
[Replit Flask :5000]
  /agent/push  ← envoie commandes BTC Core
  /agent/job   ← Ubuntu poll
  /agent/result← Ubuntu renvoie résultats
        ↕ HTTPS (AGENT_TOKEN)
[Ubuntu lvx — lvx-Vostro-5481]
  bitcoin-cli  → Bitcoin Core daemon local
  btc_mining_runner (binaire Replit copié via SCP ngrok)
```

## Étape 1 — Installer Bitcoin Core sur Ubuntu

```bash
# Ajouter le PPA officiel
sudo add-apt-repository ppa:bitcoin/bitcoin -y
sudo apt update
sudo apt install bitcoind bitcoin-cli -y

# Vérifier
bitcoind --version
bitcoin-cli --version
```

## Étape 2 — Configurer bitcoin.conf

```bash
mkdir -p ~/.bitcoin
cat > ~/.bitcoin/bitcoin.conf << 'EOF'
# LumVorax C45 — Bitcoin Core config (testnet4 recommandé)
chain=testnet4
daemon=1
server=1
rpcuser=lumvorax
rpcpassword=LumVorax2026BTC!
rpcallowip=127.0.0.1
rpcport=48332
txindex=1
zmqpubrawblock=tcp://127.0.0.1:28332
zmqpubrawtx=tcp://127.0.0.1:28333
EOF
```

## Étape 3 — Démarrer Bitcoin Core (testnet4)

```bash
bitcoind -chain=testnet4 -daemon
# Vérifier sync
bitcoin-cli -chain=testnet4 -rpcport=48332 -rpcuser=lumvorax -rpcpassword="LumVorax2026BTC!" getblockchaininfo
```

## Étape 4 — Configurer les secrets Replit

Ajouter dans les secrets Replit :
```
BTC_RPC_URL=http://lvx-Vostro-5481:48332
BTC_RPC_USER=lumvorax
BTC_RPC_PASS=LumVorax2026BTC!
UBUNTU_SSH_HOST=<ngrok-host>
UBUNTU_SSH_PORT=<ngrok-port>
UBUNTU_SSH_USER=lvx
```

## Étape 5 — Lancer l'agent Ubuntu (CLI)

```bash
# Sur Ubuntu — terminal dédié (ne pas fermer)
bash ~/agent_ubuntu.sh
```

## Étape 6 — Copier le binaire vers Ubuntu via agent

Depuis Replit (terminal) :
```bash
TOKEN="3de963ebc09043b3b1b9f22e1e771ecc"
URL="https://de10cf97-9bc3-4cd9-9cb9-e3269b99076a-00-3b0h1avyi2m16.picard.replit.dev"

# Pousser un job vers Ubuntu
curl -X POST "$URL/agent/push" \
  -H "X-Agent-Token: $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"cmd":"bitcoin-cli -chain=testnet4 -rpcport=48332 -rpcuser=lumvorax -rpcpassword=LumVorax2026BTC! getblocktemplate \"{\\\"rules\\\":[\\\"segwit\\\"]}\" 2>&1 | head -20","label":"getblocktemplate_test","timeout_s":30}'

# Voir le résultat
sleep 10
curl "$URL/agent/results" -H "X-Agent-Token: $TOKEN" | python3 -m json.tool | tail -30
```

## Étape 7 — Pipeline getblocktemplate complet

```bash
# Job: récupérer le template + lancer le mining Ubuntu
curl -X POST "$URL/agent/push" \
  -H "X-Agent-Token: $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "cmd": "python3 ~/lumvorax/scripts/btc_getblocktemplate_job.py --rpc-url http://127.0.0.1:48332 --rpc-user lumvorax --rpc-pass LumVorax2026BTC! --output ~/lumvorax/job.json && ~/lumvorax/btc_mining_runner --mode BENCHMARK --job ~/lumvorax/job.json --threads 16 --duration-s 60",
    "label": "btc_full_pipeline_testnet4",
    "timeout_s": 90
  }'
```

## Clé SSH Replit (à ajouter dans authorized_keys Ubuntu)

```
ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIGwDUInCkZnsrPGiXyfA2ZUELRXw0y+j0fNckFTsfA+q replit-lumvorax-c45
```

Sur Ubuntu :
```bash
echo "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIGwDUInCkZnsrPGiXyfA2ZUELRXw0y+j0fNckFTsfA+q replit-lumvorax-c45" >> ~/.ssh/authorized_keys
chmod 600 ~/.ssh/authorized_keys
```

## Variables d'environnement Replit requises

| Secret | Valeur | Statut |
|---|---|---|
| `SUPABASE_URL` | `https://xequjlbrvbzhrfxqhioq.supabase.co` | ✅ OK |
| `SUPABASE_SERVICE_ROLE_KEY` | `eyJ...` | ✅ OK |
| `SUPABASE_DB_PASSWORD` | mis à jour | ✅ OK |
| `DATADOG_API_TOKEN` | `ddapp_qM...` | ✅ OK |
| `SESSION_SECRET` | présent | ✅ OK (génère AGENT_TOKEN) |
| `BTC_RPC_URL` | absent | ⚠️ À configurer Ubuntu |
| `BTC_RPC_USER` | absent | ⚠️ À configurer Ubuntu |
| `BTC_RPC_PASS` | absent | ⚠️ À configurer Ubuntu |
| `SUPERMEMORY_API_KEY` | absent | ⚠️ À fournir |
| `NGROK_AUTHTOKEN` | absent | ⚠️ Optionnel (SSH backup) |

## Notes C45

- L'agent Ubuntu ne nécessite PAS ngrok — il se connecte vers Replit (sortant)
- AGENT_TOKEN = `sha256("agent:" + SESSION_SECRET)[:32]` = `3de963ebc09043b3b1b9f22e1e771ecc`
- Replit URL publique : `https://de10cf97-9bc3-4cd9-9cb9-e3269b99076a-00-3b0h1avyi2m16.picard.replit.dev`
- Le workflow "Start application" doit être actif pour que Ubuntu puisse se connecter
