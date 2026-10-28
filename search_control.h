#ifndef JACE_SEARCH_CONTROL_H
#define JACE_SEARCH_CONTROL_H
#include <stdint.h>
#include <stdatomic.h>

/* Initialize only while idle. Limit is immutable until all workers join.
   Counters/cancellation are atomic; request and board storage remain private. */
typedef struct {
 atomic_bool stop;
 atomic_uint_fast64_t nodes;
 atomic_int_fast64_t deadline;
 uint64_t nodeLimit;
} S_SEARCHCONTROL;
void SearchControlInit(S_SEARCHCONTROL *control,uint64_t limit,int64_t deadline);
void SearchControlStop(S_SEARCHCONTROL *control);
void SearchControlDeadline(S_SEARCHCONTROL *control,int64_t deadline);
int SearchControlStopped(const S_SEARCHCONTROL *control,int64_t now);
uint64_t SearchControlPublish(S_SEARCHCONTROL *control,uint64_t delta);
uint64_t SearchControlNodes(const S_SEARCHCONTROL *control);
#endif
