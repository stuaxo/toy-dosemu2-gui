/* Add this at the very top of the file, before any includes */
#define _XOPEN_SOURCE 500

#include "../include/dosemu_integration.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>  /* For va_list, va_start, va_end */
#include <limits.h>  /* For PATH_MAX */
#include <subprocess.h>

/* Process handles */
static struct subprocess_s dosemu_process;
static struct subprocess_s dosdebug_process;

/* Flags to track if processes are running */
static int dosemu_running = 0;
static int dosdebug_running = 0;

/* Buffer for reading dosdebug output */
#define BUFFER_SIZE 4096
static char read_buffer[BUFFER_SIZE];

/* Add log entry to both console and stdout */
static void log_message(const char *format, ...) {
    va_list args, args_copy;
    va_start(args, format);

    /* Copy args for reuse */
    va_copy(args_copy, args);

    /* Print to stdout */
    vfprintf(stdout, format, args);
    fflush(stdout);

    /* Print to app console if available */
    if (app.console) {
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args_copy);
        uiMultilineEntryAppend(app.console, buffer);
    }

    va_end(args_copy);
    va_end(args);
}

/* Add error log entry to both console and stderr */
static void log_error(const char *format, ...) {
    va_list args, args_copy;
    va_start(args, format);

    /* Copy args for reuse */
    va_copy(args_copy, args);

    /* Print to stderr */
    vfprintf(stderr, format, args);
    fflush(stderr);

    /* Print to app console if available */
    if (app.console) {
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args_copy);
        uiMultilineEntryAppend(app.console, buffer);
    }

    va_end(args_copy);
    va_end(args);
}

