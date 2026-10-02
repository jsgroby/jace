// jace.c

#include "stdio.h"
#include "defs.h"
#include "stdlib.h"
#include "string.h"
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif



int main(int argc, char *argv[]) {

	// print date and time - add function to misc.c for this to handle all systems
	int numofcpus = getNumberOfCores();
	fprintf(stderr,"%d logical cores detected\n",numofcpus);
	
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
    		fprintf(stderr,"Book Off\n");
    	}
    }
	fprintf(stderr,"\n");
	fprintf(stderr,"\"How about a nice game of chess?\"\n\n");
	fprintf(stderr,"Welcome to Jace! Type 'jace' for console mode...\n");

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
		} else if (!strncmp(line, "xboard",6))	{
			XBoard_Loop(pos, info);
			if(info->quit == TRUE) break;
			continue;
		} else if (!strncmp(line, "jace",4))	{
			Console_Loop(pos, info);
			if(info->quit == TRUE) break;
			continue;
		} else if (!strncmp(line, "perft",5))	{
			char fenstring[80];
			int depth;
			fprintf(stderr,"FEN: ");
			fgets(fenstring, 80, stdin);
			fprintf(stderr,"Depth: ");
			scanf("%d", &depth);
			S_BOARD perftpos[1] = {0};
			ParseFen(fenstring,perftpos);
			fprintf(stderr,"Testing position: %s",fenstring);
			PerftTest(depth,perftpos);
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
	free(pos->HashTable->pTable);
	CleanPolyBook();
	return 0;
}
