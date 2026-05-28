#!/bin/bash
# MDBAI Server — Script de démarrage
# Contourne le prompt interactif Doppler CLI (update check)
# Port: 3001 (séparé de Flask port 5000)
cd "$(dirname "$0")"
printf "n\n" | doppler run --project lumvorax --config dev_lumvorax -- node src/server.js
