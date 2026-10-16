// pvtable.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"

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

	ASSERT(table != NULL);

	// free any existing table
	if (table->pTable != NULL) {
		free(table->pTable);
		table->pTable = NULL;
	}

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

	const S_HASHENTRY *entry = &pos->HashTable->pTable[index];

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

	S_HASHENTRY *entry = &pos->HashTable->pTable[index];
    if(entry->posKey==pos->posKey && entry->generation==pos->HashTable->generation && entry->depth>depth && flags!=HFEXACT && entry->fiftyMove==pos->fiftyMove && entry->context==DrawContext(pos)) return;

	// Simple replacement policy:
	// - always replace if same key (update)
	// - otherwise, prefer deeper entries (keep higher depth info)
	if (entry->posKey != 0ULL && entry->posKey != pos->posKey) {
		if (entry->generation == pos->HashTable->generation && entry->depth > depth) {
			return; // keep the deeper entry
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
	entry->context = DrawContext(pos);
	entry->generation = pos->HashTable->generation;
	entry->move = move;
	entry->posKey = pos->posKey;
	entry->flags = flags;
	entry->score = score;
	entry->depth = depth;
}

int ProbePvMove(const S_BOARD *pos) {

	ASSERT(pos != NULL);
	ASSERT(pos->HashTable != NULL);

	if (pos->HashTable->pTable == NULL || pos->HashTable->numEntries < 2) {
		return NOMOVE;
	}

	int index = (int)(pos->posKey & (U64)(pos->HashTable->numEntries - 1));
	ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);

	if (pos->HashTable->pTable[index].posKey == pos->posKey) {
		return pos->HashTable->pTable[index].move;
	}

	return NOMOVE;
}
