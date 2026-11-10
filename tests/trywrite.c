#include "defs.h"
#include "threads.h"
#include <assert.h>
#include <stdatomic.h>
#include <string.h>
#include "pvtable.c"
static S_BOARD worker;
static int move;
static void Store(void *unused) {
 (void)unused;StoreHashEntry(&worker,move,42,HFEXACT,5);
}
int main(void) {
 AllInit();EngineOptions->UseBook=FALSE;
 S_BOARD owner={0};assert(ParseFen(START_FEN,&owner)==0);
 InitHashTable(owner.HashTable,1);assert(EnableHashSharing(owner.HashTable));
 move=ParseMove("e2e4",&owner);StoreHashEntry(&owner,move,17,HFEXACT,5);
 CopyBoardForSearch(&worker,&owner);
 int index=HashIndex(&owner);S_HASHENTRY before;
 memcpy(&before,&owner.HashTable->pTable[index],sizeof before);
 JaceMutex *lock=HashLock(owner.HashTable,index);JaceMutexLock(lock);
 JaceThread *t=JaceThreadStart(Store,NULL);assert(t);
 /* Join while holding the lock proves the worker store does not wait. */
 assert(JaceThreadJoin(t)==0);
 JaceMutexUnlock(lock);
 assert(memcmp(&before,&owner.HashTable->pTable[index],sizeof before)==0);
 S_HASHENTRY after;
 t=JaceThreadStart(Store,NULL);assert(t && JaceThreadJoin(t)==0);
 after=ReadHashEntry(owner.HashTable,index);assert(after.score==42 && after.move==move);
 FreeHashTable(worker.HashTable);DisableHashSharing(owner.HashTable);
 StoreHashEntry(&owner,move,73,HFEXACT,5);
 after=ReadHashEntry(owner.HashTable,index);assert(after.score==73);
 FreeHashTable(owner.HashTable);CleanPolyBook();puts("CONTENDED STORE SKIP / UNCONTENDED AND SERIAL STORE PASS");return 0;
}
