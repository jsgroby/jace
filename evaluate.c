// evaluate.c

#include "stdio.h"
#include "defs.h"

const int PawnIsolated = -10;
const int PawnPassed[8] = { 0, 5, 10, 20, 35, 60, 100, 200 }; 
const int RookOpenFile = 10;
const int RookSemiOpenFile = 5;
const int QueenOpenFile = 5;
const int QueenSemiOpenFile = 3;
const int BishopPair = 30;

const int PawnTable[64] = {
0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,
10	,	10	,	0	,	-10	,	-10	,	0	,	10	,	10	,
5	,	0	,	0	,	5	,	5	,	0	,	0	,	5	,
0	,	0	,	10	,	20	,	20	,	10	,	0	,	0	,
5	,	5	,	5	,	10	,	10	,	5	,	5	,	5	,
10	,	10	,	10	,	20	,	20	,	10	,	10	,	10	,
20	,	20	,	20	,	30	,	30	,	20	,	20	,	20	,
0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	
};

const int KnightTable[64] = {
0	,	-10	,	0	,	0	,	0	,	0	,	-10	,	0	,
0	,	0	,	0	,	5	,	5	,	0	,	0	,	0	,
0	,	0	,	10	,	10	,	10	,	10	,	0	,	0	,
0	,	0	,	10	,	20	,	20	,	10	,	5	,	0	,
5	,	10	,	15	,	20	,	20	,	15	,	10	,	5	,
5	,	10	,	10	,	20	,	20	,	10	,	10	,	5	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	0	,	0	,	0	,	0	,	0	,	0		
};

const int BishopTable[64] = {
0	,	0	,	-10	,	0	,	0	,	-10	,	0	,	0	,
0	,	0	,	0	,	10	,	10	,	0	,	0	,	0	,
0	,	0	,	10	,	15	,	15	,	10	,	0	,	0	,
0	,	10	,	15	,	20	,	20	,	15	,	10	,	0	,
0	,	10	,	15	,	20	,	20	,	15	,	10	,	0	,
0	,	0	,	10	,	15	,	15	,	10	,	0	,	0	,
0	,	0	,	0	,	10	,	10	,	0	,	0	,	0	,
0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	
};

const int RookTable[64] = {
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
25	,	25	,	25	,	25	,	25	,	25	,	25	,	25	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0		
};

const int KingE[64] = {	
	-50	,	-10	,	0	,	0	,	0	,	0	,	-10	,	-50	,
	-10,	0	,	10	,	10	,	10	,	10	,	0	,	-10	,
	0	,	10	,	20	,	20	,	20	,	20	,	10	,	0	,
	0	,	10	,	20	,	40	,	40	,	20	,	10	,	0	,
	0	,	10	,	20	,	40	,	40	,	20	,	10	,	0	,
	0	,	10	,	20	,	20	,	20	,	20	,	10	,	0	,
	-10,	0	,	10	,	10	,	10	,	10	,	0	,	-10	,
	-50	,	-10	,	0	,	0	,	0	,	0	,	-10	,	-50	
};

