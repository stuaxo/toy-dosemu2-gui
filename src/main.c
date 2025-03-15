#include "../include/common.h"
#include "../include/ui_main.h"

/* Global application state */
AppState app = {0};

static int on_closing(uiWindow *w, void *data) {
    uiQuit();
    return 1;
}

static int on_should_quit(void *data) {
    uiWindow *window = app.main_window;
    uiControlDestroy(uiControl(window));
    return 1;
}

int main(int argc, char **argv) {
    uiInitOptions options;
    const char *err;

    /* Initialize application state */
    memset(&app, 0, sizeof(AppState));

    /* Initialize libui */
    memset(&options, 0, sizeof(uiInitOptions));
    err = uiInit(&options);
    if (err != NULL) {
        fprintf(stderr, "Error initializing libui: %s\n", err);
        uiFreeInitError(err);
        return 1;
    }

    /* Set up UI quit handler */
    uiOnShouldQuit(on_should_quit, NULL);

    /* Create main window and UI */
    create_ui();

    /* Set window close handler */
    uiWindowOnClosing(app.main_window, on_closing, NULL);

    /* Show the window */
    uiControlShow(uiControl(app.main_window));

    /* Run the UI loop */
    uiMain();

    /* Clean up */
    uiUninit();

    return 0;
}
