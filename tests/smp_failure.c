#include "defs.h"
#include "threads.h"
#include <assert.h>
#include "search.c"
int main(void) {
 AllInit();EngineOptions->UseBook=FALSE;EngineOptions->Threads=4;
 S_BOARD *pos=calloc(1,sizeof *pos);S_SEARCHINFO *info=calloc(1,sizeof *info);
 assert(pos && info);InitHashTable(pos->HashTable,4);assert(ParseFen(START_FEN,pos)==0);
 info->GAME_MODE=UCIMODE;
 for(int fail=0;fail<3;fail++) {
  ClearForSearch(pos,info);JaceThreadFailAfter(fail);
  assert(StartHelpers(pos,info,4)==NULL);
  assert(info->control==NULL && pos->HashTable->sync==NULL);
  ClearHashTable(pos->HashTable);
  JaceThreadFailAfter(-1);
  S_SEARCHPOOL *pool=StartHelpers(pos,info,4);assert(pool);
  FinishHelpers(pool,pos,info);assert(pos->HashTable->sync==NULL);
 }
 FreeHashTable(pos->HashTable);free(pos);free(info);CleanPolyBook();SyzygyFree();
 puts("SMP PARTIAL START FAILURE PASS");return 0;
}
