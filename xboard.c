// xboard.c

#include "stdio.h"
#include "defs.h"
#include "string.h"

static int ThreeFoldRep(const S_BOARD *pos) {

	ASSERT(CheckBoard(pos));

	int i = 0, r = 0;
	for (i = 0; i < pos->hisPly; ++i)	{
	    if (pos->history[i].posKey == RepetitionKey(pos)) {
		    r++;
		}
	}
	return r;
}

static int checkresult(S_BOARD *pos) {
 if(!HasLegalMove(pos)) {
  if(SqAttacked(pos->KingSq[pos->side],pos->side^1,pos)) printf("%s {checkmate}\n",pos->side==WHITE?"0-1":"1-0");
  else printf("1/2-1/2 {stalemate}\n");
  return TRUE;
 }
 if(pos->fiftyMove>=100 || ThreeFoldRep(pos)>=2 || DeadMaterial(pos)) {printf("1/2-1/2 {draw}\n");return TRUE;}
 return FALSE;
}

static void PrintOptions(void) {
	printf("feature ping=1 setboard=1 colors=0 usermove=1 memory=1\n");
	printf("feature done=1\n");
}

void XBoard_Loop(S_BOARD *pos, S_SEARCHINFO *info) {

	info->GAME_MODE = XBOARDMODE;
	info->nodeLimit = 0;
	info->POST_THINKING = TRUE;
	setbuf(stdin, NULL);
    setbuf(stdout, NULL);
	PrintOptions(); // HACK

	int depth = -1, movestogo[2] = {30,30 }, movetime = -1;
	int time = -1, inc = 0;
	int engineSide = BOTH;
	int timeLeft = 0;
	int sec = 0;
	int mps = 0;
	int move = NOMOVE;
	char inBuf[80], command[80];
	int MB = 64;

	engineSide = BLACK;
	ParseFen(START_FEN, pos);
	depth = -1;
	time = -1;

	while(TRUE) {

		fflush(stdout);

		if(pos->side == engineSide && checkresult(pos) == FALSE) {
			info->starttime = GetTimeMs();
			info->depth = depth;
			info->timeset = FALSE;

			if(time >= 0 || movetime >= 0) {
                int moves=movestogo[pos->side]>0?movestogo[pos->side]:30;
                int64_t budget=movetime>=0?(int64_t)movetime*1000:(int64_t)time/moves+(int64_t)inc*750;
                if(movetime<0 && budget>time-10) budget=time-10;
                if(budget<1) budget=1;
                info->timeset=TRUE;info->stoptime=info->starttime+budget;
            }

			if(depth == -1 || depth > MAXDEPTH) {
				info->depth = MAXDEPTH;
			}

			fprintf(stderr,"time:%d start:%" PRId64 " stop:%" PRId64 " depth:%d timeset:%d movestogo:%d mps:%d\n",
				time,info->starttime,info->stoptime,info->depth,info->timeset, movestogo[pos->side], mps);
				SearchPosition(pos, info);
			if(info->quit) break;

			if(mps != 0) {
				movestogo[pos->side^1]--;
				if(movestogo[pos->side^1] < 1) {
					movestogo[pos->side^1] = mps;
				}
			}

		}

		fflush(stdout);

		memset(&inBuf[0], 0, sizeof(inBuf));
		fflush(stdout);
		if (!NextInputLine(inBuf, sizeof(inBuf))) break;

		if(sscanf(inBuf, "%79s", command)!=1) continue;

		fprintf(stderr,"command seen:%s\n",inBuf);

		if(!strcmp(command, "quit")) {
			info->quit = TRUE;
			break;
		}

		if(!strcmp(command, "force")) {
			engineSide = BOTH;
			continue;
		}

		if(!strcmp(command, "protover")){
			PrintOptions();
		    continue;
		}

		if(!strcmp(command, "sd")) {
			sscanf(inBuf, "sd %d", &depth);
		    fprintf(stderr,"DEBUG depth:%d\n",depth);
			continue;
		}

		if(!strcmp(command, "st")) {
			sscanf(inBuf, "st %d", &movetime);
		    fprintf(stderr,"DEBUG movetime:%d\n",movetime);
			continue;
		}

		if(!strcmp(command, "time")) {
			sscanf(inBuf, "time %d", &time);
			time *= 10;
		    fprintf(stderr,"DEBUG time:%d\n",time);
			continue;
		}
		
		if(!strcmp(command, "memory")) {			
			sscanf(inBuf, "memory %d", &MB);		
		    if(MB < 4) MB = 4;
			if(MB > MAX_HASH) MB = MAX_HASH;
			fprintf(stderr,"Set Hash to %d MB\n",MB);
			InitHashTable(pos->HashTable, MB);
			continue;
		}

		if(!strcmp(command, "level")) {
			sec = 0;
			movetime = -1;
			if( sscanf(inBuf, "level %d %d %d", &mps, &timeLeft, &inc) != 3) {
			  sscanf(inBuf, "level %d %d:%d %d", &mps, &timeLeft, &sec, &inc);
		      fprintf(stderr,"DEBUG level with :\n");
			}	else {
		      fprintf(stderr,"DEBUG level without :\n");
			}
			timeLeft *= 60000;
			timeLeft += sec * 1000;
			movestogo[0] = movestogo[1] = 30;
			if(mps != 0) {
				movestogo[0] = movestogo[1] = mps;
			}
			time = timeLeft;
		    fprintf(stderr,"DEBUG level timeLeft:%d movesToGo:%d inc:%d mps%d\n",timeLeft,movestogo[0],inc,mps);
			continue;
		}

		if(!strcmp(command, "ping")) {
			printf("pong%s\n", inBuf+4);
			continue;
		}

		if(!strcmp(command, "new")) {
			ClearHashTable(pos->HashTable);
			engineSide = BLACK;
			ParseFen(START_FEN, pos);
			depth = -1;
			time = -1;
			continue;
		}

		if(!strcmp(command, "setboard")){
			engineSide = BOTH;
			ParseFen(inBuf+9, pos);
			continue;
		}

		if(!strcmp(command, "go")) {
			engineSide = pos->side;
			continue;
		}

		if(!strcmp(command, "usermove")){
			movestogo[pos->side]--;
			move = ParseMove(inBuf+9, pos);
			if(move == NOMOVE) continue;
			MakeMove(pos, move);
            pos->ply=0;
		}
    }
}


