/* Frozen pre-cache evaluator for differential tests. */
// evaluate.c

#include "stdio.h"
#include "defs.h"

const int ReferencePawnIsolated = -10;
const int ReferencePawnPassed[8] = { 0, 5, 10, 20, 35, 60, 100, 200 };
const int ReferenceRookOpenFile = 10;
const int ReferenceRookSemiOpenFile = 5;
const int ReferenceQueenOpenFile = 5;
const int ReferenceQueenSemiOpenFile = 3;
const int ReferenceBishopPair = 30;

const int ReferencePawnTable[64] = {
0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,
10	,	10	,	0	,	-10	,	-10	,	0	,	10	,	10	,
5	,	0	,	0	,	5	,	5	,	0	,	0	,	5	,
0	,	0	,	10	,	20	,	20	,	10	,	0	,	0	,
5	,	5	,	5	,	10	,	10	,	5	,	5	,	5	,
10	,	10	,	10	,	20	,	20	,	10	,	10	,	10	,
20	,	20	,	20	,	30	,	30	,	20	,	20	,	20	,
0	,	0	,	0	,	0	,	0	,	0	,	0	,	0
};

const int ReferenceKnightTable[64] = {
0	,	-10	,	0	,	0	,	0	,	0	,	-10	,	0	,
0	,	0	,	0	,	5	,	5	,	0	,	0	,	0	,
0	,	0	,	10	,	10	,	10	,	10	,	0	,	0	,
0	,	0	,	10	,	20	,	20	,	10	,	5	,	0	,
5	,	10	,	15	,	20	,	20	,	15	,	10	,	5	,
5	,	10	,	10	,	20	,	20	,	10	,	10	,	5	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	0	,	0	,	0	,	0	,	0	,	0
};

const int ReferenceBishopTable[64] = {
0	,	0	,	-10	,	0	,	0	,	-10	,	0	,	0	,
0	,	0	,	0	,	10	,	10	,	0	,	0	,	0	,
0	,	0	,	10	,	15	,	15	,	10	,	0	,	0	,
0	,	10	,	15	,	20	,	20	,	15	,	10	,	0	,
0	,	10	,	15	,	20	,	20	,	15	,	10	,	0	,
0	,	0	,	10	,	15	,	15	,	10	,	0	,	0	,
0	,	0	,	0	,	10	,	10	,	0	,	0	,	0	,
0	,	0	,	0	,	0	,	0	,	0	,	0	,	0
};

const int ReferenceRookTable[64] = {
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
25	,	25	,	25	,	25	,	25	,	25	,	25	,	25	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0
};

const int ReferenceKingE[64] = {
	-50	,	-10	,	0	,	0	,	0	,	0	,	-10	,	-50	,
	-10,	0	,	10	,	10	,	10	,	10	,	0	,	-10	,
	0	,	10	,	20	,	20	,	20	,	20	,	10	,	0	,
	0	,	10	,	20	,	40	,	40	,	20	,	10	,	0	,
	0	,	10	,	20	,	40	,	40	,	20	,	10	,	0	,
	0	,	10	,	20	,	20	,	20	,	20	,	10	,	0	,
	-10,	0	,	10	,	10	,	10	,	10	,	0	,	-10	,
	-50	,	-10	,	0	,	0	,	0	,	0	,	-10	,	-50
};

const int ReferenceKingO[64] = {
	0	,	5	,	5	,	-10	,	-10	,	0	,	10	,	5	,
	-30	,	-30	,	-30	,	-30	,	-30	,	-30	,	-30	,	-30	,
	-50	,	-50	,	-50	,	-50	,	-50	,	-50	,	-50	,	-50	,
	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,
	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,
	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,
	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,
	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70
};

// sjeng 11.2
//8/6R1/2k5/6P1/8/8/4nP2/6K1 w - - 1 41
int ReferenceDeadMaterial(const S_BOARD *pos) {
 if(pos->pceNum[wP] || pos->pceNum[bP] || pos->pceNum[wR] || pos->pceNum[bR] || pos->pceNum[wQ] || pos->pceNum[bQ]) return FALSE;
 int knights=pos->pceNum[wN]+pos->pceNum[bN];
 int bishops=pos->pceNum[wB]+pos->pceNum[bB];
 if(knights+bishops<=1) return TRUE;
 if(knights) return FALSE;
 int color=-1;
 for(int piece=wB;piece<=bB;piece+=bB-wB) {
  for(int i=0;i<pos->pceNum[piece];i++) {
   int sq=pos->pList[piece][i], c=(FilesBrd[sq]+RanksBrd[sq])&1;
   if(color>=0 && color!=c) return FALSE;
   color=c;
  }
 }
 return TRUE;
}

#define ENDGAME_MAT (1 * PieceVal[wR] + 2 * PieceVal[wN] + 2 * PieceVal[wP] + PieceVal[wK])

