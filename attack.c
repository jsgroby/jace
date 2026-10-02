// attack.c

#include "stdio.h"
#include "defs.h"
#include <string.h>

const int KnDir[8] = { -8, -19,	-21, -12, 8, 19, 21, 12 };
const int RkDir[4] = { -1, -10,	1, 10 };
const int BiDir[4] = { -9, -11, 11, 9 };
const int KiDir[8] = { -1, -10,	1, 10, -9, -11, 11, 9 };

static int RawSqAttacked(const int sq, const int side, const int *pieces) {

	int pce,index,t_sq,dir;
	
	ASSERT(SqOnBoard(sq));
	ASSERT(SideValid(side));

	
	// pawns
	if(side == WHITE) {
		if(pieces[sq-11] == wP || pieces[sq-9] == wP) {
			return TRUE;
		}
	} else {
		if(pieces[sq+11] == bP || pieces[sq+9] == bP) {
			return TRUE;
		}	
	}
	
	// knights
	for(index = 0; index < 8; ++index) {		
		pce = pieces[sq + KnDir[index]];
		ASSERT(PceValidEmptyOffbrd(pce));
		if(pce != OFFBOARD && IsKn(pce) && PieceCol[pce] == side) {
			return TRUE;
		}
	}
	
	// rooks, queens
	for(index = 0; index < 4; ++index) {		
		dir = RkDir[index];
		t_sq = sq + dir;
		ASSERT(SqIs120(t_sq));
		pce = pieces[t_sq];
		ASSERT(PceValidEmptyOffbrd(pce));
		while(pce != OFFBOARD) {
			if(pce != EMPTY) {
				if(IsRQ(pce) && PieceCol[pce] == side) {
					return TRUE;
				}
				break;
			}
			t_sq += dir;
			ASSERT(SqIs120(t_sq));
			pce = pieces[t_sq];
		}
	}
	
	// bishops, queens
	for(index = 0; index < 4; ++index) {		
		dir = BiDir[index];
		t_sq = sq + dir;
		ASSERT(SqIs120(t_sq));
		pce = pieces[t_sq];
		ASSERT(PceValidEmptyOffbrd(pce));
		while(pce != OFFBOARD) {
			if(pce != EMPTY) {
				if(IsBQ(pce) && PieceCol[pce] == side) {
					return TRUE;
				}
				break;
			}
			t_sq += dir;
			ASSERT(SqIs120(t_sq));
			pce = pieces[t_sq];
		}
	}
	
	// kings
	for(index = 0; index < 8; ++index) {		
		pce = pieces[sq + KiDir[index]];
		ASSERT(PceValidEmptyOffbrd(pce));
		if(pce != OFFBOARD && IsKi(pce) && PieceCol[pce]==side) {
			return TRUE;
		}
	}
	
	return FALSE;
	
}
int SqAttacked(const int sq,const int side,const S_BOARD *pos) {
 ASSERT(CheckBoard(pos));return RawSqAttacked(sq,side,pos->pieces);
}
U64 RepetitionKey(const S_BOARD *pos) {
 if(pos->enPas==NO_SQ) return pos->posKey;
 int to=pos->enPas,step=pos->side==WHITE?10:-10,pawn=pos->side==WHITE?wP:bP;
 for(int d=-1;d<=1;d+=2) {
  int from=to-step+d;
  if(pos->pieces[from]!=pawn) continue;
  int pieces[BRD_SQ_NUM];memcpy(pieces,pos->pieces,sizeof pieces);
  pieces[from]=EMPTY;pieces[to-step]=EMPTY;pieces[to]=pawn;
  if(!RawSqAttacked(pos->KingSq[pos->side],pos->side^1,pieces)) return pos->posKey;
 }
 return pos->posKey ^ PieceKeys[EMPTY][pos->enPas];
}
