// misc.c

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "defs.h"

#ifdef WIN32

#include <windows.h>

#else

#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>

#ifdef MACOS
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#endif


/*
 * Return the number of logical processors available to the engine.
 */
int getNumberOfCores(void)
{
#ifdef WIN32

    SYSTEM_INFO sysinfo;

    GetSystemInfo(&sysinfo);

    if (sysinfo.dwNumberOfProcessors < 1) {
        return 1;
    }

    return (int)sysinfo.dwNumberOfProcessors;

#elif defined(MACOS)

    int nm[2];
    size_t len;
    uint32_t count = 0;

    nm[0] = CTL_HW;
    nm[1] = HW_AVAILCPU;
    len = sizeof(count);

    if (sysctl(nm, 2, &count, &len, NULL, 0) != 0 || count < 1) {
        nm[1] = HW_NCPU;
        len = sizeof(count);
        count = 0;

        if (sysctl(nm, 2, &count, &len, NULL, 0) != 0 || count < 1) {
            count = 1;
        }
    }

    return (int)count;

#else

    long cores = sysconf(_SC_NPROCESSORS_ONLN);

    if (cores < 1) {
        return 1;
    }

    return (int)cores;

#endif
}


/*
 * Return the current time in milliseconds.
 */
int GetTimeMs(void)
{
#ifdef WIN32

    return (int)GetTickCount();

#else

    struct timeval t;

    gettimeofday(&t, NULL);

    return (int)(
        (t.tv_sec * 1000L) +
        (t.tv_usec / 1000L)
    );

#endif
}


/*
 * Check whether input is waiting on stdin.
 */
int InputWaiting(void)
{
#ifndef WIN32

    fd_set readfds;
    struct timeval tv;
    int result;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    result = select(
        STDIN_FILENO + 1,
        &readfds,
        NULL,
        NULL,
        &tv
    );

    if (result <= 0) {
        return 0;
    }

    return FD_ISSET(STDIN_FILENO, &readfds) ? 1 : 0;

#else

    static int init = 0;
    static int pipe;
    static HANDLE inh;
    DWORD dw;

    if (!init) {
        init = 1;

        inh = GetStdHandle(STD_INPUT_HANDLE);

        pipe = !GetConsoleMode(inh, &dw);

        if (!pipe) {
            SetConsoleMode(
                inh,
                dw & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT)
            );

            FlushConsoleInputBuffer(inh);
        }
    }

    if (pipe) {

        if (!PeekNamedPipe(
                inh,
                NULL,
                0,
                NULL,
                &dw,
                NULL)) {
            return 1;
        }

        return (dw != 0) ? 1 : 0;

    } else {

        GetNumberOfConsoleInputEvents(inh, &dw);

        return (dw <= 1) ? 0 : 1;
    }

#endif
}


/*
 * Read commands from stdin while the engine is searching.
 *
 * This allows commands such as "quit" to interrupt the search.
 */
void ReadInput(S_SEARCHINFO *info)
{
    char input[256] = "";
    char *endc;

#ifndef WIN32

    ssize_t bytes;

#endif

    if (info == NULL) {
        return;
    }

    if (!InputWaiting()) {
        return;
    }

    info->stopped = TRUE;

#ifndef WIN32

    do {

        bytes = read(
            STDIN_FILENO,
            input,
            sizeof(input) - 1
        );

    } while (bytes < 0);

    if (bytes > 0) {

        input[(size_t)bytes] = '\0';

    } else {

        input[0] = '\0';
    }

#else

    if (fgets(input, sizeof(input), stdin) == NULL) {
        input[0] = '\0';
    }

#endif

    endc = strchr(input, '\n');

    if (endc != NULL) {
        *endc = '\0';
    }

    endc = strchr(input, '\r');

    if (endc != NULL) {
        *endc = '\0';
    }

    if (input[0] != '\0') {

        if (strncmp(input, "quit", 4) == 0) {
            info->quit = TRUE;
        }
    }
}
