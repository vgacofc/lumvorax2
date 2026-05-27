#!/bin/bash

# Script d'analyse hashrate MIN/MAX réels depuis les logs forensiques
# Session C286 - Analyse complète

LOG_FILE="logs/forensic/modules/btc_qm_engine_forensic_btc_20260519T191944Z_184415.log"

echo "═══════════════════════════════════════════════════════════════"
echo "  ANALYSE HASHRATE MIN/MAX RÉELS - SESSION C286"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Vérifier que le fichier existe
if [ ! -f "$LOG_FILE" ]; then
    echo "❌ ERREUR: Fichier log non trouvé: $LOG_FILE"
    exit 1
fi

echo "📁 Fichier analysé: $LOG_FILE"
echo "📊 Extraction des métriques hashrate..."
echo ""

# Extraire tous les hashrates des batches GPU (format: "Batch X: Y.YY MH/s")
grep -oP 'Batch \d+: \K[\d.]+(?= MH/s)' "$LOG_FILE" > /tmp/hashrates_mhs.txt 2>/dev/null || true
grep -oP 'Batch \d+: \K[\d.]+(?= GH/s)' "$LOG_FILE" > /tmp/hashrates_ghs.txt 2>/dev/null || true

# Convertir GH/s en MH/s pour comparaison uniforme
if [ -f /tmp/hashrates_ghs.txt ] && [ -s /tmp/hashrates_ghs.txt ]; then
    awk '{print $1 * 1000}' /tmp/hashrates_ghs.txt >> /tmp/hashrates_mhs.txt
fi

# Vérifier qu'on a des données
if [ ! -f /tmp/hashrates_mhs.txt ] || [ ! -s /tmp/hashrates_mhs.txt ]; then
    echo "❌ ERREUR: Aucune donnée hashrate trouvée dans les logs"
    exit 1
fi

# Calculer statistiques
TOTAL_BATCHES=$(wc -l < /tmp/hashrates_mhs.txt)
MIN_MHS=$(sort -n /tmp/hashrates_mhs.txt | head -1)
MAX_MHS=$(sort -n /tmp/hashrates_mhs.txt | tail -1)
AVG_MHS=$(awk '{sum+=$1} END {print sum/NR}' /tmp/hashrates_mhs.txt)

# Convertir en GH/s
MIN_GHS=$(echo "scale=2; $MIN_MHS / 1000" | bc)
MAX_GHS=$(echo "scale=2; $MAX_MHS / 1000" | bc)
AVG_GHS=$(echo "scale=2; $AVG_MHS / 1000" | bc)

# Trouver les numéros de batch pour MIN et MAX
MIN_BATCH=$(grep -n "$(printf "%.2f" $MIN_MHS) MH/s\|$(printf "%.2f" $MIN_GHS) GH/s" "$LOG_FILE" | head -1 | grep -oP 'Batch \K\d+' || echo "N/A")
MAX_BATCH=$(grep -n "$(printf "%.2f" $MAX_MHS) MH/s\|$(printf "%.2f" $MAX_GHS) GH/s" "$LOG_FILE" | head -1 | grep -oP 'Batch \K\d+' || echo "N/A")

# Affichage des résultats
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  📈 STATISTIQUES HASHRATE GPU"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "  Total Batches Analysés : $TOTAL_BATCHES batches"
echo ""
echo "  🔻 HASHRATE MINIMUM"
echo "     └─ Valeur    : $MIN_GHS GH/s ($MIN_MHS MH/s)"
echo "     └─ Batch #   : $MIN_BATCH"
echo ""
echo "  🔺 HASHRATE MAXIMUM"
echo "     └─ Valeur    : $MAX_GHS GH/s ($MAX_MHS MH/s)"
echo "     └─ Batch #   : $MAX_BATCH"
echo ""
echo "  📊 HASHRATE MOYEN"
echo "     └─ Valeur    : $AVG_GHS GH/s ($AVG_MHS MH/s)"
echo ""
echo "  📉 DÉGRADATION"
echo "     └─ Absolue   : $(echo "scale=2; $MAX_GHS - $MIN_GHS" | bc) GH/s"
echo "     └─ Relative  : $(echo "scale=1; (($MAX_GHS - $MIN_GHS) / $MAX_GHS) * 100" | bc)%"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Analyse distribution par tranches
echo ""
echo "  📊 DISTRIBUTION HASHRATE (par tranches de 0.5 GH/s)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

awk '{
    ghs = $1 / 1000
    if (ghs < 0.5) bucket = "0.0-0.5"
    else if (ghs < 1.0) bucket = "0.5-1.0"
    else if (ghs < 1.5) bucket = "1.0-1.5"
    else if (ghs < 2.0) bucket = "1.5-2.0"
    else if (ghs < 2.5) bucket = "2.0-2.5"
    else if (ghs < 3.0) bucket = "2.5-3.0"
    else if (ghs < 3.5) bucket = "3.0-3.5"
    else if (ghs < 4.0) bucket = "3.5-4.0"
    else bucket = "4.0+"
    count[bucket]++
}
END {
    for (b in count) {
        pct = (count[b] / NR) * 100
        printf "  %s GH/s : %4d batches (%5.1f%%) ", b, count[b], pct
        # Barre de progression
        bars = int(pct / 2)
        for (i = 0; i < bars; i++) printf "█"
        printf "\n"
    }
}' /tmp/hashrates_mhs.txt | sort

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Top 10 et Bottom 10
echo ""
echo "  🏆 TOP 10 BATCHES (Hashrate le plus élevé)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
grep -oP 'Batch \d+: [\d.]+ [MG]H/s' "$LOG_FILE" | \
    awk '{
        if ($3 == "GH/s") mhs = $2 * 1000
        else mhs = $2
        print mhs, $0
    }' | sort -rn | head -10 | \
    awk '{printf "  #%-4s : %6.2f GH/s\n", substr($4, 1, length($4)-1), $1/1000}'

echo ""
echo "  ⚠️  BOTTOM 10 BATCHES (Hashrate le plus faible)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
grep -oP 'Batch \d+: [\d.]+ [MG]H/s' "$LOG_FILE" | \
    awk '{
        if ($3 == "GH/s") mhs = $2 * 1000
        else mhs = $2
        print mhs, $0
    }' | sort -n | head -10 | \
    awk '{printf "  #%-4s : %6.2f GH/s\n", substr($4, 1, length($4)-1), $1/1000}'

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  ✅ Analyse terminée"
echo "═══════════════════════════════════════════════════════════════"

# Nettoyage
rm -f /tmp/hashrates_mhs.txt /tmp/hashrates_ghs.txt

# Made with Bob
