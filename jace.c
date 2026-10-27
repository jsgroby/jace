// jace.c

#include "stdio.h"
#include "defs.h"
#include "stdlib.h"
#include "string.h"
#include <errno.h>
#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif



int main(int argc, char *argv[]) {

	
	AllInit();

	S_BOARD pos[1] = {0};
    S_SEARCHINFO info[1] = {0};
    info->quit = FALSE;
	pos->HashTable->pTable = NULL;
    InitHashTable(pos->HashTable, 64);
	ParseFen(START_FEN, pos);
#ifdef _WIN32
    /* Keep CRT CRLF lookahead from hiding bytes from PeekNamedPipe.
       ReadChar already handles CR and LF explicitly. */
    if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
        perror("stdin binary mode");
        return EXIT_FAILURE;
    }
#endif
	setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    
    int ArgNum = 0;
    
    for(ArgNum = 0; ArgNum < argc; ++ArgNum) {
    	if(strncmp(argv[ArgNum], "NoBook", 6) == 0) {
    		EngineOptions->UseBook = FALSE;
    	}
    }

	char line[256];
	while (TRUE) {
		memset(&line[0], 0, sizeof(line));

		fflush(stdout);
		if (!NextInputLine(line, sizeof(line))) break;
		if (line[0] == '\n')
			continue;
		if (!strncmp(line, "uci",3)) {
			Uci_Loop(pos, info);
			if(info->quit == TRUE) break;
			continue;
		} else if (!strncmp(line, "jace",4))	{
			Console_Loop(pos, info);
			if(info->quit == TRUE) break;
			continue;
		} else if (!strncmp(line, "perft",5))	{
            char fenstring[256], depthline[256], *end;
            fprintf(stderr,"FEN: ");
            if (!NextInputLine(fenstring, sizeof fenstring)) break;
            S_BOARD perftpos[1] = {0};
            if (ParseFen(fenstring, perftpos) != 0) {
                fprintf(stderr,"Invalid FEN\n");
                continue;
            }
            fprintf(stderr,"Depth: ");
            if (!NextInputLine(depthline, sizeof depthline)) break;
            errno = 0;
            long depth = strtol(depthline, &end, 10);
            int hasNumber = end != depthline;
            while (isspace((unsigned char)*end)) end++;
            if (errno == ERANGE || !hasNumber || *end || depth < 1 || depth >= MAXDEPTH) {
                fprintf(stderr,"Invalid perft depth (expected 1..%d)\n", MAXDEPTH-1);
                continue;
            }
            fprintf(stderr,"Testing position: %s\n",fenstring);
            PerftTest((int)depth,perftpos);
			continue;
		} else if (!strncmp(line, "debug",5))	{
			DebugAnalysisTest(pos,info);
			break;
		} else if (!strncmp(line, "mirror",6))	{
			MirrorEvalTest(pos);
			break;
		} else if(!strncmp(line, "quit",4))	{
			break;
		}
	}
	FreeHashTable(pos->HashTable);
	CleanPolyBook();
 SyzygyFree();
	return 0;
}
