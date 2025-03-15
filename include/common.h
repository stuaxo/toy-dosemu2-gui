#ifndef DOSEMU2_GUI_COMMON_H
#define DOSEMU2_GUI_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ui.h>

/* Application state structure */
typedef struct AppState {
    uiWindow *main_window;
    uiBox *main_vbox;
    uiTab *main_tab;

    /* DOS configuration */
    char *dos_path;
    char *config_path;
    bool auto_start;
    int memory_size;
    int xms_size;
    int ems_size;
    bool use_console;
    bool use_vga;

    /* UI controls */
    uiEntry *dos_path_entry;
    uiEntry *config_path_entry;
    uiEntry *memory_size_entry;
    uiEntry *xms_size_entry;
    uiEntry *ems_size_entry;
    uiCheckbox *auto_start_checkbox;
    uiCheckbox *console_checkbox;
    uiCheckbox *vga_checkbox;
    uiButton *start_button;
    uiButton *stop_button;
    uiMultilineEntry *console;
} AppState;

/* Global application state */
extern AppState app;

#endif /* DOSEMU2_GUI_COMMON_H */
