#include "defs.h"
#include <string.h>
#include <limits.h>

static long long Number(const char *line,const char *token,long long fallback) {
 const char *p=strstr(line,token);
 if(!p) return fallback;
 char *end;long long n=strtoll(p+strlen(token),&end,10);
 return end==p+strlen(token)?fallback:n;
}
static int GoKeyword(const char *s) {
 const char *keys[]={"searchmoves","ponder","wtime","btime","winc","binc",
                     "movestogo","depth","nodes","mate","movetime","infinite"};
 for(size_t i=0;i<sizeof keys/sizeof keys[0];i++) if(!strcmp(s,keys[i])) return TRUE;
 return FALSE;
}
static void GoControls(const char *line,S_SEARCHINFO *info,S_BOARD *pos) {
 char copy[4096];snprintf(copy,sizeof copy,"%s",line);
 info->restricted=info->searchMoveCount=info->excludedCount=0;
 info->pondering=info->infinite=FALSE;
 int collecting=FALSE;
 for(char *token=strtok(copy," \t\r\n");token;token=strtok(NULL," \t\r\n")) {
  if(GoKeyword(token)) collecting=FALSE;
  if(!strcmp(token,"ponder")) info->pondering=TRUE;
  else if(!strcmp(token,"infinite")) info->infinite=TRUE;
  else if(!strcmp(token,"searchmoves")) {info->restricted=TRUE;collecting=TRUE;}
  else if(collecting) {
   int move=NOMOVE;size_t n=strlen(token);
   if(n==4 || n==5) move=ParseMove(token,pos);
   if(move==NOMOVE || strcmp(PrMove(move),token) || !MoveExists(pos,move)) {printf("info string ignored invalid searchmove %s\n",token);continue;}
   int duplicate=FALSE;
   for(int i=0;i<info->searchMoveCount;i++) if(info->searchMoves[i]==move) duplicate=TRUE;
   if(!duplicate && info->searchMoveCount<MAXPOSITIONMOVES) info->searchMoves[info->searchMoveCount++]=move;
  }
 }
}
static void ParseGo(char *line,S_SEARCHINFO *info,S_BOARD *pos) {
 GoControls(line,info,pos);
 info->ponderBudget=0;
 long long depth=Number(line,"depth ",MAXDEPTH-1);
 if(depth<1) depth=1;if(depth>=MAXDEPTH) depth=MAXDEPTH-1;
 long long nodes=Number(line,"nodes ",0);
 if(nodes<0) nodes=0;if(nodes>LONG_MAX) nodes=LONG_MAX;info->nodeLimit=(long)nodes;
 info->depth=(int)depth;info->timeset=FALSE;info->starttime=GetTimeMs();info->stoptime=0;
 long long time=Number(line,pos->side==WHITE?"wtime ":"btime ",-1);
 long long inc=Number(line,pos->side==WHITE?"winc ":"binc ",0);
 long long mtg=Number(line,"movestogo ",30),fixed=Number(line,"movetime ",-1);
 if(mtg<1) mtg=1;if(inc<0) inc=0;if(inc>INT_MAX) inc=INT_MAX;
 if(time>INT_MAX) time=INT_MAX;if(fixed>INT_MAX) fixed=INT_MAX;
 if(!info->infinite && (time>=0 || fixed>=0)) {
  long long budget;
  if(fixed>=0) budget=fixed;
  else {
   long long reserve=time>1000?50:time/20;
   budget=time/mtg+inc*3/4;
   if(budget>time-reserve) budget=time-reserve;
  }
  if(budget<1) budget=1;
  info->ponderBudget=budget;
  if(!info->pondering) {info->timeset=TRUE;info->stoptime=info->starttime+budget;}
 }
 SearchPosition(pos,info);
}
static void ParsePosition(char *line,S_BOARD *pos) {
 S_BOARD next=*pos;
 char *fen=strstr(line,"fen ");
 if(ParseFen(fen?fen+4:START_FEN,&next)!=0) {printf("info string invalid FEN\n");return;}
 char *moves=strstr(line,"moves ");
 if(moves) {
  char *token=strtok(moves+6," \t\r\n");
  while(token) {
   size_t len=strlen(token);
   if(len<4 || len>5) {printf("info string invalid move\n");return;}
   int move=ParseMove(token,&next);
   if(move==NOMOVE || !MakeMove(&next,move)) {printf("info string illegal move\n");return;}
   next.ply=0;token=strtok(NULL," \t\r\n");
  }
 }
 *pos=next;
}
static void Identify(void) {
 printf("id name %s\nid author %s\n",NAME,ENGINE_AUTHOR);
 printf("option name Hash type spin default 64 min 4 max %d\n",MAX_HASH);
 printf("option name Book type check default true\noption name TaperedEval type check default true\noption name MultiPV type spin default 1 min 1 max 16\noption name Ponder type check default false\nuciok\n");
}
void Uci_Loop(S_BOARD *pos,S_SEARCHINFO *info) {
 info->GAME_MODE=UCIMODE;setbuf(stdin,NULL);setbuf(stdout,NULL);Identify();
 char line[4096];
 while(!info->quit && NextInputLine(line,sizeof line)) {
  if(!strcmp(line,"isready")) printf("readyok\n");
  else if(!strncmp(line,"position ",9)) ParsePosition(line,pos);
  else if(!strcmp(line,"ucinewgame")) {ClearHashTable(pos->HashTable);ParseFen(START_FEN,pos);}
  else if(!strcmp(line,"go") || !strncmp(line,"go ",3)) ParseGo(line,info,pos);
  else if(!strcmp(line,"quit")) info->quit=TRUE;
  else if(!strcmp(line,"uci")) Identify();
  else if(!strncmp(line,"setoption name Hash value ",26)) {
   long long mb=Number(line,"value ",64);if(mb<4) mb=4;if(mb>MAX_HASH) mb=MAX_HASH;InitHashTable(pos->HashTable,(int)mb);
  } else if(!strncmp(line,"setoption name MultiPV value ",28)) {
   long long n=Number(line,"value ",1);if(n<1)n=1;if(n>16)n=16;EngineOptions->MultiPV=(int)n;
  } else if(!strncmp(line,"setoption name Ponder value ",27)) EngineOptions->Ponder=strstr(line+27,"true")!=NULL;
  else if(!strncmp(line,"setoption name Book value ",26)) EngineOptions->UseBook=strstr(line+26,"true")!=NULL;
  else if(!strncmp(line,"setoption name TaperedEval value ",33)) {EngineOptions->TaperedEval=strstr(line+33,"true")!=NULL;ClearHashTable(pos->HashTable);}
 }
}
