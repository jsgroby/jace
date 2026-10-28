// pvtable.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "threads.h"
#include <stdatomic.h>

#define HASH_STRIPES 256u
struct S_HASHSYNC { JaceMutex *locks[HASH_STRIPES]; atomic_uint borrowers; };
static JaceMutex *HashLock(const S_HASHTABLE *table,int index) {
 return table->sync?table->sync->locks[(unsigned)index%HASH_STRIPES]:NULL;
}
S_HASHENTRY ReadHashEntry(const S_HASHTABLE *table,int index) {
 ASSERT(index>=0 && index<table->numEntries);
 JaceMutex *lock=HashLock(table,index);if(lock) JaceMutexLock(lock);
 S_HASHENTRY entry=table->pTable[index];
 if(lock) JaceMutexUnlock(lock);
 return entry;
}
static int HasBorrowers(const S_HASHTABLE *table) {
 return table->sync && atomic_load_explicit(&table->sync->borrowers,memory_order_acquire)!=0;
}
int EnableHashSharing(S_HASHTABLE *table) {
 if(table->borrowed || !table->pTable) return FALSE;
 if(table->sync) return TRUE;
 struct S_HASHSYNC *sync=calloc(1,sizeof *sync);if(!sync) return FALSE;
 atomic_init(&sync->borrowers,0);
 for(unsigned i=0;i<HASH_STRIPES;i++) {
  sync->locks[i]=JaceMutexCreate();
  if(!sync->locks[i]) {
   for(unsigned j=0;j<i;j++) JaceMutexDestroy(sync->locks[j]);
   free(sync);return FALSE;
  }
 }
 table->sync=sync;return TRUE;
}

/* Coordinator-only, after every borrowed board has detached. */
void DisableHashSharing(S_HASHTABLE *table) {
 if(table->borrowed || HasBorrowers(table) || !table->sync) return;
 for(unsigned i=0;i<HASH_STRIPES;i++) JaceMutexDestroy(table->sync->locks[i]);
 free(table->sync);table->sync=NULL;
}

// Return the largest power of two <= n (with a safe minimum of 2).
static int LargestPow2LEQ(int n) {
	if (n < 2) return 2;
	// Bit-hack: propagate highest set bit, then keep only that bit.
	unsigned int x = (unsigned int)n;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return (int)(x - (x >> 1));
}

/* Preserve move reuse while keeping score bounds specific to reversible history.
   This conservative context check also distinguishes partial FEN histories. */
static U64 DrawContext(const S_BOARD *pos) {
 U64 key=(U64)pos->fiftyMove;
 int first=pos->hisPly-pos->fiftyMove;
 if(first<0) first=0;
 for(int i=pos->hisPly-1;i>=first;i--) {
  key=(key ^ pos->history[i].posKey)*0x9e3779b97f4a7c15ULL;
  if(pos->history[i].move==NOMOVE) break;
 }
 return key;
}

// Hash index using bitmask (numEntries is guaranteed power-of-two)
static inline int HashIndex(const S_BOARD *pos) {
	return (int)(pos->posKey & (U64)(pos->HashTable->numEntries - 1));
}

/* Printed continuations must stop at claimable draws. Search intentionally
   uses an earlier (twofold) cycle cutoff; that search policy is unchanged. */
int PvDrawReached(const S_BOARD *pos) {
 if(pos->fiftyMove>=100 || DeadMaterial(pos)) return TRUE;
 U64 key=RepetitionKey(pos);
 int first=pos->hisPly-pos->fiftyMove,matches=0;
 if(first<0) first=0;
 for(int i=pos->hisPly-1;i>=first;i--) {
  if(pos->history[i].move==NOMOVE) break;
  if(pos->history[i].posKey==key && ++matches==2) return TRUE;
 }
 return FALSE;
}

int GetPvLine(const int depth, S_BOARD *pos) {

	ASSERT(depth < MAXDEPTH && depth >= 1);

	int move = ProbePvMove(pos);
	int count = 0;
	int startPly = pos->ply;

	while (move != NOMOVE && count < depth) {

		ASSERT(count < MAXDEPTH);

		if (MoveExists(pos, move)) {
			MakeMove(pos, move);
			pos->PvArray[count++] = move;
		} else {
			break;
		}

		if (PvDrawReached(pos)) break;
		move = ProbePvMove(pos);
	}

	while (pos->ply > startPly) {
		TakeMove(pos);
	}

	return count;
}

