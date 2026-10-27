#ifndef JACE_THREADS_H
#define JACE_THREADS_H

/* OS types stay private. Objects are heap allocated, noncopyable and owned by
   the coordinator. Destroy only after all users have joined. No detached threads.
   Create/start return NULL on failure; join returns 0 on success and consumes
   the handle. A failed join leaves ownership with the caller. */
typedef struct JaceThread JaceThread;
typedef struct JaceMutex JaceMutex;
typedef struct JaceCondition JaceCondition;
typedef void (*JaceThreadFunction)(void *);

JaceThread *JaceThreadStart(JaceThreadFunction function,void *argument);
int JaceThreadJoin(JaceThread *thread);
JaceMutex *JaceMutexCreate(void);
void JaceMutexDestroy(JaceMutex *mutex);
void JaceMutexLock(JaceMutex *mutex);
void JaceMutexUnlock(JaceMutex *mutex);
JaceCondition *JaceConditionCreate(void);
void JaceConditionDestroy(JaceCondition *condition);
/* Wait atomically releases/reacquires the mutex. Always loop over a predicate
   protected by that mutex; notifications alone are not persistent state. */
void JaceConditionWait(JaceCondition *condition,JaceMutex *mutex);
void JaceConditionBroadcast(JaceCondition *condition);

#ifdef JACE_THREAD_TESTING
/* Coordinator-only deterministic start failure. -1 disables; 0 fails next and
   subsequent starts; n permits n successful starts before failing. */
void JaceThreadFailAfter(int count);
#endif
#endif