void Console_Loop(S_BOARD *pos, S_SEARCHINFO *info) {

	printf("Welcome to Jace In Console Mode!\n");
	printf("Type help for commands\n\n");

	info->GAME_MODE = CONSOLEMODE;
	info->nodeLimit = 0;
	info->POST_THINKING = TRUE;
	setbuf(stdin, NULL);
    setbuf(stdout, NULL);

	int depth = MAXDEPTH, movetime = 3000;
	int engineSide = BOTH;
	int move = NOMOVE;
	char inBuf[80], command[80];

	engineSide = BLACK;
	ParseFen(START_FEN, pos);

	while(TRUE) {

		fflush(stdout);

		if(pos->side == engineSide && checkresult(pos) == FALSE) {
			info->starttime = GetTimeMs();
			info->depth = depth;

			info->timeset = FALSE;
			if(movetime != 0) {
				info->timeset = TRUE;
				info->stoptime = info->starttime + movetime;
			}

			SearchPosition(pos, info);
			if(info->quit) break;
		}

		printf("\nJace > ");

		fflush(stdout);

		memset(&inBuf[0], 0, sizeof(inBuf));
		fflush(stdout);
		if (!NextInputLine(inBuf, sizeof(inBuf))) break;

		if(sscanf(inBuf, "%79s", command)!=1) continue;

		if(!strcmp(command, "help")) {
			printf("Commands:\n");
			printf("quit - quit game\n");
			printf("force - computer will not think\n");
			printf("print - show board\n");
			printf("post - show thinking\n");
			printf("nopost - do not show thinking\n");
			printf("new - start new game\n");
			printf("go - set computer thinking\n");
			printf("depth x - set depth to x\n");
			printf("time x - set thinking time to x seconds (depth still applies if set)\n");
			printf("view - show current depth and movetime settings\n");
			printf("setboard x - set position to fen x\n");
			printf("** note ** - to reset time and depth, set to 0\n");
			printf("enter moves using b7b8q notation\n\n\n");
			continue;
		}

		if(!strcmp(command, "mirror")) {
			engineSide = BOTH;
			MirrorEvalTest(pos);
			continue;
		}

		if(!strcmp(command, "eval")) {
			PrintBoard(pos);
			printf("Eval:%d",EvalPosition(pos));
			MirrorBoard(pos);
			PrintBoard(pos);
			printf("Eval:%d",EvalPosition(pos));
			continue;
		}

		if(!strcmp(command, "setboard")){
			engineSide = BOTH;
			ParseFen(inBuf+9, pos);
			continue;
		}

		if(!strcmp(command, "quit")) {
			info->quit = TRUE;
			break;
		}

		if(!strcmp(command, "post")) {
			info->POST_THINKING = TRUE;
			continue;
		}

		if(!strcmp(command, "print")) {
			PrintBoard(pos);
			continue;
		}

		if(!strcmp(command, "nopost")) {
			info->POST_THINKING = FALSE;
			continue;
		}

		if(!strcmp(command, "force")) {
			engineSide = BOTH;
			continue;
		}

		if(!strcmp(command, "view")) {
			if(depth == MAXDEPTH) printf("depth not set ");
			else printf("depth %d",depth);

			if(movetime != 0) printf(" movetime %ds\n",movetime/1000);
			else printf(" movetime not set\n");

			continue;
		}

		if(!strcmp(command, "depth")) {
			sscanf(inBuf, "depth %d", &depth);
		    if(depth==0) depth = MAXDEPTH;
			continue;
		}

		if(!strcmp(command, "time")) {
			sscanf(inBuf, "time %d", &movetime);
			movetime *= 1000;
			continue;
		}

		if(!strcmp(command, "new")) {
			ClearHashTable(pos->HashTable);
			engineSide = BLACK;
			ParseFen(START_FEN, pos);
			continue;
		}

		if(!strcmp(command, "go")) {
			engineSide = pos->side;
			continue;
		}

		move = ParseMove(inBuf, pos);
		if(move == NOMOVE) {
			printf("Command unknown:%s\n",inBuf);
			continue;
		}
		MakeMove(pos, move);
		pos->ply=0;
    }
}
