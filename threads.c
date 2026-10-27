#include "threads.h"
#include <stdlib.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#include <process.h>
struct JaceMutex { CRITICAL_SECTION native; };
struct JaceCondition { CONDITION_VARIABLE native; };
struct JaceThread { HANDLE native; JaceThreadFunction function; void *argument; };
#else
#include <pthread.h>
struct JaceMutex { pthread_mutex_t native; };
struct JaceCondition { pthread_cond_t native; };
struct JaceThread { pthread_t native; JaceThreadFunction function; void *argument; };
#endif

/* An unexpected synchronization failure cannot safely be ignored. Allocation
   and thread-start failures are recoverable and reported to the coordinator. */
static void Require(int ok) { if(!ok) abort(); }
#ifdef JACE_THREAD_TESTING
static int startsRemaining=-1;
void JaceThreadFailAfter(int count) { startsRemaining=count; }
#endif

#ifdef _WIN32
static unsigned __stdcall Run(void *argument) {
 JaceThread *thread=argument;thread->function(thread->argument);return 0;
}
#else
static void *Run(void *argument) {
 JaceThread *thread=argument;thread->function(thread->argument);return NULL;
}
#endif

JaceThread *JaceThreadStart(JaceThreadFunction function,void *argument) {
 if(!function) return NULL;
#ifdef JACE_THREAD_TESTING
 if(startsRemaining==0) return NULL;
#endif
 JaceThread *thread=malloc(sizeof *thread);if(!thread) return NULL;
 thread->function=function;thread->argument=argument;
 /* Reserve 8 MiB explicitly: recursive chess search must not depend on the
    smaller Windows default stack. Worker board/cache storage will be on heap. */
#ifdef _WIN32
 uintptr_t handle=_beginthreadex(NULL,8u*1024u*1024u,Run,thread,
                               STACK_SIZE_PARAM_IS_A_RESERVATION,NULL);
 if(!handle) {free(thread);return NULL;}
 thread->native=(HANDLE)handle;
#else
 pthread_attr_t attributes;
 if(pthread_attr_init(&attributes)!=0) {free(thread);return NULL;}
 int result=pthread_attr_setstacksize(&attributes,8u*1024u*1024u);
 if(!result) result=pthread_create(&thread->native,&attributes,Run,thread);
 Require(pthread_attr_destroy(&attributes)==0);
 if(result) {free(thread);return NULL;}
#endif
#ifdef JACE_THREAD_TESTING
 if(startsRemaining>0) startsRemaining--;
#endif
 return thread;
}
int JaceThreadJoin(JaceThread *thread) {
 if(!thread) return -1;
#ifdef _WIN32
 if(WaitForSingleObject(thread->native,INFINITE)!=WAIT_OBJECT_0) return -1;
 if(!CloseHandle(thread->native)) return -1;
#else
 if(pthread_join(thread->native,NULL)!=0) return -1;
#endif
 free(thread);return 0;
}
JaceMutex *JaceMutexCreate(void) {
 JaceMutex *mutex=malloc(sizeof *mutex);if(!mutex) return NULL;
#ifdef _WIN32
 if(!InitializeCriticalSectionEx(&mutex->native,0,0)) {free(mutex);return NULL;}
#else
 if(pthread_mutex_init(&mutex->native,NULL)!=0) {free(mutex);return NULL;}
#endif
 return mutex;
}
void JaceMutexDestroy(JaceMutex *mutex) {
 if(!mutex) return;
#ifdef _WIN32
 DeleteCriticalSection(&mutex->native);
#else
 Require(pthread_mutex_destroy(&mutex->native)==0);
#endif
 free(mutex);
}
void JaceMutexLock(JaceMutex *mutex) {
#ifdef _WIN32
 EnterCriticalSection(&mutex->native);
#else
 Require(pthread_mutex_lock(&mutex->native)==0);
#endif
}
void JaceMutexUnlock(JaceMutex *mutex) {
#ifdef _WIN32
 LeaveCriticalSection(&mutex->native);
#else
 Require(pthread_mutex_unlock(&mutex->native)==0);
#endif
}
JaceCondition *JaceConditionCreate(void) {
 JaceCondition *condition=malloc(sizeof *condition);if(!condition) return NULL;
#ifdef _WIN32
 InitializeConditionVariable(&condition->native);
#else
 if(pthread_cond_init(&condition->native,NULL)!=0) {free(condition);return NULL;}
#endif
 return condition;
}
void JaceConditionDestroy(JaceCondition *condition) {
 if(!condition) return;
#ifndef _WIN32
 Require(pthread_cond_destroy(&condition->native)==0);
#endif
 free(condition);
}
void JaceConditionWait(JaceCondition *condition,JaceMutex *mutex) {
#ifdef _WIN32
 Require(SleepConditionVariableCS(&condition->native,&mutex->native,INFINITE)!=0);
#else
 Require(pthread_cond_wait(&condition->native,&mutex->native)==0);
#endif
}
void JaceConditionBroadcast(JaceCondition *condition) {
#ifdef _WIN32
 WakeAllConditionVariable(&condition->native);
#else
 Require(pthread_cond_broadcast(&condition->native)==0);
#endif
}
