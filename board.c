// board.c

#include "stdio.h"
#include "defs.h"
#include <string.h>

#ifdef DEBUG
static int PceListOk(const S_BOARD *pos) {
	int pce = wP;
	int sq;
	int num;
	for(pce = wP; pce <= bK; ++pce) {
		if(pos->pceNum[pce]<0 || pos->pceNum[pce]>=10) return FALSE;
	}

	if(pos->pceNum[wK]!=1 || pos->pceNum[bK]!=1) return FALSE;

	for(pce = wP; pce <= bK; ++pce) {
		for(num = 0; num < pos->pceNum[pce]; ++num) {
			sq = pos->pList[pce][num];
			if(!SqOnBoard(sq)) return FALSE;
		}
	}
    return TRUE;
}

#endif

int CheckBoard(const S_BOARD *pos) {
#ifndef DEBUG
 (void)pos; return TRUE;
#else

	int t_pceNum[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int t_bigPce[2] = { 0, 0};
	int t_majPce[2] = { 0, 0};
	int t_minPce[2] = { 0, 0};
	int t_material[2] = { 0, 0};

	int sq64,t_piece,t_pce_num,sq120,colour,pcount;

	U64 t_pawns[3] = {0ULL, 0ULL, 0ULL};

	t_pawns[WHITE] = pos->pawns[WHITE];
	t_pawns[BLACK] = pos->pawns[BLACK];
	t_pawns[BOTH] = pos->pawns[BOTH];

	// check piece lists
	for(t_piece = wP; t_piece <= bK; ++t_piece) {
		for(t_pce_num = 0; t_pce_num < pos->pceNum[t_piece]; ++t_pce_num) {
			sq120 = pos->pList[t_piece][t_pce_num];
			ASSERT(pos->pieces[sq120]==t_piece);
		}
	}

	// check piece count and other counters
	for(sq64 = 0; sq64 < 64; ++sq64) {
		sq120 = SQ120(sq64);
		t_piece = pos->pieces[sq120];
		t_pceNum[t_piece]++;
		colour = PieceCol[t_piece];
		if( PieceBig[t_piece] == TRUE) t_bigPce[colour]++;
		if( PieceMin[t_piece] == TRUE) t_minPce[colour]++;
		if( PieceMaj[t_piece] == TRUE) t_majPce[colour]++;

		if(t_piece != EMPTY) t_material[colour] += PieceVal[t_piece];
	}

	for(t_piece = wP; t_piece <= bK; ++t_piece) {
		ASSERT(t_pceNum[t_piece]==pos->pceNum[t_piece]);
	}

	// check bitboards count
	pcount = CNT(t_pawns[WHITE]);
	ASSERT(pcount == pos->pceNum[wP]);
	pcount = CNT(t_pawns[BLACK]);
	ASSERT(pcount == pos->pceNum[bP]);
	pcount = CNT(t_pawns[BOTH]);
	ASSERT(pcount == (pos->pceNum[bP] + pos->pceNum[wP]));

	// check bitboards squares
	while(t_pawns[WHITE]) {
		sq64 = POP(&t_pawns[WHITE]);
		ASSERT(pos->pieces[SQ120(sq64)] == wP);
	}

	while(t_pawns[BLACK]) {
		sq64 = POP(&t_pawns[BLACK]);
		ASSERT(pos->pieces[SQ120(sq64)] == bP);
	}

	while(t_pawns[BOTH]) {
		sq64 = POP(&t_pawns[BOTH]);
		ASSERT( (pos->pieces[SQ120(sq64)] == bP) || (pos->pieces[SQ120(sq64)] == wP) );
	}

	ASSERT(t_material[WHITE]==pos->material[WHITE] && t_material[BLACK]==pos->material[BLACK]);
	ASSERT(t_minPce[WHITE]==pos->minPce[WHITE] && t_minPce[BLACK]==pos->minPce[BLACK]);
	ASSERT(t_majPce[WHITE]==pos->majPce[WHITE] && t_majPce[BLACK]==pos->majPce[BLACK]);
	ASSERT(t_bigPce[WHITE]==pos->bigPce[WHITE] && t_bigPce[BLACK]==pos->bigPce[BLACK]);

	ASSERT(pos->side==WHITE || pos->side==BLACK);
	ASSERT(GeneratePosKey(pos)==pos->posKey);

	ASSERT(pos->enPas==NO_SQ || ( RanksBrd[pos->enPas]==RANK_6 && pos->side == WHITE)
		 || ( RanksBrd[pos->enPas]==RANK_3 && pos->side == BLACK));

	ASSERT(pos->pieces[pos->KingSq[WHITE]] == wK);
	ASSERT(pos->pieces[pos->KingSq[BLACK]] == bK);

	ASSERT(pos->castlePerm >= 0 && pos->castlePerm <= 15);

	ASSERT(PceListOk(pos));

	return TRUE;
#endif
}

void UpdateListsMaterial(S_BOARD *pos) {

	int piece,sq,index,colour;

	for(index = 0; index < BRD_SQ_NUM; ++index) {
		sq = index;
		piece = pos->pieces[index];
		ASSERT(PceValidEmptyOffbrd(piece));
		if(piece!=OFFBOARD && piece!= EMPTY) {
			colour = PieceCol[piece];
			ASSERT(SideValid(colour));

		    if( PieceBig[piece] == TRUE) pos->bigPce[colour]++;
		    if( PieceMin[piece] == TRUE) pos->minPce[colour]++;
		    if( PieceMaj[piece] == TRUE) pos->majPce[colour]++;

			pos->material[colour] += PieceVal[piece];

			ASSERT(pos->pceNum[piece] < 10 && pos->pceNum[piece] >= 0);

			pos->pList[piece][pos->pceNum[piece]] = sq;
			pos->pceNum[piece]++;


			if(piece==wK) pos->KingSq[WHITE] = sq;
			if(piece==bK) pos->KingSq[BLACK] = sq;

			if(piece==wP) {
				SETBIT(pos->pawns[WHITE],SQ64(sq));
				SETBIT(pos->pawns[BOTH],SQ64(sq));
			} else if(piece==bP) {
				SETBIT(pos->pawns[BLACK],SQ64(sq));
				SETBIT(pos->pawns[BOTH],SQ64(sq));
			}
		}
	}
}

int ParseFen(char *fen, S_BOARD *out) {
 if(!fen || !out) return -1;
 S_BOARD tmp=*out, *pos=&tmp;
 ResetBoard(pos);
 const char *p=fen;
 for(int rank=7;rank>=0;rank--) {
  int file=0;
  while(file<8) {
   char c=*p++; int piece=EMPTY;
   if(c>='1' && c<='8') {file+=c-'0';if(file>8) return -1;continue;}
   const char *chars=".PNBRQKpnbrqk";
   const char *found=c?strchr(chars,c):NULL;
   if(!found || found==chars) return -1;
   piece=(int)(found-chars);
   if(PiecePawn[piece] && (rank==0 || rank==7)) return -1;
   pos->pieces[FR2SQ(file,rank)]=piece;file++;
  }
  if(*p++!=(rank?'/':' ')) return -1;
 }
 if(*p!='w' && *p!='b') return -1;
 pos->side=*p++=='w'?WHITE:BLACK;
 if(*p++!=' ') return -1;
 if(*p=='-') p++;
 else {
  int count=0;
  while(*p && *p!=' ') {
   int flag=0;
   switch(*p++) {case 'K':flag=WKCA;break;case 'Q':flag=WQCA;break;case 'k':flag=BKCA;break;case 'q':flag=BQCA;break;default:return -1;}
   if(++count>4 || (pos->castlePerm&flag)) return -1;
   pos->castlePerm|=flag;
  }
 }
 if(*p++!=' ') return -1;
 if(*p=='-') p++;
 else {
  if(p[0]<'a' || p[0]>'h' || (p[1]!='3' && p[1]!='6')) return -1;
  pos->enPas=FR2SQ(p[0]-'a',p[1]-'1');p+=2;
  if(RanksBrd[pos->enPas]!=(pos->side==WHITE?RANK_6:RANK_3) || pos->pieces[pos->enPas]!=EMPTY || pos->pieces[pos->enPas+(pos->side==WHITE?-10:10)]!=(pos->side==WHITE?bP:wP)) return -1;
 }
 if(*p && *p!=' ' && *p!='\n' && *p!='\r') return -1;
 while(*p==' ') p++;
 if(*p>='0' && *p<='9') {
  char *end;long clock=strtol(p,&end,10);
  if(clock<0 || clock>1000000) return -1;
  pos->fiftyMove=(int)clock;p=end;
  if(*p && *p!=' ' && *p!='\n' && *p!='\r') return -1;
 }
 int counts[13]={0};
 for(int i=0;i<64;i++) {int piece=pos->pieces[SQ120(i)];if(piece && ++counts[piece]>9) return -1;}
 int white=0,black=0;for(int i=wP;i<=wK;i++) white+=counts[i];for(int i=bP;i<=bK;i++) black+=counts[i];
 if(white>16 || black>16) return -1;
 if(counts[wK]!=1 || counts[bK]!=1 || counts[wP]>8 || counts[bP]>8) return -1;
 if((pos->castlePerm&WKCA) && (pos->pieces[E1]!=wK || pos->pieces[H1]!=wR)) return -1;
 if((pos->castlePerm&WQCA) && (pos->pieces[E1]!=wK || pos->pieces[A1]!=wR)) return -1;
 if((pos->castlePerm&BKCA) && (pos->pieces[E8]!=bK || pos->pieces[H8]!=bR)) return -1;
 if((pos->castlePerm&BQCA) && (pos->pieces[E8]!=bK || pos->pieces[A8]!=bR)) return -1;
 pos->posKey=GeneratePosKey(pos);UpdateListsMaterial(pos);
 if(SqAttacked(pos->KingSq[pos->side^1],pos->side,pos)) return -1;
 memset(pos->history,0,sizeof pos->history);memset(pos->PvArray,0,sizeof pos->PvArray);
 *out=*pos;return 0;
}

void ResetBoard(S_BOARD *pos) {

	int index = 0;

	for(index = 0; index < BRD_SQ_NUM; ++index) {
		pos->pieces[index] = OFFBOARD;
	}

	for(index = 0; index < 64; ++index) {
		pos->pieces[SQ120(index)] = EMPTY;
	}

	for(index = 0; index < 2; ++index) {
		pos->bigPce[index] = 0;
		pos->majPce[index] = 0;
		pos->minPce[index] = 0;
		pos->material[index] = 0;
	}

	for(index = 0; index < 3; ++index) {
		pos->pawns[index] = 0ULL;
	}

	for(index = 0; index < 13; ++index) {
		pos->pceNum[index] = 0;
	}

	pos->KingSq[WHITE] = pos->KingSq[BLACK] = NO_SQ;

	pos->side = BOTH;
	pos->enPas = NO_SQ;
	pos->fiftyMove = 0;

	pos->ply = 0;
	pos->hisPly = 0;

	pos->castlePerm = 0;

	pos->posKey = 0ULL;

}
void PrintBoard(const S_BOARD *pos) {

	int sq,file,rank,piece;

	printf("\nGame Board:\n\n");

	for(rank = RANK_8; rank >= RANK_1; rank--) {
		printf("%d  ",rank+1);
		for(file = FILE_A; file <= FILE_H; file++) {
			sq = FR2SQ(file,rank);
			piece = pos->pieces[sq];
			printf("%3c",PceChar[piece]);
		}
		printf("\n");
	}

	printf("\n   ");
	for(file = FILE_A; file <= FILE_H; file++) {
		printf("%3c",'a'+file);
	}
	printf("\n");
	printf("side:%c\n",SideChar[pos->side]);
	printf("enPas:%d\n",pos->enPas);
	printf("castle:%c%c%c%c\n",
			pos->castlePerm & WKCA ? 'K' : '-',
			pos->castlePerm & WQCA ? 'Q' : '-',
			pos->castlePerm & BKCA ? 'k' : '-',
			pos->castlePerm & BQCA ? 'q' : '-'
			);
	printf("PosKey:%llX\n",pos->posKey);
}

void MirrorBoard(S_BOARD *pos) {

    int tempPiecesArray[64];
    int tempSide = pos->side^1;
	int SwapPiece[13] = { EMPTY, bP, bN, bB, bR, bQ, bK, wP, wN, wB, wR, wQ, wK };
    int tempCastlePerm = 0;
    int tempEnPas = NO_SQ;

	int sq;
	int tp;

    if (pos->castlePerm & WKCA) tempCastlePerm |= BKCA;
    if (pos->castlePerm & WQCA) tempCastlePerm |= BQCA;

    if (pos->castlePerm & BKCA) tempCastlePerm |= WKCA;
    if (pos->castlePerm & BQCA) tempCastlePerm |= WQCA;

	if (pos->enPas != NO_SQ)  {
        tempEnPas = SQ120(Mirror64[SQ64(pos->enPas)]);
    }

    for (sq = 0; sq < 64; sq++) {
        tempPiecesArray[sq] = pos->pieces[SQ120(Mirror64[sq])];
    }

    ResetBoard(pos);

	for (sq = 0; sq < 64; sq++) {
        tp = SwapPiece[tempPiecesArray[sq]];
        pos->pieces[SQ120(sq)] = tp;
    }

	pos->side = tempSide;
    pos->castlePerm = tempCastlePerm;
    pos->enPas = tempEnPas;

    pos->posKey = GeneratePosKey(pos);

	UpdateListsMaterial(pos);

    ASSERT(CheckBoard(pos));
}