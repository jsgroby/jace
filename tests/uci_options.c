/* Inspect protocol effects without running a timed search. */
#include "defs.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "uci.c"

void SearchPosition(S_BOARD *pos,S_SEARCHINFO *info) {(void)pos;(void)info;}

static void Option(S_BOARD *pos,const char *text) {
 char line[256];snprintf(line,sizeof line,"setoption name %s",text);
 SetOption(line,pos);
}
static void Go(S_BOARD *pos,S_SEARCHINFO *info,const char *text) {
 char line[256];snprintf(line,sizeof line,"%s",text);ParseGo(line,info,pos);
}
int main(void) {
 AllInit();
 S_BOARD pos={0};S_SEARCHINFO info={0};
 InitHashTable(pos.HashTable,4);assert(ParseFen(START_FEN,&pos)==0);
 assert(EngineOptions->MoveOverhead==50);
 Option(&pos,"mUlTiPv VaLuE 3");assert(EngineOptions->MultiPV==3);
 Option(&pos,"pOnDeR value TRUE");assert(EngineOptions->Ponder==TRUE);
 Option(&pos,"bOoK value FALSE");assert(EngineOptions->UseBook==FALSE);
 Option(&pos,"TAPERedeval value FALSE");assert(EngineOptions->TaperedEval==FALSE);
 Option(&pos,"TAPERedeval value true");assert(EngineOptions->TaperedEval==TRUE);
 Option(&pos,"Ponder value untrue");assert(EngineOptions->Ponder==TRUE);
 Option(&pos,"MultiPV value 3junk");assert(EngineOptions->MultiPV==3);
 Option(&pos,"MultiPV value 999999999999999999999999999");assert(EngineOptions->MultiPV==3);
 Option(&pos,"move overhead value -5");assert(EngineOptions->MoveOverhead==0);
 Option(&pos,"move overhead value 9000");assert(EngineOptions->MoveOverhead==5000);
 Option(&pos,"move overhead value 50");
 /* MultiPV retains the existing fixed clock allocation. */
 for(int t=0;t<=120000;t+=137) {
  for(int inc=0;inc<=1000;inc+=500) {
   long long expected=t/30+inc*3/4;
   long long reserve=t>1000?50:t/20;
   if(expected>t-reserve)expected=t-reserve;
   if(expected<1)expected=1;
   char go[128];snprintf(go,sizeof go,"go wtime %d winc %d",t,inc);
   Go(&pos,&info,go);assert(info.timeset);
   assert(info.stoptime-info.starttime==expected);
  }
 }
 Option(&pos,"Move Overhead value 500");
 Go(&pos,&info,"go wtime 10000 movestogo 1");assert(info.ponderBudget==9500);
 Go(&pos,&info,"go btime 1 wtime 1000 movestogo 1");assert(info.ponderBudget==950);
 Option(&pos,"Move Overhead value 0");
 Go(&pos,&info,"go wtime 10000 movestogo 1");assert(info.ponderBudget==10000);
 Option(&pos,"Move Overhead value 500");
 Go(&pos,&info,"go movetime 1000");assert(info.ponderBudget==1000);
 Go(&pos,&info,"go ponder wtime 10000 movestogo 1");
 assert(info.pondering && !info.timeset && info.ponderBudget==9500);
 Go(&pos,&info,"go infinite wtime 10000");assert(!info.timeset && info.ponderBudget==0);
 Go(&pos,&info,"go depth 5");assert(!info.timeset && info.depth==5);
 /* Single-PV clock searches have a soft budget and capped hard deadline. */
 Option(&pos,"MultiPV value 1");Option(&pos,"Move Overhead value 50");
 for(int t=0;t<=120000;t+=137) for(int inc=0;inc<=1000;inc+=500) {
  long long available=t-(t>1000?50:t/20);
  long long soft=t/30+inc*3/4;if(soft>available)soft=available;if(soft<1)soft=1;
  long long hard=2*soft;if(hard>available/4)hard=available/4;if(hard<1)hard=1;
  char go[128];snprintf(go,sizeof go,"go wtime %d winc %d",t,inc);
  Go(&pos,&info,go);
  assert(info.adaptiveTime && info.timeset && info.softBudget==soft);
  assert(info.ponderBudget==hard && info.stoptime-info.timeStart==hard);
 }
 Go(&pos,&info,"go wtime 60000 winc 1000");assert(info.softBudget==2750 && info.ponderBudget==5500);
 Go(&pos,&info,"go wtime 100 winc 100");assert(info.ponderBudget==23);
 Go(&pos,&info,"go wtime 1000 movestogo 1");assert(info.softBudget==950 && info.ponderBudget==950);
 Go(&pos,&info,"go wtime 2147483647 winc 2147483647 movestogo 1");
 assert(info.ponderBudget==2147483597LL);
 Go(&pos,&info,"go ponder wtime 60000 winc 1000");
 assert(info.adaptiveTime && info.pondering && !info.timeset && info.ponderBudget==5500);
 Go(&pos,&info,"go movetime 80 wtime 60000");
 assert(!info.adaptiveTime && info.softBudget==0 && info.ponderBudget==80);
 Go(&pos,&info,"go infinite wtime 60000");assert(!info.adaptiveTime && !info.timeset);
 Go(&pos,&info,"go depth 5");assert(!info.adaptiveTime && !info.timeset);
 Go(&pos,&info,"go nodes 1000");assert(!info.adaptiveTime && !info.timeset);
 pos.side=BLACK;Go(&pos,&info,"go wtime 1 btime 60000 binc 1000");
 assert(info.softBudget==2750 && info.ponderBudget==5500);pos.side=WHITE;
 int move=ParseMove("e2e4",&pos);assert(MakeMove(&pos,move));pos.ply=0;
 StoreHashEntry(&pos,ParseMove("e7e5",&pos),42,HFEXACT,4);
 assert(pos.HashTable->newWrite>0);
 U64 key=pos.posKey;int history=pos.hisPly,entries=pos.HashTable->numEntries;
 S_HASHENTRY *memory=pos.HashTable->pTable;
 Option(&pos,"cLeAr hAsH");
 assert(pos.posKey==key && pos.hisPly==history && CheckBoard(&pos));
 assert(pos.HashTable->pTable==memory && pos.HashTable->numEntries==entries);
 assert(pos.HashTable->newWrite==0 && ProbePvMove(&pos)==NOMOVE);
 for(int i=0;i<entries;i++)assert(pos.HashTable->pTable[i].posKey==0);
 free(pos.HashTable->pTable);CleanPolyBook();
 puts("UCI OPTIONS AND TIME BUDGET TESTS PASS");
 return 0;
}