static int ReferencePawnAttacked(const S_BOARD *pos,int sq,int enemy) {
 return enemy==WHITE ? pos->pieces[sq-9]==wP || pos->pieces[sq-11]==wP : pos->pieces[sq+9]==bP || pos->pieces[sq+11]==bP;
}
static int ReferenceMobility(const S_BOARD *pos,int sq,int piece,int side) {
 int n=0;
 for(int i=0;i<8;i++) {
  int dir=IsKn(piece)?KnDir[i]:KiDir[i];
  if(!IsKn(piece) && (i<4 ? !IsRQ(piece):!IsBQ(piece))) continue;
  for(int to=sq+dir;pos->pieces[to]!=OFFBOARD;to+=dir) {
   int victim=pos->pieces[to];
   if(victim!=EMPTY && PieceCol[victim]==side) break;
   if(!ReferencePawnAttacked(pos,to,side^1)) n++;
   if(victim!=EMPTY || IsKn(piece)) break;
  }
 }
 return n;
}
static int ReferencePawnShield(const S_BOARD *pos,int side) {
 int king=pos->KingSq[side],file=FilesBrd[king],rank=RanksBrd[king],step=side==WHITE?1:-1;
 int ownPawn=side==WHITE?wP:bP,score=0;
 for(int f=file-1;f<=file+1;f++) {
  if(f<0 || f>7) continue;
  if(!(pos->pawns[side]&FileBBMask[f])) score-=12;
  int near=rank+step,far=rank+2*step;
  if(near>=0 && near<8 && pos->pieces[FR2SQ(f,near)]==ownPawn) score+=10;
  else if(far>=0 && far<8 && pos->pieces[FR2SQ(f,far)]==ownPawn) score+=5;
 }
 /* King cover matters in proportion to the opponent's attacking material. */
 int enemy=side^1,offset=enemy==WHITE?0:6;
 int attack=pos->pceNum[wN+offset]+pos->pceNum[wB+offset]+2*pos->pceNum[wR+offset]+4*pos->pceNum[wQ+offset];
 if(attack>12) attack=12;
 return score*attack/12;
}

/* Independent slow board scan for the new term; legacy terms below remain
 * the uncached differential oracle. Tests can disable only this adjustment. */
static int referenceKingProximity=TRUE;
static int ReferencePassedKings(const S_BOARD *pos) {
 int score=0;
 for(int rank=1;rank<=6;rank++) for(int file=0;file<8;file++) {
  int piece=pos->pieces[FR2SQ(file,rank)];
  if(piece!=wP && piece!=bP) continue;
  int side=piece==wP?WHITE:BLACK,step=side==WHITE?1:-1;
  int relative=side==WHITE?rank:7-rank;
  if(relative<3) continue;
  int eligible=TRUE;
  for(int r=rank+step;r>=0 && r<8;r+=step) for(int f=0;f<8;f++) {
   int occupant=pos->pieces[FR2SQ(f,r)];
   if((f==file && occupant==piece) ||
      (abs(f-file)<=1 && occupant==(side==WHITE?bP:wP))) eligible=FALSE;
  }
  if(!eligible) continue;
  int distances[2];
  for(int color=WHITE;color<=BLACK;color++) {
   int df=abs(FilesBrd[pos->KingSq[color]]-file);
   int dr=abs(RanksBrd[pos->KingSq[color]]-(rank+step));
   distances[color]=(df+dr+abs(df-dr))/2;
  }
  int value=2*(relative-2)*(distances[side^1]-distances[side]);
  score+=side==WHITE?value:-value;
 }
 return score;
}

