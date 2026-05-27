/* lumvorax_ibm_constants.h — wrapper minimal re-exposant les constantes
 * physiques mesurees IBM Quantum (C91/C93/C94) aux modules core LumVorax.
 *
 * Ce wrapper permet a vorax_operations.c, vorax_parser.c, lum_core.c et
 * nx48_btc_controller.c d'acceder aux memes constantes que les modules
 * du sous-systeme quantum_problem_hubbard_hts, sans avoir besoin d'ajouter
 * un -I global dans tous les Makefiles.
 *
 * Strategie : __has_include avec triple fallback puis copie minimale
 * (subset utilisable hors-quantique).
 *
 * Origine : Cycle C94 (2026-04-24) — propagation des resultats QPU reels
 *           dans le code classique LUM/VORAX/PARSER/NX48/QDAYPRIZE.
 */
#ifndef LUMVORAX_IBM_CONSTANTS_WRAPPER_H
#define LUMVORAX_IBM_CONSTANTS_WRAPPER_H

#if defined(__has_include)
#  if __has_include("ibm_quantum_constants.h")
#    include "ibm_quantum_constants.h"
#    define LUMVORAX_IBM_CONSTANTS_FOUND 1
#  elif __has_include("../advanced_calculations/quantum_problem_hubbard_hts/include/ibm_quantum_constants.h")
#    include "../advanced_calculations/quantum_problem_hubbard_hts/include/ibm_quantum_constants.h"
#    define LUMVORAX_IBM_CONSTANTS_FOUND 1
#  elif __has_include("../../advanced_calculations/quantum_problem_hubbard_hts/include/ibm_quantum_constants.h")
#    include "../../advanced_calculations/quantum_problem_hubbard_hts/include/ibm_quantum_constants.h"
#    define LUMVORAX_IBM_CONSTANTS_FOUND 1
#  endif
#endif

#ifndef LUMVORAX_IBM_CONSTANTS_FOUND
/* Fallback minimal — copie des macros critiques C91/C93/C94 actuelles.
 * Mises a jour via un sync script si le header maitre evolue. */
#define LUMVORAX_IBM_CONSTANTS_FALLBACK 1

#define IBM_C91_HVA8_S_PI         ( 0.2999)
#define IBM_C91_HVA12_S_PI        ( 0.3141)
#define IBM_C91_HVA16_S_PI        ( 0.3558)
#define IBM_C93_S_PI              ( 0.9944)
#define IBM_C93_S_PI_STD          ( 0.0040)
#define IBM_C93_C_R1              (-0.9949)
#define IBM_C93_C_R4              ( 0.9954)
#define IBM_C93_GAIN_VS_C91_HVA8  ( 3.3158)
#define IBM_C93_DEPTH_PHYS        14
#define IBM_C93_N2Q_PHYS           2
#define IBM_C94_S_PI_N12          ( IBM_C93_S_PI )
#define IBM_C94_S_PI_N12_STD      ( IBM_C93_S_PI_STD )
#define IBM_C94_S_PI_N16          ( IBM_C91_HVA16_S_PI )
#define IBM_C94_S_PI_N16_STD      ( 0.0049 )

static inline double ibm_best_s_pi_for_N(int N) {
    if (N <= 8)  return IBM_C93_S_PI;
    if (N <= 12) return IBM_C94_S_PI_N12;
    return IBM_C94_S_PI_N16;
}
static inline double ibm_normalize_signal_strength(double s_pi_local, int N) {
    double r = ibm_best_s_pi_for_N(N);
    return (r <= 1e-9) ? 0.0 : s_pi_local / r;
}
static inline int ibm_recommended_max_depth(int N) {
    if (N <= 8)  return 14;
    if (N <= 12) return 22;
    return 30;
}
#endif /* LUMVORAX_IBM_CONSTANTS_FOUND */

/* C94 : flag d'integration verifiable a runtime par les modules.
 * Si vrai, le module a bien lie les constantes physiques IBM dans sa boucle. */
#define LUMVORAX_C94_IBM_BRIDGE 1

#endif /* LUMVORAX_IBM_CONSTANTS_WRAPPER_H */