/* Start DOSEmu with the given paths */
bool start_dosemu(const char *dos_path, const char *config_path) {
    int result;
    struct timespec ts;

    if (is_dosemu_running()) {
        log_error("DOSEmu is already running\n");
        return false;
    }

    /* Prepare arguments for dosemu */
    int use_default_config = !config_path || !*config_path ||
                             strcmp(config_path, "~/.dosemurc") == 0;

    /* Build the command array */
    const char *dosemu_command[4];
    dosemu_command[0] = dos_path;

    if (use_default_config) {
        dosemu_command[1] = NULL;
        log_message("Executing: %s (with default config)\n", dos_path);
    } else {
        dosemu_command[1] = "-f";
        dosemu_command[2] = config_path;
        dosemu_command[3] = NULL;
        log_message("Executing: %s %s %s\n", dos_path, dosemu_command[1], dosemu_command[2]);
    }

    /* Verify dosemu executable exists and is executable */
    if (access(dos_path, X_OK) != 0) {
        log_error("Cannot execute %s: %s\n", dos_path, strerror(errno));
        return false;
    }

    /* Launch dosemu process */
    result = subprocess_create(
        dosemu_command,          /* Command array */
        subprocess_option_inherit_environment | subprocess_option_no_window |
        subprocess_option_enable_async,  /* Make sure it's async */
        &dosemu_process          /* Process handle */
    );

    if (result != 0) {
        log_error("Failed to start DOSEmu: %s\n", strerror(errno));
        return false;
    }

    dosemu_running = 1;
    /* Don't join with the process as that would block */
    log_message("Started DOSEmu\n");

    /* Wait for dosemu to initialize before starting dosdebug */
    ts.tv_sec = 0;
    ts.tv_nsec = 500000000; /* 500 milliseconds */
    nanosleep(&ts, NULL);

    log_message("Waited 500ms for DOSEmu initialization\n");

    /* Check if dosemu is still running after the initial wait */
    if (!subprocess_alive(&dosemu_process)) {
        log_error("DOSEmu terminated unexpectedly during initialization\n");
        dosemu_running = 0;
        subprocess_destroy(&dosemu_process);
        return false;
    }

    /* Now start dosdebug using the known path */
    const char *dosdebug_path = "/usr/bin/dosdebug";

    /* Verify dosdebug executable exists and is executable */
    if (access(dosdebug_path, X_OK) != 0) {
        log_error("Cannot execute %s: %s\n", dosdebug_path, strerror(errno));

        /* Kill dosemu since we can't control it without dosdebug */
        subprocess_terminate(&dosemu_process);
        subprocess_destroy(&dosemu_process);
        dosemu_running = 0;
        return false;
    }

    log_message("Using dosdebug at: %s\n", dosdebug_path);

    const char *dosdebug_command[] = {dosdebug_path, NULL};

    log_message("Starting dosdebug\n");
    result = subprocess_create(
        dosdebug_command,        /* Command array */
        subprocess_option_inherit_environment | subprocess_option_no_window |
        subprocess_option_enable_async,
        &dosdebug_process        /* Process handle */
    );

    if (result != 0) {
        log_error("Failed to start dosdebug: %s (errno: %d)\n", strerror(errno), errno);
        /* Double-check file existence */
        if (access(dosdebug_path, F_OK) != 0) {
            log_error("File does not exist: %s\n", dosdebug_path);
        } else if (access(dosdebug_path, X_OK) != 0) {
            log_error("File exists but is not executable: %s\n", dosdebug_path);
        }

        /* Kill dosemu since we can't control it without dosdebug */
        subprocess_terminate(&dosemu_process);
        subprocess_destroy(&dosemu_process);
        dosemu_running = 0;
        return false;
    }

    dosdebug_running = 1;
    log_message("Started dosdebug\n");

    /* Give dosdebug time to initialize */
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000; /* 100 milliseconds */
    nanosleep(&ts, NULL);

    /* Read initial output from dosdebug */
    FILE *dosdebug_stdout = subprocess_stdout(&dosdebug_process);
    if (dosdebug_stdout) {
        /* Try to read any initial output */
        size_t bytes_read = 0;
        memset(read_buffer, 0, BUFFER_SIZE);

        /* Try multiple reads with short delays to get complete output */
        for (int i = 0; i < 5; i++) {
            if (fgets(read_buffer + bytes_read, BUFFER_SIZE - bytes_read, dosdebug_stdout)) {
                size_t len = strlen(read_buffer + bytes_read);
                bytes_read += len;

                /* Stop if we got the prompt or buffer is nearly full */
                if (strstr(read_buffer, "dosdebug> ") || bytes_read >= BUFFER_SIZE - 100) {
                    break;
                }
            }

            /* Small delay between reads */
            ts.tv_sec = 0;
            ts.tv_nsec = 50000000; /* 50 milliseconds */
            nanosleep(&ts, NULL);
        }

        if (bytes_read > 0) {
            log_message("Initial dosdebug output:\n%s", read_buffer);
        }
    }

    /* Send '?' command to verify connection and list available commands */
    log_message("Verifying dosdebug connection...\n");

    FILE *dosdebug_stdin = subprocess_stdin(&dosdebug_process);
    if (dosdebug_stdin) {
        fputs("?\n", dosdebug_stdin);
        fflush(dosdebug_stdin);
        log_message("Sent to dosdebug: ?\n");

        /* Read the help text */
        if (dosdebug_stdout) {
            size_t bytes_read = 0;
            memset(read_buffer, 0, BUFFER_SIZE);

            /* Try multiple reads with delays to get complete help text */
            for (int i = 0; i < 10; i++) {
                if (fgets(read_buffer + bytes_read, BUFFER_SIZE - bytes_read, dosdebug_stdout)) {
                    size_t len = strlen(read_buffer + bytes_read);
                    bytes_read += len;

                    /* Stop if we got the prompt again or buffer is nearly full */
                    if (strstr(read_buffer + bytes_read - len, "dosdebug> ") ||
                        bytes_read >= BUFFER_SIZE - 100) {
                        break;
                    }
                }

                /* Small delay between reads */
                ts.tv_sec = 0;
                ts.tv_nsec = 100000000; /* 100 milliseconds */
                nanosleep(&ts, NULL);
            }

            if (bytes_read > 0) {
                log_message("From dosdebug:\n%s", read_buffer);
            }
        }
    }

    return true;
}