int ReferenceEvalPosition(const S_BOARD *pos) {

	ASSERT(CheckBoard(pos));

	int pce;
	int pceNum;
	int sq;
	int score = pos->material[WHITE] - pos->material[BLACK];
 int mg=0,eg=0;
 int phase=pos->pceNum[wN]+pos->pceNum[bN]+pos->pceNum[wB]+pos->pceNum[bB]+2*(pos->pceNum[wR]+pos->pceNum[bR])+4*(pos->pceNum[wQ]+pos->pceNum[bQ]);
 if(phase>24) phase=24;

	if(!pos->pceNum[wP] && !pos->pceNum[bP] && ReferenceDeadMaterial(pos) == TRUE) {
		return 0;
	}

	pce = wP;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += ReferencePawnTable[SQ64(sq)];

		if( (IsolatedMask[SQ64(sq)] & pos->pawns[WHITE]) == 0) {
			//printf("wP Iso:%s\n",PrSq(sq));
			score += ReferencePawnIsolated;
		}

		if( (WhitePassedMask[SQ64(sq)] & pos->pawns[BLACK]) == 0) {
			//printf("wP Passed:%s\n",PrSq(sq));
			score += ReferencePawnPassed[RanksBrd[sq]];
		}

	}

	pce = bP;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= ReferencePawnTable[MIRROR64(SQ64(sq))];

		if( (IsolatedMask[SQ64(sq)] & pos->pawns[BLACK]) == 0) {
			//printf("bP Iso:%s\n",PrSq(sq));
			score -= ReferencePawnIsolated;
		}

		if( (BlackPassedMask[SQ64(sq)] & pos->pawns[WHITE]) == 0) {
			//printf("bP Passed:%s\n",PrSq(sq));
			score -= ReferencePawnPassed[7 - RanksBrd[sq]];
		}
	}

	pce = wN;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += ReferenceKnightTable[SQ64(sq)];
	}

	pce = bN;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= ReferenceKnightTable[MIRROR64(SQ64(sq))];
	}

	pce = wB;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += ReferenceBishopTable[SQ64(sq)];
	}

	pce = bB;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= ReferenceBishopTable[MIRROR64(SQ64(sq))];
	}

	pce = wR;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += ReferenceRookTable[SQ64(sq)];

		ASSERT(FileRankValid(FilesBrd[sq]));

		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score += ReferenceRookOpenFile;
		} else if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			score += ReferenceRookSemiOpenFile;
		}
	}

	pce = bR;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= ReferenceRookTable[MIRROR64(SQ64(sq))];
		ASSERT(FileRankValid(FilesBrd[sq]));
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score -= ReferenceRookOpenFile;
		} else if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			score -= ReferenceRookSemiOpenFile;
		}
	}

	pce = wQ;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		ASSERT(FileRankValid(FilesBrd[sq]));
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score += ReferenceQueenOpenFile;
		} else if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			score += ReferenceQueenSemiOpenFile;
		}
	}

	pce = bQ;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		ASSERT(FileRankValid(FilesBrd[sq]));
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score -= ReferenceQueenOpenFile;
		} else if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			score -= ReferenceQueenSemiOpenFile;
		}
	}
 if(EngineOptions->TaperedEval) {
  mg += ReferenceKingO[SQ64(pos->KingSq[WHITE])] - ReferenceKingO[MIRROR64(SQ64(pos->KingSq[BLACK]))];
  eg += ReferenceKingE[SQ64(pos->KingSq[WHITE])] - ReferenceKingE[MIRROR64(SQ64(pos->KingSq[BLACK]))];
  mg += ReferencePawnShield(pos,WHITE)-ReferencePawnShield(pos,BLACK);
  if(referenceKingProximity) eg += ReferencePassedKings(pos);
  for(int side=WHITE;side<=BLACK;side++) {
   int sign=side==WHITE?1:-1,offset=side==WHITE?0:6;
   for(int piece=wN;piece<=wQ;piece++) for(int i=0;i<pos->pceNum[piece+offset];i++) {
    int count=ReferenceMobility(pos,pos->pList[piece+offset][i],piece+offset,side);
    int mgWeight=piece==wN?4:piece==wB?4:piece==wR?2:1;
    int egWeight=piece==wN?3:piece==wB?4:piece==wR?3:2;
    mg+=sign*count*mgWeight;eg+=sign*count*egWeight;
   }
   int pawn=wP+offset;
   for(int i=0;i<pos->pceNum[pawn];i++) {
    int square=pos->pList[pawn][i],rank=RanksBrd[square];
    int relative=side==WHITE?rank:7-rank;
    U64 passed=side==WHITE?WhitePassedMask[SQ64(square)]:BlackPassedMask[SQ64(square)];
    if(!(passed&pos->pawns[side^1])) mg-=sign*(ReferencePawnPassed[relative]/2);
    int behind=side==WHITE?-10:10;
    int supported=pos->pieces[square+behind-1]==pawn || pos->pieces[square+behind+1]==pawn;
    int connected=pos->pieces[square-1]==pawn || pos->pieces[square+1]==pawn;
    if(supported || connected) {mg+=sign*5;eg+=sign*8;}
    /* Penalize the rear pawn on a doubled file; avoid double-counting pairs. */
    int ahead=FALSE,step=side==WHITE?10:-10;
    for(int to=square+step;pos->pieces[to]!=OFFBOARD;to+=step) if(pos->pieces[to]==pawn) {ahead=TRUE;break;}
    if(ahead) {
     mg-=sign*10;eg-=sign*15;
     if(!(passed&pos->pawns[side^1])) {mg-=sign*(ReferencePawnPassed[relative]-ReferencePawnPassed[relative]/2);eg-=sign*ReferencePawnPassed[relative];}
    }
   }
  }
  score+=(mg*phase+eg*(24-phase))/24;
 } else {
  int whiteSq=SQ64(pos->KingSq[WHITE]),blackSq=MIRROR64(SQ64(pos->KingSq[BLACK]));
  score+=pos->material[BLACK]<=ENDGAME_MAT?ReferenceKingE[whiteSq]:ReferenceKingO[whiteSq];
  score-=pos->material[WHITE]<=ENDGAME_MAT?ReferenceKingE[blackSq]:ReferenceKingO[blackSq];
 }

	if(pos->pceNum[wB] >= 2) score += ReferenceBishopPair;
	if(pos->pceNum[bB] >= 2) score -= ReferenceBishopPair;

	if(pos->side == WHITE) {
		return score;
	} else {
		return -score;
	}
}
