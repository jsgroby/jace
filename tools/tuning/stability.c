/* Offline filter: keep positions whose baseline static and qsearch scores agree. */
#include "defs.h"
#include <string.h>
#define PARAM(name, value, low, high) extern int name;
#include "eval_params.def"
#undef PARAM
static void NoInput(S_SEARCHINFO *info) {(void)info;}
#define ReadInput NoInput
#include "search.c"
#undef ReadInput
int main(void) {
 AllInit();EngineOptions->UseBook=FALSE;
 /* Use the accepted baseline for filtering even after fitting new bonuses. */
 PassMG2=3;PassMG3=5;PassMG4=10;PassMG5=18;PassMG6=30;PassMG7=50;
 PassEG2=5;PassEG3=10;PassEG4=20;PassEG5=35;PassEG6=60;PassEG7=100;
 S_BOARD *p=calloc(1,sizeof *p);S_SEARCHINFO *info=calloc(1,sizeof *info);
 if(!p || !info) return 2;
 InitHashTable(p->HashTable,1);char line[512];unsigned kept=0,total=0,limited=0;
 while(fgets(line,sizeof line,stdin)) {
  char *fen=strchr(line,'\t');if(!fen || ParseFen(fen+1,p)!=0) return 2;
  if(SqAttacked(p->KingSq[p->side],p->side^1,p)) return 2;
  total++;memset(info,0,sizeof *info);info->nodeLimit=10000;
  ClearForSearch(p,info);
  int evaluation=EvalPosition(p),quiet=Quiescence(-INFINITE,INFINITE,p,info);
  if(info->stopped) limited++;
  else if(abs(evaluation-quiet)<=25) {fputs(line,stdout);kept++;}
 }
 fprintf(stderr,"Stability: %u/%u kept, %u node-limited; abs(static-qsearch)<=25 cp\n",kept,total,limited);
 FreeHashTable(p->HashTable);free(info);free(p);return 0;
}
