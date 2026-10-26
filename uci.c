#include "defs.h"
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

/* Option identifiers and values are case-insensitive; command keywords are not. */
static int EqualOption(const char *a,const char *b) {
 while(*a && *b) {
  if(tolower((unsigned char)*a)!=tolower((unsigned char)*b)) return FALSE;
  a++;b++;
 }
 return *a==*b;
}
int UciDebugCommand(const char *line) {
 static int enabled=FALSE;
 if(!strcmp(line,"debug") || !strncmp(line,"debug ",6)) {
  if(!strcmp(line,"debug on")) enabled=TRUE;
  else if(!strcmp(line,"debug off")) enabled=FALSE;
  if(enabled) printf("info string debug on\n");
  return TRUE;
 }
 if(enabled) printf("info string received %s\n",line);
 return FALSE;
}
static int OptionNumber(const char *text,long long *value) {
 char *end;errno=0;
 long long n=strtoll(text,&end,10);
 if(end==text || *end || errno==ERANGE) return FALSE;
 *value=n;return TRUE;
}
static void SetOption(char *line,S_BOARD *pos) {
 char *name=line+15,*value=NULL;
 /* Input whitespace is normalized before reaching this parser. */
 for(char *p=name;*p;p++) {
  if(*p==' ' && strlen(p)>=7 &&
     tolower((unsigned char)p[1])=='v' && tolower((unsigned char)p[2])=='a' &&
     tolower((unsigned char)p[3])=='l' && tolower((unsigned char)p[4])=='u' &&
     tolower((unsigned char)p[5])=='e' && p[6]==' ') {
   *p=0;value=p+7;break;
  }
 }
 if(EqualOption(name,"Clear Hash")) {ClearHashTable(pos->HashTable);return;}
 if(!value) return;
 if(EqualOption(name,"SyzygyPath")) {
  unsigned n=SyzygyInit(value);ClearHashTable(pos->HashTable);
  printf("info string Syzygy available up to %u pieces\n",n);return;
 }
 if(EqualOption(name,"SyzygyProbeLimit")) {
  long long limit;if(!OptionNumber(value,&limit))return;
  if(limit<0)limit=0;
  if(limit>7)limit=7;
  EngineOptions->SyzygyProbeLimit=(int)limit;ClearHashTable(pos->HashTable);return;
 }
 long long n;
 if(EqualOption(name,"Hash") || EqualOption(name,"MultiPV") || EqualOption(name,"Move Overhead")) {
  if(!OptionNumber(value,&n)) return;
  if(EqualOption(name,"Hash")) {
   if(n<4)n=4;
   if(n>MAX_HASH)n=MAX_HASH;
   InitHashTable(pos->HashTable,(int)n);
  } else if(EqualOption(name,"MultiPV")) {
   if(n<1)n=1;
   if(n>16)n=16;
   EngineOptions->MultiPV=(int)n;
  } else {
   if(n<0)n=0;
   if(n>5000)n=5000;
   EngineOptions->MoveOverhead=(int)n;
  }
 } else {
  int flag;
  if(EqualOption(value,"true")) flag=TRUE;
  else if(EqualOption(value,"false")) flag=FALSE;
  else return;
  if(EqualOption(name,"Ponder")) EngineOptions->Ponder=flag;
  else if(EqualOption(name,"Book")) EngineOptions->UseBook=flag;
  else if(EqualOption(name,"TaperedEval")) {
   EngineOptions->TaperedEval=flag;ClearHashTable(pos->HashTable);
  }
 }
}
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
 info->ponderBudget=0;info->adaptiveTime=FALSE;info->softBudget=0;info->timeStart=0;
 long long depth=Number(line,"depth ",MAXDEPTH-1);
 if(depth<1) depth=1;
 if(depth>=MAXDEPTH) depth=MAXDEPTH-1;
 long long nodes=Number(line,"nodes ",0);
 if(nodes<0) nodes=0;
 if(nodes>LONG_MAX) nodes=LONG_MAX;
 info->nodeLimit=(long)nodes;
 info->depth=(int)depth;info->timeset=FALSE;info->starttime=GetTimeMs();info->stoptime=0;
 long long time=Number(line,pos->side==WHITE?"wtime ":"btime ",-1);
 long long inc=Number(line,pos->side==WHITE?"winc ":"binc ",0);
 long long mtg=Number(line,"movestogo ",30),fixed=Number(line,"movetime ",-1);
 if(mtg<1) mtg=1;
 if(inc<0) inc=0;
 if(inc>INT_MAX) inc=INT_MAX;
 if(time>INT_MAX) time=INT_MAX;
 if(fixed>INT_MAX) fixed=INT_MAX;
 if(!info->infinite && (time>=0 || fixed>=0)) {
  long long budget,available=0;
  if(fixed>=0) budget=fixed;
  else {
   /* Preserve the old default reserve; never reserve over 5% of the clock. */
   long long reserve=EngineOptions->MoveOverhead;
   if(reserve>time/20) reserve=time/20;
   budget=time/mtg+inc*3/4;
   available=time-reserve;
   if(budget>available) budget=available;
  }
  if(budget<1) budget=1;
  /* Keep fixed movetime and analysis MultiPV allocation unchanged. */
  if(fixed<0 && EngineOptions->MultiPV==1) {
   info->adaptiveTime=TRUE;info->softBudget=budget;
   budget*=2;
   /* Preserve clock for later moves, especially when the increment dominates
      the nominal budget. A sudden unstable iteration must not drain it. */
   if(mtg>1 && budget>available/4) budget=available/4;
   if(budget>available) budget=available;
   if(budget<1) budget=1;
  }
  info->timeStart=info->starttime;
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
 printf("option name Clear Hash type button\n");
 printf("option name Move Overhead type spin default 50 min 0 max 5000\n");
 printf("option name SyzygyPath type string default <empty>\noption name SyzygyProbeLimit type spin default 5 min 0 max 7\n");
 printf("option name Book type check default true\noption name TaperedEval type check default true\noption name MultiPV type spin default 1 min 1 max 16\noption name Ponder type check default false\nuciok\n");
}
void Uci_Loop(S_BOARD *pos,S_SEARCHINFO *info) {
 info->GAME_MODE=UCIMODE;setbuf(stdin,NULL);setbuf(stdout,NULL);Identify();
 char line[4096];
 while(!info->quit && NextInputLine(line,sizeof line)) {
  if(UciDebugCommand(line)) continue;
  if(!strcmp(line,"isready")) printf("readyok\n");
  else if(!strncmp(line,"position ",9)) ParsePosition(line,pos);
  else if(!strcmp(line,"ucinewgame")) {ClearHashTable(pos->HashTable);ParseFen(START_FEN,pos);}
  else if(!strcmp(line,"go") || !strncmp(line,"go ",3)) ParseGo(line,info,pos);
  else if(!strcmp(line,"quit")) info->quit=TRUE;
  else if(!strcmp(line,"uci")) Identify();
  else if(!strncmp(line,"setoption name ",15)) SetOption(line,pos);
 }
}
