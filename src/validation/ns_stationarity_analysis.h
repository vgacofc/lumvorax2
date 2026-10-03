/* **************************************************************************
** ns_stationarity_analysis.h — Analyse de stationnarité fenêtre finale (T04-STRONG)
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / stationarity analysis
** Auteur : LumVorax Project
**
** Objet : fournir une analyse multi-points de la fenêtre finale d'une
**         simulation NS pour décider si le champ est quasi-stationnaire.
**         Ferme le chantier T04 du registre 176 §5 (OPEN renforcé).
**
** Critère de quasi-stationnarité (tous obligatoires) :
**   1. variation max-min relative < QS_RANGE_REL
**   2. écart-type relatif      < QS_STD_REL
**   3. |pente normalisée|      < QS_SLOPE_REL  (régression OLS sur la fenêtre)
**   4. Nombre de points de fenêtre >= QS_MIN_POINTS
**
** Utilisation :
**   StationarityWindow *sw = stationarity_window_create(capacity);
**   stationarity_window_push(sw, linf_value, step);
**   StationarityResult r = stationarity_analyze(sw);
**   stationarity_window_destroy(sw);
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#ifndef NS_STATIONARITY_ANALYSIS_H
#define NS_STATIONARITY_ANALYSIS_H

#include <stdint.h>

/* ── Seuils du critère de quasi-stationnarité ──────────────────────────── */

#define QS_RANGE_REL    0.05   /* max-min / mean < 5 % */
#define QS_STD_REL      0.02   /* std     / mean < 2 % */
#define QS_SLOPE_REL    0.02   /* |pente| / mean < 2 % par point */
#define QS_MIN_POINTS   20     /* fenêtre minimale fiable */

/* ── Structure fenêtre circulaire ──────────────────────────────────────── */

typedef struct {
    double   *values;   /* échantillons de Linf (ou autre observable) */
    int      *steps;    /* numéros de pas correspondants */
    int       capacity; /* taille maximale */
    int       head;     /* index du prochain emplacement d'écriture */
    int       count;    /* nombre d'éléments présents */
} StationarityWindow;

/* ── Résultat de l'analyse ─────────────────────────────────────────────── */

typedef struct {
    /* Statistiques descriptives */
    double   min_val;
    double   max_val;
    double   mean;
    double   std;          /* écart-type population (non biaisé si n>1) */
    double   range;        /* max - min */
    double   range_rel;    /* range / mean (0 si mean < 1e-30) */
    double   std_rel;      /* std / mean   (0 si mean < 1e-30) */

    /* Régression linéaire OLS : valeur = slope * t + intercept */
    double   slope;        /* pente brute (unités/pas) */
    double   slope_rel;    /* |slope| / mean par pas (0 si mean < 1e-30) */
    double   intercept;

    /* Diagnostics */
    int      n_points;     /* nombre de points analysés */
    int      quasi_stationary; /* 1 si tous les critères sont satisfaits */
    char     reason[256];  /* description du verdict (DEBUG) */
} StationarityResult;

/* ── API publique ──────────────────────────────────────────────────────── */

/**
 * Crée une fenêtre circulaire de capacité `capacity`.
 * Retourne NULL en cas d'échec d'allocation.
 */
StationarityWindow *stationarity_window_create(int capacity);

/**
 * Libère la fenêtre.
 */
void stationarity_window_destroy(StationarityWindow *sw);

/**
 * Ajoute une observation (value, step) dans la fenêtre.
 * Si la fenêtre est pleine, écrase le plus ancien échantillon.
 */
void stationarity_window_push(StationarityWindow *sw, double value, int step);

/**
 * Analyse tous les points présents dans la fenêtre.
 * Retourne un StationarityResult avec quasi_stationary=0 si la fenêtre
 * contient moins de QS_MIN_POINTS points.
 */
StationarityResult stationarity_analyze(const StationarityWindow *sw);

/**
 * Imprime un résumé lisible de l'analyse (mode DEBUG).
 */
void stationarity_print_result(const StationarityResult *r, const char *label);

#endif /* NS_STATIONARITY_ANALYSIS_H */
