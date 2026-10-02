#include "defs.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
static void NoInput(S_SEARCHINFO *info) {(void)info;}
#define ReadInput NoInput
#define WaitForSearchSignal NoInput
#ifdef PROFILE
static unsigned long long calls,duplicates,seeNs;
static long nodeId[MAXDEPTH],seenId[MAXDEPTH];
static int seenMoves[MAXDEPTH][MAXPOSITIONMOVES],seenCount[MAXDEPTH];
extern int RawSEE(const S_BOARD *pos,int move);
static unsigned long long Ns(void) {
 struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);
 return (unsigned long long)t.tv_sec*1000000000ULL+(unsigned long long)t.tv_nsec;
}
static int ProfileSEE(const S_BOARD *pos,int move) {
 int ply=pos->ply;
 if(seenId[ply]!=nodeId[ply]) {seenId[ply]=nodeId[ply];seenCount[ply]=0;}
 int found=0;
 for(int i=0;i<seenCount[ply];i++) if(seenMoves[ply][i]==move) {found=1;break;}
 if(found) duplicates++;else {if(seenCount[ply]>=MAXPOSITIONMOVES) abort();seenMoves[ply][seenCount[ply]++]=move;}
 calls++;unsigned long long start=Ns();int value=RawSEE(pos,move);seeNs+=Ns()-start;return value;
}
#define SEE ProfileSEE
#include "search-profile.inc"
#else
#include "search.c"
#endif
int main(int argc,char **argv) {
 if(argc!=2)return 1;AllInit();EngineOptions->UseBook=FALSE;
 EngineOptions->TaperedEval=TRUE;EngineOptions->MultiPV=1;EngineOptions->Ponder=FALSE;
 S_BOARD p={0};S_SEARCHINFO info={0};info.GAME_MODE=UCIMODE;info.depth=6;
 InitHashTable(p.HashTable,64);FILE *f=fopen(argv[1],"r");if(!f)return 2;
 char fen[256];int n=0;
 while(fgets(fen,sizeof fen,f)) {
  if(ParseFen(fen,&p))return 3;ClearHashTable(p.HashTable);
#ifdef PROFILE
  calls=duplicates=seeNs=0;memset(seenCount,0,sizeof seenCount);memset(seenId,0,sizeof seenId);
#endif
  printf("POSITION %d\n",n++);info.starttime=GetTimeMs();SearchPosition(&p,&info);
#ifdef PROFILE
  printf("PROFILE %llu %llu %llu\n",calls,duplicates,seeNs);
#endif
 }
 fclose(f);free(p.HashTable->pTable);CleanPolyBook();return 0;
}
