// misc.c

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>

#include "defs.h"

#ifdef _WIN32

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
#ifdef _WIN32

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
int64_t GetTimeMs(void)
{
#ifdef _WIN32

    return (int64_t)GetTickCount64();

#else

    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000;

#endif
}


/*
 * Check whether input is waiting on stdin.
 */
static int InputWaiting(void)
{
#ifndef _WIN32

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
                dw & ~(DWORD)(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT)
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

#define INPUT_SIZE 4096
#define INPUT_QUEUE 64
static char queued[INPUT_QUEUE][INPUT_SIZE];
static unsigned head, tail, pending;
static char partial[INPUT_SIZE];
static size_t used;
static int inputEof, inputOverflow;

static void AcceptLine(S_SEARCHINFO *info) {
 /* UCI allows arbitrary whitespace between tokens. Normalize complete lines. */
 size_t out=0;int gap=FALSE;
 for(size_t i=0;i<used;i++) {
  if(isspace((unsigned char)partial[i])) {if(out) gap=TRUE;}
  else {if(gap) partial[out++]=' ';partial[out++]=partial[i];gap=FALSE;}
 }
 used=out;partial[used]='\0';
 if(!inputOverflow && used) {
  if(info && strcmp(partial,"isready")==0) {printf("readyok\n");fflush(stdout);}
  else if(info && strcmp(partial,"ponderhit")==0) {
   if(info->pondering) {
    info->pondering=FALSE;
    if(info->ponderBudget>0 && !info->infinite) {
     info->timeset=TRUE;info->stoptime=GetTimeMs()+info->ponderBudget;
    }
   }
  }
  else if(info && strcmp(partial,"stop")==0) {info->stopped=TRUE;info->pondering=info->infinite=FALSE;}
  else if(info && strcmp(partial,"quit")==0) {info->stopped=TRUE;info->quit=TRUE;}
  else {memcpy(queued[tail],partial,used+1);tail=(tail+1)%INPUT_QUEUE;pending++;if(info) {info->stopped=TRUE;info->pondering=info->infinite=FALSE;}}
 }
 used=0;inputOverflow=0;
}
static void ReadChar(S_SEARCHINFO *info) {
 int c=fgetc(stdin);
 if(c==EOF) {inputEof=TRUE;if(used) AcceptLine(info);if(info) {info->stopped=TRUE;info->quit=TRUE;}return;}
 if(c=='\n') AcceptLine(info);
 else if(c!='\r') {if(used+1<INPUT_SIZE) partial[used++]=(char)c;else inputOverflow=TRUE;}
}
int NextInputLine(char *line,size_t size) {
 while(!pending && !inputEof) ReadChar(NULL);
 if(!pending || !size) return FALSE;
 snprintf(line,size,"%s",queued[head]);head=(head+1)%INPUT_QUEUE;pending--;return TRUE;
}
void ReadInput(S_SEARCHINFO *info) {
 if(!info) return;
 /* Existing queued commands require returning to the protocol loop. */
 if(pending) {info->stopped=TRUE;return;}
 while(!inputEof && pending<INPUT_QUEUE && InputWaiting()) ReadChar(info);
}

/* Search has reached a depth/node/mate limit. Block without spinning until
   stop/ponderhit/quit, still answering readiness and preserving queued input. */
void WaitForSearchSignal(S_SEARCHINFO *info) {
 while((info->pondering || info->infinite) && !info->quit) {
  if(pending) {info->stopped=TRUE;info->pondering=info->infinite=FALSE;break;}
  ReadChar(info);
 }
}