void ClearHashTable(S_HASHTABLE *table) {
 if(table->borrowed || HasBorrowers(table)) return; /* Lifecycle operations belong to the owner. */

	ASSERT(table != NULL);

	if (table->pTable != NULL && table->numEntries > 0) {
		// NOMOVE == 0 in your codebase, so memset is safe
		memset(table->pTable, 0, (size_t)table->numEntries * sizeof(S_HASHENTRY));
	}

	table->newWrite = 0;
	table->overWrite = 0;
	table->hit = 0;
	table->cut = 0;
}

void InitHashTable(S_HASHTABLE *table, const int MB) {
 if(table->borrowed || HasBorrowers(table)) return;

	ASSERT(table != NULL);

 FreeHashTable(table);

	table->numEntries = 0;
	table->generation = 0;
	table->newWrite = 0;
	table->overWrite = 0;
	table->hit = 0;
	table->cut = 0;

	int mbTry = MB;
	if (mbTry < 1) mbTry = 1;

	while (mbTry >= 1) {

		int HashSize = 0x100000 * mbTry; // MB -> bytes
		int entries = HashSize / (int)sizeof(S_HASHENTRY);

		// keep a little slack; then force power-of-two for bitmask indexing
		entries -= 2;
		entries = LargestPow2LEQ(entries);

		if (entries < 2) entries = 2;

		S_HASHENTRY *mem = (S_HASHENTRY *)malloc((size_t)entries * sizeof(S_HASHENTRY));
		if (mem != NULL) {
			table->pTable = mem;
			table->numEntries = entries;
			ClearHashTable(table);

			return;
		}

		fprintf(stderr,"Hash Allocation Failed, trying %dMB...\n", mbTry / 2);
		mbTry /= 2;
	}

	// If we get here, allocation failed even at 1MB
	fprintf(stderr,"Hash Allocation Failed completely. Running without hash table.\n");
	table->pTable = NULL;
	table->numEntries = 0;
	table->generation = 0;
}

int ProbeHashEntry(S_BOARD *pos, int *move, int *score, int alpha, int beta, int depth) {

	ASSERT(pos != NULL);
	ASSERT(pos->HashTable != NULL);
	ASSERT(move != NULL);
	ASSERT(score != NULL);
	ASSERT(depth >= 1 && depth < MAXDEPTH);
	ASSERT(alpha < beta);
	ASSERT(alpha >= -INFINITE && alpha <= INFINITE);
	ASSERT(beta >= -INFINITE && beta <= INFINITE);
	ASSERT(pos->ply >= 0 && pos->ply < MAXDEPTH);

	if (pos->HashTable->pTable == NULL || pos->HashTable->numEntries < 2) {
		return FALSE;
	}

	int index = HashIndex(pos);
	ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);

	S_HASHENTRY snapshot=ReadHashEntry(pos->HashTable,index);
 const S_HASHENTRY *entry=&snapshot;

	if (entry->posKey == pos->posKey) {

		*move = entry->move;

		if (entry->depth >= depth && entry->fiftyMove == pos->fiftyMove && entry->context == DrawContext(pos)) {
			pos->HashTable->hit++;

			ASSERT(entry->depth >= 1 && entry->depth < MAXDEPTH);
			ASSERT(entry->flags >= HFALPHA && entry->flags <= HFEXACT);

			*score = entry->score;

			// Fix mate scores to be relative to current ply
			if (*score > ISMATE) *score -= pos->ply;
			else if (*score < -ISMATE) *score += pos->ply;

			switch (entry->flags) {

				case HFALPHA:
					if (*score <= alpha) {
						*score = alpha;
						return TRUE;
					}
					break;

				case HFBETA:
					if (*score >= beta) {
						*score = beta;
						return TRUE;
					}
					break;

				case HFEXACT:
					return TRUE;

				default:
					ASSERT(FALSE);
					break;
			}
		}
	}

	return FALSE;
}

