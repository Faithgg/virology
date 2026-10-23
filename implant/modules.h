#ifndef S0P_IMPLANT_MODULES_H
#define S0P_IMPLANT_MODULES_H

#include "../shared/protocol.h"

typedef struct {
    const char *name;
    const char *description;
    const char *mitre_id;
    const char *blue_team_note;
    char *(*execute)(const char *args);
} module_def_t;

const module_def_t *modules_get_all(int *count);
const module_def_t *modules_find(const char *name);

#endif