const int KingO[64] = {	
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
int DeadMaterial(const S_BOARD *pos) {
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

static int PawnAttacked(const S_BOARD *pos,int sq,int enemy) {
 return enemy==WHITE ? pos->pieces[sq-9]==wP || pos->pieces[sq-11]==wP : pos->pieces[sq+9]==bP || pos->pieces[sq+11]==bP;
}
static int Mobility(const S_BOARD *pos,int sq,int piece,int side) {
 int n=0;
 for(int i=0;i<8;i++) {
  int dir=IsKn(piece)?KnDir[i]:KiDir[i];
  if(!IsKn(piece) && (i<4 ? !IsRQ(piece):!IsBQ(piece))) continue;
  for(int to=sq+dir;pos->pieces[to]!=OFFBOARD;to+=dir) {
   int victim=pos->pieces[to];
   if(victim!=EMPTY && PieceCol[victim]==side) break;
   if(!PawnAttacked(pos,to,side^1)) n++;
   if(victim!=EMPTY || IsKn(piece)) break;
  }
 }
 return n;
}
static int PawnShield(const S_BOARD *pos,int side) {
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

/* King-dependent passer support belongs outside the pawn-only cache.
 * Chebyshev distance counts king steps to the pawn's next square; it is a
 * positional hint, not a proof that a king can catch or escort the pawn. */
static int KingDistance(int king,int square) {
 int file=abs(FilesBrd[king]-FilesBrd[square]);
 int rank=abs(RanksBrd[king]-RanksBrd[square]);
 return file>rank?file:rank;
}
static int PassedKingProximity(const S_BOARD *pos) {
 static const int weight[8]={0,0,0,2,4,6,8,0};
 int score=0;
 for(int side=WHITE;side<=BLACK;side++) {
  int pawn=side==WHITE?wP:bP,step=side==WHITE?10:-10;
  for(int i=0;i<pos->pceNum[pawn];i++) {
   int square=pos->pList[pawn][i];
   int rank=side==WHITE?RanksBrd[square]:7-RanksBrd[square];
   if(!weight[rank]) continue;
   U64 mask=side==WHITE?WhitePassedMask[SQ64(square)]:BlackPassedMask[SQ64(square)];
   if(mask&pos->pawns[side^1]) continue;
   /* Only the front pawn on a doubled file receives this adjustment. */
   int front=TRUE;
   for(int to=square+step;pos->pieces[to]!=OFFBOARD;to+=step)
    if(pos->pieces[to]==pawn) {front=FALSE;break;}
   if(!front) continue;
   int next=square+step;
   int value=weight[rank]*(KingDistance(pos->KingSq[side^1],next)-KingDistance(pos->KingSq[side],next));
   score+=side==WHITE?value:-value;
  }
 }
 return score;
}

/* Only pawn-dependent terms belong here. King shelter, mobility, material,
   phase and side-to-move remain outside the cache. */
static S_PAWNSCORE CalculatePawnScore(const S_BOARD *pos) {
 int pce,pceNum,sq,score=0,mg=0,eg=0;
	pce = wP;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += PawnTable[SQ64(sq)];	
		
		if( (IsolatedMask[SQ64(sq)] & pos->pawns[WHITE]) == 0) {
			//printf("wP Iso:%s\n",PrSq(sq));
			score += PawnIsolated;
		}
		
		if( (WhitePassedMask[SQ64(sq)] & pos->pawns[BLACK]) == 0) {
			//printf("wP Passed:%s\n",PrSq(sq));
			score += PawnPassed[RanksBrd[sq]];
		}
		
	}	

	pce = bP;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= PawnTable[MIRROR64(SQ64(sq))];	
		
		if( (IsolatedMask[SQ64(sq)] & pos->pawns[BLACK]) == 0) {
			//printf("bP Iso:%s\n",PrSq(sq));
			score -= PawnIsolated;
		}
		
		if( (BlackPassedMask[SQ64(sq)] & pos->pawns[WHITE]) == 0) {
			//printf("bP Passed:%s\n",PrSq(sq));
			score -= PawnPassed[7 - RanksBrd[sq]];
		}
	}	

 for(int side=WHITE;side<=BLACK;side++) {
  int sign=side==WHITE?1:-1,offset=side==WHITE?0:6;
   int pawn=wP+offset;
   for(int i=0;i<pos->pceNum[pawn];i++) {
    int square=pos->pList[pawn][i],rank=RanksBrd[square];
    int relative=side==WHITE?rank:7-rank;
    U64 passed=side==WHITE?WhitePassedMask[SQ64(square)]:BlackPassedMask[SQ64(square)];
    if(!(passed&pos->pawns[side^1])) mg-=sign*(PawnPassed[relative]/2);
    int behind=side==WHITE?-10:10;
    int supported=pos->pieces[square+behind-1]==pawn || pos->pieces[square+behind+1]==pawn;
    int connected=pos->pieces[square-1]==pawn || pos->pieces[square+1]==pawn;
    if(supported || connected) {mg+=sign*5;eg+=sign*8;}
    /* Penalize the rear pawn on a doubled file; avoid double-counting pairs. */
    int ahead=FALSE,step=side==WHITE?10:-10;
    for(int to=square+step;pos->pieces[to]!=OFFBOARD;to+=step) if(pos->pieces[to]==pawn) {ahead=TRUE;break;}
    if(ahead) {
     mg-=sign*10;eg-=sign*15;
     if(!(passed&pos->pawns[side^1])) {mg-=sign*(PawnPassed[relative]-PawnPassed[relative]/2);eg-=sign*PawnPassed[relative];}
    }
   }
 }
 S_PAWNSCORE result={score,mg,eg};return result;
}
static unsigned PawnCacheIndex(U64 white,U64 black) {
 U64 key=white^((black<<32)|(black>>32));
 key^=key>>30;key*=0xbf58476d1ce4e5b9ULL;
 key^=key>>27;key*=0x94d049bb133111ebULL;
 key^=key>>31;
 return (unsigned)(key&(PAWN_CACHE_SIZE-1u));
}
static S_PAWNSCORE PawnScore(const S_BOARD *pos,S_PAWNCACHE *cache) {
 if(!cache) return CalculatePawnScore(pos);
 U64 white=pos->pawns[WHITE],black=pos->pawns[BLACK];
 S_PAWNENTRY *entry=&cache->entries[PawnCacheIndex(white,black)];
 /* Compare both complete bitboards: index collisions cannot change scores. */
 if(entry->valid && entry->white==white && entry->black==black) return entry->score;
 S_PAWNSCORE score=CalculatePawnScore(pos);
 entry->white=white;entry->black=black;entry->score=score;entry->valid=TRUE;
 return score;
}

int EvalPositionForSearch(const S_BOARD *pos,S_PAWNCACHE *cache,int tapered) {

	ASSERT(CheckBoard(pos));

	int pce;
	int pceNum;
	int sq;
	int score = pos->material[WHITE] - pos->material[BLACK];
 int mg=0,eg=0;
 int phase=pos->pceNum[wN]+pos->pceNum[bN]+pos->pceNum[wB]+pos->pceNum[bB]+2*(pos->pceNum[wR]+pos->pceNum[bR])+4*(pos->pceNum[wQ]+pos->pceNum[bQ]);
 if(phase>24) phase=24;

	if(!pos->pceNum[wP] && !pos->pceNum[bP] && DeadMaterial(pos) == TRUE) {
		return 0;
	}

 S_PAWNSCORE pawns=PawnScore(pos,cache);
 score+=pawns.base;
 if(tapered) {mg+=pawns.mg;eg+=pawns.eg;}

	pce = wN;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += KnightTable[SQ64(sq)];
	}	

	pce = bN;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= KnightTable[MIRROR64(SQ64(sq))];
	}			

	pce = wB;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += BishopTable[SQ64(sq)];
	}	

	pce = bB;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= BishopTable[MIRROR64(SQ64(sq))];
	}	

	pce = wR;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += RookTable[SQ64(sq)];
		
		ASSERT(FileRankValid(FilesBrd[sq]));
		
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score += RookOpenFile;
		} else if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			score += RookSemiOpenFile;
		}
	}	

	pce = bR;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= RookTable[MIRROR64(SQ64(sq))];
		ASSERT(FileRankValid(FilesBrd[sq]));
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score -= RookOpenFile;
		} else if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			score -= RookSemiOpenFile;
		}
	}	

	pce = wQ;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		ASSERT(FileRankValid(FilesBrd[sq]));
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score += QueenOpenFile;
		} else if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			score += QueenSemiOpenFile;
		}
	}	

	pce = bQ;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		ASSERT(FileRankValid(FilesBrd[sq]));
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score -= QueenOpenFile;
		} else if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			score -= QueenSemiOpenFile;
		}
	}	
 if(tapered) {
  mg += KingO[SQ64(pos->KingSq[WHITE])] - KingO[MIRROR64(SQ64(pos->KingSq[BLACK]))];
  eg += KingE[SQ64(pos->KingSq[WHITE])] - KingE[MIRROR64(SQ64(pos->KingSq[BLACK]))];
  mg += PawnShield(pos,WHITE)-PawnShield(pos,BLACK);
  if(phase<24) eg += PassedKingProximity(pos);
  for(int side=WHITE;side<=BLACK;side++) {
   int sign=side==WHITE?1:-1,offset=side==WHITE?0:6;
   for(int piece=wN;piece<=wQ;piece++) for(int i=0;i<pos->pceNum[piece+offset];i++) {
    int count=Mobility(pos,pos->pList[piece+offset][i],piece+offset,side);
    int mgWeight=piece==wN?4:piece==wB?4:piece==wR?2:1;
    int egWeight=piece==wN?3:piece==wB?4:piece==wR?3:2;
    mg+=sign*count*mgWeight;eg+=sign*count*egWeight;
   }

  }
  score+=(mg*phase+eg*(24-phase))/24;
 } else {
  int whiteSq=SQ64(pos->KingSq[WHITE]),blackSq=MIRROR64(SQ64(pos->KingSq[BLACK]));
  score+=pos->material[BLACK]<=ENDGAME_MAT?KingE[whiteSq]:KingO[whiteSq];
  score-=pos->material[WHITE]<=ENDGAME_MAT?KingE[blackSq]:KingO[blackSq];
 }

	if(pos->pceNum[wB] >= 2) score += BishopPair;
	if(pos->pceNum[bB] >= 2) score -= BishopPair;

	if(pos->side == WHITE) {
		return score;
	} else {
		return -score;
	}	
}

/* Diagnostics need no persistent cache. Search supplies its private cache. */
int EvalPositionCached(const S_BOARD *pos,S_PAWNCACHE *cache) {
 return EvalPositionForSearch(pos,cache,EngineOptions->TaperedEval);
}
int EvalPosition(const S_BOARD *pos) { return EvalPositionCached(pos,NULL); }
