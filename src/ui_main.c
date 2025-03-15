#include "../include/ui_main.h"
#include "../include/dosemu_integration.h"

/* Create a labeled control with horizontal layout */
static uiBox *create_labeled_control(const char *label_text, uiControl *control) {
    uiBox *hbox = uiNewHorizontalBox();
    uiBoxSetPadded(hbox, 1);

    uiLabel *label = uiNewLabel(label_text);
    uiBoxAppend(hbox, uiControl(label), 0);
    uiBoxAppend(hbox, control, 1);

    return hbox;
}

/* Create a file browser button and handler */
static uiButton *create_browse_button(uiButton **button, void (*callback)(uiButton *, void *)) {
    *button = uiNewButton("Browse...");
    uiButtonOnClicked(*button, callback, NULL);
    return *button;
}

/* Create the configuration tab */
static uiControl *create_config_tab(void) {
    uiBox *vbox = uiNewVerticalBox();
    uiBoxSetPadded(vbox, 1);

    /* DOS path entry with browse button */
    uiBox *dos_path_box = uiNewHorizontalBox();
    uiBoxSetPadded(dos_path_box, 1);

    app.dos_path_entry = uiNewEntry();
    uiEntrySetText(app.dos_path_entry, "/usr/bin/dosemu");

    uiButton *browse_dos_button;
    create_browse_button(&browse_dos_button, on_browse_dos_path_clicked);

    uiBoxAppend(dos_path_box, uiControl(create_labeled_control("DOSEmu Path:", uiControl(app.dos_path_entry))), 1);
    uiBoxAppend(dos_path_box, uiControl(browse_dos_button), 0);

    /* Config path entry with browse button */
    uiBox *config_path_box = uiNewHorizontalBox();
    uiBoxSetPadded(config_path_box, 1);

    app.config_path_entry = uiNewEntry();
    uiEntrySetText(app.config_path_entry, "~/.dosemurc");

    uiButton *browse_config_button;
    create_browse_button(&browse_config_button, on_browse_config_path_clicked);

    uiBoxAppend(config_path_box, uiControl(create_labeled_control("Config Path:", uiControl(app.config_path_entry))), 1);
    uiBoxAppend(config_path_box, uiControl(browse_config_button), 0);

    /* Memory configurations */
    uiForm *memory_form = uiNewForm();
    uiFormSetPadded(memory_form, 1);

    app.memory_size_entry = uiNewEntry();
    uiEntrySetText(app.memory_size_entry, "16384");
    uiFormAppend(memory_form, "Memory Size (KB):", uiControl(app.memory_size_entry), 0);

    app.xms_size_entry = uiNewEntry();
    uiEntrySetText(app.xms_size_entry, "8192");
    uiFormAppend(memory_form, "XMS Size (KB):", uiControl(app.xms_size_entry), 0);

    app.ems_size_entry = uiNewEntry();
    uiEntrySetText(app.ems_size_entry, "8192");
    uiFormAppend(memory_form, "EMS Size (KB):", uiControl(app.ems_size_entry), 0);

    /* Video options */
    uiBox *video_box = uiNewHorizontalBox();
    uiBoxSetPadded(video_box, 1);

    app.console_checkbox = uiNewCheckbox("Use PC Console Video (-c)");
    uiCheckboxSetChecked(app.console_checkbox, 1);

    app.vga_checkbox = uiNewCheckbox("Use BIOS-VGA Modes (-V)");
    uiCheckboxSetChecked(app.vga_checkbox, 1);

    uiBoxAppend(video_box, uiControl(app.console_checkbox), 1);
    uiBoxAppend(video_box, uiControl(app.vga_checkbox), 1);

    /* Auto-start checkbox */
    app.auto_start_checkbox = uiNewCheckbox("Auto-start DOSEmu at launch");

    /* Add components to the configuration tab */
    uiBoxAppend(vbox, uiControl(dos_path_box), 0);
    uiBoxAppend(vbox, uiControl(config_path_box), 0);
    uiBoxAppend(vbox, uiControl(memory_form), 0);
    uiBoxAppend(vbox, uiControl(video_box), 0);
    uiBoxAppend(vbox, uiControl(app.auto_start_checkbox), 0);

    return uiControl(vbox);
}