void StoreHashEntry(S_BOARD *pos, const int move, int score, const int flags, const int depth) {

	ASSERT(pos != NULL);
	ASSERT(pos->HashTable != NULL);
	ASSERT(depth >= 1 && depth < MAXDEPTH);
	ASSERT(flags >= HFALPHA && flags <= HFEXACT);
	ASSERT(score >= -INFINITE && score <= INFINITE);
	ASSERT(pos->ply >= 0 && pos->ply < MAXDEPTH);

	if (pos->HashTable->pTable == NULL || pos->HashTable->numEntries < 2) {
		return;
	}

	int index = HashIndex(pos);
	ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);

 U64 context=DrawContext(pos);
 JaceMutex *lock=HashLock(pos->HashTable,index);if(lock) JaceMutexLock(lock);
	S_HASHENTRY *entry = &pos->HashTable->pTable[index];
    if(entry->posKey==pos->posKey && entry->generation==pos->HashTable->generation && entry->depth>depth && flags!=HFEXACT && entry->fiftyMove==pos->fiftyMove && entry->context==context) goto done;

	// Simple replacement policy:
	// - always replace if same key (update)
	// - otherwise, prefer deeper entries (keep higher depth info)
	if (entry->posKey != 0ULL && entry->posKey != pos->posKey) {
		if (entry->generation == pos->HashTable->generation && entry->depth > depth) {
			goto done; // keep the deeper entry
		}
	}

	if (entry->posKey == 0ULL) {
		pos->HashTable->newWrite++;
	} else {
		pos->HashTable->overWrite++;
	}

	// Store mate scores as "from root" so they can be corrected on probe
	if (score > ISMATE) score += pos->ply;
	else if (score < -ISMATE) score -= pos->ply;

	entry->fiftyMove = pos->fiftyMove;
	entry->context = context;
	entry->generation = pos->HashTable->generation;
	entry->move = move;
	entry->posKey = pos->posKey;
	entry->flags = flags;
	entry->score = score;
	entry->depth = depth;
done:
 if(lock) JaceMutexUnlock(lock);
}

int ProbePvMove(const S_BOARD *pos) {

	ASSERT(pos != NULL);
	ASSERT(pos->HashTable != NULL);

	if (pos->HashTable->pTable == NULL || pos->HashTable->numEntries < 2) {
		return NOMOVE;
	}

	int index = (int)(pos->posKey & (U64)(pos->HashTable->numEntries - 1));
	ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);

	S_HASHENTRY entry=ReadHashEntry(pos->HashTable,index);
	if (entry.posKey == pos->posKey) {
		return entry.move;
	}

	return NOMOVE;
}

/* Detaching a borrowed view never releases the owner's allocation. All views
   must be idle and detached before their owner is resized or destroyed. */
void FreeHashTable(S_HASHTABLE *table) {
 if(table->borrowed) {
  if(table->sync) atomic_fetch_sub_explicit(&table->sync->borrowers,1,memory_order_release);
 } else {
  if(HasBorrowers(table)) return;
  if(table->sync) {
   for(unsigned i=0;i<HASH_STRIPES;i++) JaceMutexDestroy(table->sync->locks[i]);
   free(table->sync);
  }
  free(table->pTable);
 }
 memset(table,0,sizeof *table);
}
/* Destination must be fresh (zero initialized or already detached). Copies of
   a live board made with assignment remain diagnostic snapshots, not owners. */
void CopyBoardForSearch(S_BOARD *dst,const S_BOARD *src) {
 ASSERT(dst!=src);
 ASSERT(dst->HashTable->pTable==NULL);
 if(dst==src || dst->HashTable->pTable!=NULL) return;
 *dst=*src;
 dst->HashTable->borrowed=TRUE;
 if(dst->HashTable->sync) atomic_fetch_add_explicit(&dst->HashTable->sync->borrowers,1,memory_order_relaxed);
 dst->HashTable->newWrite=dst->HashTable->overWrite=0;
 dst->HashTable->hit=dst->HashTable->cut=0;
}
