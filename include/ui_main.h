#ifndef DOSEMU2_GUI_UI_MAIN_H
#define DOSEMU2_GUI_UI_MAIN_H

#include "common.h"

/* Create the main UI */
void create_ui(void);

/* UI callbacks */
void on_start_button_clicked(uiButton *button, void *data);
void on_stop_button_clicked(uiButton *button, void *data);
void on_browse_dos_path_clicked(uiButton *button, void *data);
void on_browse_config_path_clicked(uiButton *button, void *data);

#endif /* DOSEMU2_GUI_UI_MAIN_H */
