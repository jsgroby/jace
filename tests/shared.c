#include "defs.h"
#include "threads.h"
#include "search_control.h"
#include <assert.h>
#include <string.h>

typedef struct {
 S_BOARD position,tb;
 S_SEARCHCONTROL *control;
 int id,index,probe;
 int *moves;
 uint64_t iterations;
 JaceMutex *gate;
 JaceCondition *start;
 int *go;
} Worker;
static void Run(void *argument) {
 Worker *worker=argument;S_BOARD *p=&worker->position;
 JaceMutexLock(worker->gate);
 while(!*worker->go) JaceConditionWait(worker->start,worker->gate);
 JaceMutexUnlock(worker->gate);
 while(!SearchControlStopped(worker->control,0)) {
  StoreHashEntry(p,worker->moves[worker->id],100+worker->id,HFEXACT,5);
  int move=0,score=0;
  if(ProbeHashEntry(p,&move,&score,-INFINITE,INFINITE,5)) {
   assert(move==worker->moves[worker->id] && score==100+worker->id);
  }
  (void)ProbePvMove(p);
  S_HASHENTRY entry=ReadHashEntry(p->HashTable,worker->index);
  int id=entry.score-100;
  assert(id>=0 && id<4 && entry.move==worker->moves[id]);
  assert(entry.fiftyMove==id && entry.context==(U64)id && entry.depth==5 && entry.flags==HFEXACT);
  if(worker->probe) {
   int tbScore=0;assert(SyzygyWdl(&worker->tb,&tbScore,5) && tbScore==20000);
  }
  worker->iterations++;
  SearchControlPublish(worker->control,1);
 }
}
int main(int argc,char **argv) {
 AllInit();EngineOptions->UseBook=FALSE;
 int probe=argc>1 && SyzygyInit(argv[1])>=3;
 S_BOARD owner={0};assert(ParseFen(START_FEN,&owner)==0);InitHashTable(owner.HashTable,1);
 assert(EnableHashSharing(owner.HashTable));
 int moves[4];char *names[]={"e2e4","d2d4","g1f3","c2c4"};
 for(int i=0;i<4;i++) moves[i]=ParseMove(names[i],&owner);
 for(int cycle=0;cycle<10;cycle++) {
  S_SEARCHCONTROL control;SearchControlInit(&control,20000,0);
  Worker *workers=calloc(4,sizeof *workers);assert(workers);JaceThread *threads[4];
  JaceMutex *gate=JaceMutexCreate();JaceCondition *start=JaceConditionCreate();int go=0;
  assert(gate && start);
  owner.HashTable->generation++;
  for(int i=0;i<4;i++) {
   Worker *w=&workers[i];CopyBoardForSearch(&w->position,&owner);
   w->id=i;w->control=&control;w->moves=moves;w->probe=probe;
   w->gate=gate;w->start=start;w->go=&go;
   /* Distinct keys intentionally collide at the same TT slot. */
   w->position.posKey=owner.posKey+(U64)owner.HashTable->numEntries*(U64)(i+1);
   w->position.fiftyMove=i;w->index=(int)(owner.posKey&(U64)(owner.HashTable->numEntries-1));
   assert(ParseFen("7k/8/8/8/8/8/8/KQ6 w - - 0 1",&w->tb)==0);
  }
  S_HASHENTRY *allocation=owner.HashTable->pTable;
  /* Outstanding views prohibit owner lifecycle operations even when idle. */
  InitHashTable(owner.HashTable,2);FreeHashTable(owner.HashTable);
  assert(owner.HashTable->pTable==allocation);
  for(int i=0;i<4;i++) {threads[i]=JaceThreadStart(Run,&workers[i]);assert(threads[i]);}
  JaceMutexLock(gate);go=1;JaceConditionBroadcast(start);JaceMutexUnlock(gate);
  uint64_t total=0;
  for(int i=0;i<4;i++) {assert(JaceThreadJoin(threads[i])==0);total+=workers[i].iterations;FreeHashTable(workers[i].position.HashTable);}
  assert(total==SearchControlNodes(&control) && total>=20000 && total<=20003);
  assert(SearchControlStopped(&control,0));
  JaceConditionDestroy(start);JaceMutexDestroy(gate);
  free(workers);ClearHashTable(owner.HashTable);
 }
 S_SEARCHCONTROL control;SearchControlInit(&control,0,0);
 assert(!SearchControlStopped(&control,100));SearchControlDeadline(&control,200);
 assert(!SearchControlStopped(&control,199) && SearchControlStopped(&control,200));
 SearchControlDeadline(&control,0);assert(!SearchControlStopped(&control,200));
 SearchControlStop(&control);assert(SearchControlStopped(&control,0));
 FreeHashTable(owner.HashTable);SyzygyFree();CleanPolyBook();
 puts("SHARED TT / CANCELLATION PASS: 10 collision cycles, bounded node publication");
 puts(probe?"CONCURRENT REAL SYZYGY WDL PASS":"Real concurrent Syzygy NOT RUN: supply tables");
 return 0;
}