/* Stop the dosemu process using dosdebug */
bool stop_dosemu(void) {
    struct timespec ts;

    if (!is_dosemu_running()) {
        log_error("DOSEmu is not running\n");
        return false;
    }

    /* Send kill command to terminate dosemu */
    log_message("Sending kill command to terminate DOSEmu\n");

    FILE *dosdebug_stdin = subprocess_stdin(&dosdebug_process);
    if (dosdebug_stdin) {
        fputs("kill\n", dosdebug_stdin);
        fflush(dosdebug_stdin);
        log_message("Sent to dosdebug: kill\n");

        /* Read any output from the kill command */
        FILE *dosdebug_stdout = subprocess_stdout(&dosdebug_process);
        if (dosdebug_stdout) {
            memset(read_buffer, 0, BUFFER_SIZE);
            if (fgets(read_buffer, BUFFER_SIZE, dosdebug_stdout)) {
                log_message("From dosdebug:\n%s", read_buffer);
            }
        }
    }

    /* Wait briefly to ensure dosemu has time to terminate */
    ts.tv_sec = 0;
    ts.tv_nsec = 500000000; /* 500 milliseconds */
    nanosleep(&ts, NULL);

    /* Send quit command to exit the debug session */
    log_message("Sending quit command to exit debug session\n");

    if (dosdebug_stdin) {
        fputs("quit\n", dosdebug_stdin);
        fflush(dosdebug_stdin);
        log_message("Sent to dosdebug: quit\n");

        /* Read any output from the quit command */
        FILE *dosdebug_stdout = subprocess_stdout(&dosdebug_process);
        if (dosdebug_stdout) {
            memset(read_buffer, 0, BUFFER_SIZE);
            if (fgets(read_buffer, BUFFER_SIZE, dosdebug_stdout)) {
                log_message("From dosdebug:\n%s", read_buffer);
            }
        }
    }

    /* Terminate and clean up dosdebug */
    if (dosdebug_running) {
        /* Send Ctrl-C to dosdebug */
        if (dosdebug_stdin) {
            char ctrlc = 3;  /* ASCII code for Ctrl-C */
            fputc(ctrlc, dosdebug_stdin);
            fflush(dosdebug_stdin);
        }

        /* Wait for dosdebug to terminate, but don't wait too long */
        int return_code;
        subprocess_join(&dosdebug_process, &return_code);

        /* Check if the process is still running after a brief wait */
        ts.tv_sec = 1;  /* Wait for 1 second */
        ts.tv_nsec = 0;
        nanosleep(&ts, NULL);

        if (subprocess_alive(&dosdebug_process)) {
            /* Process is still running, force terminate */
            subprocess_terminate(&dosdebug_process);
        }

        subprocess_destroy(&dosdebug_process);
        dosdebug_running = 0;
    }

    /* If dosemu is somehow still running, terminate it directly */
    if (subprocess_alive(&dosemu_process)) {
        log_error("DOSEmu didn't terminate via dosdebug, terminating directly\n");
        subprocess_terminate(&dosemu_process);
    }

    /* Clean up dosemu process */
    subprocess_destroy(&dosemu_process);
    dosemu_running = 0;

    log_message("DOSEmu terminated\n");
    return true;
}

/* Check if DOSEmu is currently running */
bool is_dosemu_running(void) {
    if (!dosemu_running) {
        return false;
    }

    /* Check if the process is still alive */
    if (!subprocess_alive(&dosemu_process)) {
        /* Process has terminated */
        dosemu_running = 0;

        /* If dosdebug is still running, terminate it */
        if (dosdebug_running) {
            log_message("DOSEmu has terminated, closing dosdebug\n");

            /* Send quit command */
            FILE *dosdebug_stdin = subprocess_stdin(&dosdebug_process);
            if (dosdebug_stdin) {
                fputs("quit\n", dosdebug_stdin);
                fflush(dosdebug_stdin);
            }

            /* Terminate and clean up */
            subprocess_terminate(&dosdebug_process);
            subprocess_destroy(&dosdebug_process);
            dosdebug_running = 0;
        }

        return false;
    }

    return true;
}
