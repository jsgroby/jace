// see.c
// Static Exchange Evaluation (SEE) for capture moves.
// Conservative, simple implementation suitable for 120-square board representation.

#include "defs.h"

static int PieceAttacksSq(const S_BOARD *pos, const int from, const int pce, const int to) {

	// Pawns
	if(pce == wP) {
		if(from + 9 == to || from + 11 == to) return TRUE;
		return FALSE;
	}
	if(pce == bP) {
		if(from - 9 == to || from - 11 == to) return TRUE;
		return FALSE;
	}

	// Knights
	if(pce == wN || pce == bN) {
		for(int i = 0; i < 8; ++i) {
			if(from + KnDir[i] == to) return TRUE;
		}
		return FALSE;
	}

	// Kings
	if(pce == wK || pce == bK) {
		for(int i = 0; i < 8; ++i) {
			if(from + KiDir[i] == to) return TRUE;
		}
		return FALSE;
	}

	// Sliding pieces
	if(pce == wB || pce == bB || pce == wQ || pce == bQ) {
		for(int d = 0; d < 4; ++d) {
			int sq = from + BiDir[d];
			while(pos->pieces[sq] != OFFBOARD) {
				if(sq == to) return TRUE;
				if(pos->pieces[sq] != EMPTY) break;
				sq += BiDir[d];
			}
		}
	}

	if(pce == wR || pce == bR || pce == wQ || pce == bQ) {
		for(int d = 0; d < 4; ++d) {
			int sq = from + RkDir[d];
			while(pos->pieces[sq] != OFFBOARD) {
				if(sq == to) return TRUE;
				if(pos->pieces[sq] != EMPTY) break;
				sq += RkDir[d];
			}
		}
	}

	return FALSE;
}

static int FindLeastValuableAttacker(const S_BOARD *pos, const int side, const int to, int *fromSq) {

	int bestSq = NO_SQ;
	int bestVal = 1000000;

	for(int sq = 0; sq < BRD_SQ_NUM; ++sq) {

		int pce = pos->pieces[sq];
		if(pce == OFFBOARD || pce == EMPTY) continue;
		if(PieceCol[pce] != side) continue;

		if(!PieceAttacksSq(pos, sq, pce, to)) continue;

		int v = PieceVal[pce];
		if(v < bestVal) {
			bestVal = v;
			bestSq = sq;

			// Can't do better than a pawn.
			if(bestVal <= PieceVal[wP]) break;
		}
	}

	*fromSq = bestSq;
	return (bestSq != NO_SQ);
}

int SEE(const S_BOARD *pos, int move) {

	ASSERT(CheckBoard(pos));

	// Only meaningful for captures / en-passant. Promotions are handled.
	const int isEp = (move & MFLAGEP) ? TRUE : FALSE;
	const int cap = CAPTURED(move);

	if(cap == EMPTY && !isEp) {
		return 0;
	}

	// We do a lightweight "swap" sequence on a copied position.
	S_BOARD tmp = *pos; // struct copy (includes pieces[] array)
	tmp.ply = 0;
	tmp.hisPly = 0;

	const int from = FROMSQ(move);
	const int to = TOSQ(move);

	int side = tmp.side;
	int moving = tmp.pieces[from];

	// Determine initial victim value
	int victimPce = cap;
	if(isEp) {
		victimPce = (side == WHITE) ? bP : wP;
	}

	int gain[32];
	int depth = 0;

	gain[0] = PieceVal[victimPce];

	// Make the capture on the temp board (minimal, not using MakeMove)
	tmp.pieces[to] = moving;
	tmp.pieces[from] = EMPTY;

	// remove en-passant pawn
	if(isEp) {
		int epSq = (side == WHITE) ? (to - 10) : (to + 10);
		ASSERT(SqOnBoard(epSq));
		tmp.pieces[epSq] = EMPTY;
	}

	// promotion: piece on to becomes promoted piece
	const int prom = PROMOTED(move);
	if(prom != EMPTY) {
		tmp.pieces[to] = prom;
		moving = prom;
	}

	// The piece currently on 'to' is what will be captured next
	int capturedOnTo = tmp.pieces[to];

	// Alternate captures
	side ^= 1;

	while(depth < 30) {

		int attSq = NO_SQ;
		if(!FindLeastValuableAttacker(&tmp, side, to, &attSq)) {
			break;
		}

		int attPce = tmp.pieces[attSq];

		// next ply gain: attacker value - previous gain
		depth++;
		gain[depth] = PieceVal[attPce] - gain[depth - 1];

		// If this side already does worse by continuing, it might still be forced,
		// but SEE assumes optimal play; we can cut when the sequence can't improve.
		if(gain[depth] < 0 && depth >= 2) {
			// keep going a little is usually unnecessary; break is fine for this simple SEE
			// break;
			;
		}

		// Make the capture: move attacker to 'to'
		tmp.pieces[to] = attPce;
		tmp.pieces[attSq] = EMPTY;

		capturedOnTo = attPce;

		side ^= 1;
	}

	// minimax backwards
	for(int i = depth - 1; i >= 0; --i) {
		if(-gain[i + 1] > gain[i]) {
			gain[i] = -gain[i + 1];
		}
	}

	return gain[0];
}
