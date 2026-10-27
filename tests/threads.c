#include "threads.h"
#include <assert.h>
#include <stdio.h>
#include <stdatomic.h>

typedef struct {
 JaceMutex *mutex;
 JaceCondition *work,*done;
 unsigned epoch,completed,visits;
 int stop;
 atomic_uint progress;
} Pool;
static void Worker(void *argument) {
 Pool *pool=argument;unsigned seen=0;
 JaceMutexLock(pool->mutex);
 for(;;) {
  while(!pool->stop && pool->epoch==seen) JaceConditionWait(pool->work,pool->mutex);
  if(pool->stop) break;
  seen=pool->epoch;pool->visits++;
  JaceMutexUnlock(pool->mutex);
  atomic_fetch_add_explicit(&pool->progress,1,memory_order_relaxed);
  JaceMutexLock(pool->mutex);
  pool->completed++;JaceConditionBroadcast(pool->done);
 }
 JaceMutexUnlock(pool->mutex);
}
static void Cycle(int allowed) {
 Pool pool={0};atomic_init(&pool.progress,0);
 pool.mutex=JaceMutexCreate();pool.work=JaceConditionCreate();pool.done=JaceConditionCreate();
 assert(pool.mutex && pool.work && pool.done);
 JaceThread *threads[4]={0};unsigned count=0;
 JaceThreadFailAfter(allowed);
 for(;count<4;count++) {
  threads[count]=JaceThreadStart(Worker,&pool);
  if(!threads[count]) break;
 }
 assert(count==(allowed<0?4u:(unsigned)allowed));
 JaceThreadFailAfter(-1);
 /* On partial failure, immediately stop/join every successfully started worker.
    No work is published into a partially constructed pool. */
 if(count==4) for(unsigned epoch=1;epoch<=100;epoch++) {
  JaceMutexLock(pool.mutex);
  pool.completed=0;pool.epoch=epoch;JaceConditionBroadcast(pool.work);
  while(pool.completed<count) JaceConditionWait(pool.done,pool.mutex);
  assert(pool.visits==epoch*count);
  /* A broadcast without changing the predicate must not create extra work. */
  JaceConditionBroadcast(pool.work);
  JaceMutexUnlock(pool.mutex);
 }
 JaceMutexLock(pool.mutex);pool.stop=1;JaceConditionBroadcast(pool.work);JaceMutexUnlock(pool.mutex);
 for(unsigned i=0;i<count;i++) assert(JaceThreadJoin(threads[i])==0);
 assert(atomic_load_explicit(&pool.progress,memory_order_relaxed)==(count==4?400u:0u));
 JaceConditionDestroy(pool.done);JaceConditionDestroy(pool.work);JaceMutexDestroy(pool.mutex);
}
int main(void) {
 assert(JaceThreadStart(NULL,NULL)==NULL);assert(JaceThreadJoin(NULL)!=0);
 for(int i=0;i<20;i++) {Cycle(-1);Cycle(0);Cycle(1);Cycle(2);Cycle(3);}
 puts("THREAD LIFECYCLE PASS: 100 pools, 8000 tasks, partial-start cleanup");
 return 0;
}
