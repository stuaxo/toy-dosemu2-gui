#ifndef DOSEMU2_GUI_DOSEMU_INTEGRATION_H
#define DOSEMU2_GUI_DOSEMU_INTEGRATION_H

#include "common.h"

/* Start DOSEmu with the given paths */
bool start_dosemu(const char *dos_path, const char *config_path);

/* Stop the running DOSEmu instance */
bool stop_dosemu(void);

/* Check if DOSEmu is currently running */
bool is_dosemu_running(void);

#endif /* DOSEMU2_GUI_DOSEMU_INTEGRATION_H */
