#include "search_control.h"
#include <stdbool.h>
void SearchControlInit(S_SEARCHCONTROL *control,uint64_t limit,int64_t deadline) {
 atomic_init(&control->tbHits,0);atomic_init(&control->stop,false);atomic_init(&control->nodes,0);
 atomic_init(&control->deadline,deadline);control->nodeLimit=limit;
}
void SearchControlStop(S_SEARCHCONTROL *control) {
 atomic_store_explicit(&control->stop,true,memory_order_release);
}
void SearchControlDeadline(S_SEARCHCONTROL *control,int64_t deadline) {
 atomic_store_explicit(&control->deadline,deadline,memory_order_release);
}
int SearchControlStopped(const S_SEARCHCONTROL *control,int64_t now) {
 if(atomic_load_explicit(&control->stop,memory_order_acquire)) return 1;
 int64_t deadline=atomic_load_explicit(&control->deadline,memory_order_acquire);
 return deadline>0 && now>=deadline;
}
uint64_t SearchControlPublish(S_SEARCHCONTROL *control,uint64_t delta) {
 uint64_t total=atomic_fetch_add_explicit(&control->nodes,delta,memory_order_relaxed)+delta;
 if(control->nodeLimit && total>=control->nodeLimit) SearchControlStop(control);
 return total;
}
uint64_t SearchControlNodes(const S_SEARCHCONTROL *control) {
 return atomic_load_explicit(&control->nodes,memory_order_relaxed);
}

void SearchControlTbPublish(S_SEARCHCONTROL *control,uint64_t delta) {
 atomic_fetch_add_explicit(&control->tbHits,delta,memory_order_relaxed);
}
uint64_t SearchControlTbHits(const S_SEARCHCONTROL *control) {
 return atomic_load_explicit(&control->tbHits,memory_order_relaxed);
}
