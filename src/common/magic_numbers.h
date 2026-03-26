#ifndef MAGIC_NUMBERS_H
#define MAGIC_NUMBERS_H

#define MAGIC_DESTROYED_PATTERN  0xDEADBEEF
#define LUM_CORE_MAGIC           0x51554E54

/* LUM_GROUP_MAGIC: identifiant de groupe LumVorax pour vérification
 * d'intégrité des fichiers persistés (data_persistence.c §integrity). */
#define LUM_GROUP_MAGIC          0x4C554D47  /* "LUMG" */

#endif