/* Create the control tab */
static uiControl *create_control_tab(void) {
    uiBox *vbox = uiNewVerticalBox();
    uiBoxSetPadded(vbox, 1);

    /* Status display */
    uiForm *status_form = uiNewForm();
    uiFormSetPadded(status_form, 1);

    uiLabel *status_label = uiNewLabel("Stopped");
    uiFormAppend(status_form, "Status:", uiControl(status_label), 0);

    /* Control buttons */
    uiBox *button_box = uiNewHorizontalBox();
    uiBoxSetPadded(button_box, 1);

    app.start_button = uiNewButton("Start DOSEmu");
    uiButtonOnClicked(app.start_button, on_start_button_clicked, NULL);

    app.stop_button = uiNewButton("Stop DOSEmu");
    uiButtonOnClicked(app.stop_button, on_stop_button_clicked, NULL);
    uiControlDisable(uiControl(app.stop_button));

    uiBoxAppend(button_box, uiControl(app.start_button), 1);
    uiBoxAppend(button_box, uiControl(app.stop_button), 1);

    /* Add components to the control tab */
    uiBoxAppend(vbox, uiControl(status_form), 0);
    uiBoxAppend(vbox, uiControl(button_box), 0);

    /* Add a spacer */
    uiBoxAppend(vbox, uiControl(uiNewVerticalSeparator()), 0);

    /* Add output console (placeholder) */
    uiBox *console_box = uiNewVerticalBox();
    uiBoxSetPadded(console_box, 1);

    uiLabel *console_label = uiNewLabel("Console Output:");
    uiBoxAppend(console_box, uiControl(console_label), 0);

    app.console = uiNewMultilineEntry();
    uiMultilineEntrySetReadOnly(app.console, 1);
    uiBoxAppend(console_box, uiControl(app.console), 1);

    uiBoxAppend(vbox, uiControl(console_box), 1);

    return uiControl(vbox);
}

/* Create the main UI */
void create_ui(void) {
    /* Create main window */
    app.main_window = uiNewWindow("DOSEmu2 GUI", 600, 400, 1);

    /* Create main vertical box */
    app.main_vbox = uiNewVerticalBox();
    uiBoxSetPadded(app.main_vbox, 1);

    /* Create tab container */
    app.main_tab = uiNewTab();

    /* Add tabs */
    uiTabAppend(app.main_tab, "Control", create_control_tab());
    uiTabAppend(app.main_tab, "Configuration", create_config_tab());

    /* Add tab container to main box */
    uiBoxAppend(app.main_vbox, uiControl(app.main_tab), 1);

    /* Set main box as window content */
    uiWindowSetChild(app.main_window, uiControl(app.main_vbox));
    uiWindowSetMargined(app.main_window, 1);
}

/* UI callback implementations */
void on_start_button_clicked(uiButton *button, void *data) {
    const char *dos_path = uiEntryText(app.dos_path_entry);
    const char *config_path = uiEntryText(app.config_path_entry);

    if (start_dosemu(dos_path, config_path)) {
        uiControlDisable(uiControl(app.start_button));
        uiControlEnable(uiControl(app.stop_button));
    } else {
        uiMsgBoxError(app.main_window, "Error", "Failed to start DOSEmu");
    }
}

void on_stop_button_clicked(uiButton *button, void *data) {
    if (stop_dosemu()) {
        uiControlEnable(uiControl(app.start_button));
        uiControlDisable(uiControl(app.stop_button));
    } else {
        uiMsgBoxError(app.main_window, "Error", "Failed to stop DOSEmu");
    }
}

void on_browse_dos_path_clicked(uiButton *button, void *data) {
    char *filename = uiOpenFile(app.main_window);
    if (filename != NULL) {
        uiEntrySetText(app.dos_path_entry, filename);
        uiFreeText(filename);
    }
}

void on_browse_config_path_clicked(uiButton *button, void *data) {
    char *filename = uiOpenFile(app.main_window);
    if (filename != NULL) {
        uiEntrySetText(app.config_path_entry, filename);
        uiFreeText(filename);
    }
}
