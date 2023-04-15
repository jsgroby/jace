// jace.c

#include "stdio.h"
#include "defs.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"


int main(int argc, char *argv[]) {

	// print date and time - add function to misc.c for this to handle all systems
	int numofcpus = getNumberOfCores();
	printf("%d physical cores detected\n",numofcpus);
	
	AllInit();

	S_BOARD pos[1];
    S_SEARCHINFO info[1];
    info->quit = FALSE;
	pos->HashTable->pTable = NULL;
    InitHashTable(pos->HashTable, 64);
	setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    
    int ArgNum = 0;
    
    for(ArgNum = 0; ArgNum < argc; ++ArgNum) {
    	if(strncmp(argv[ArgNum], "NoBook", 6) == 0) {
    		EngineOptions->UseBook = FALSE;
    		printf("Book Off\n");
    	}
    }
	printf("\n");
	printf("\"How about a nice game of chess?\"\n\n");
	printf("Welcome to Jace! Type 'jace' for console mode...\n");

	char line[256];
	while (TRUE) {
		memset(&line[0], 0, sizeof(line));

		fflush(stdout);
		if (!fgets(line, 256, stdin))
			continue;
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
			printf("FEN: ");
			fgets(fenstring, 80, stdin);
			printf("Depth: ");
			scanf("%d", &depth);
			S_BOARD perftpos[1];
			ParseFen(fenstring,perftpos);
			printf("Testing position: %s",fenstring);
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
