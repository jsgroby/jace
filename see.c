#include "defs.h"
#include <string.h>

/* SEE uses only a mailbox and king squares; no history/search-table copies. */
typedef struct {int pieces[120];int king[2];} ExchangeBoard;
/* Enumerate from the exchange square, then visit ascending board squares.
   The order preserves the existing choice between equal-value attackers. */
static U64 Attackers(const ExchangeBoard *b,int to,int side) {
 U64 attackers=0;
 int pawn=side==WHITE?wP:bP,back=side==WHITE?-10:10;
 for(int offset=-1;offset<=1;offset+=2) {
  int from=to+back+offset;
  if(b->pieces[from]==pawn) SETBIT(attackers,Sq120ToSq64[from]);
 }
 int knight=side==WHITE?wN:bN,king=side==WHITE?wK:bK;
 for(int i=0;i<8;i++) {
  int from=to+KnDir[i];
  if(b->pieces[from]==knight) SETBIT(attackers,Sq120ToSq64[from]);
  from=to+KiDir[i];
  if(b->pieces[from]==king) SETBIT(attackers,Sq120ToSq64[from]);
  for(from=to+KiDir[i];b->pieces[from]!=OFFBOARD;from+=KiDir[i]) {
   int p=b->pieces[from];
   if(p==EMPTY) continue;
   if(PieceCol[p]==side && (i<4?IsRQ(p):IsBQ(p)))
    SETBIT(attackers,Sq120ToSq64[from]);
   break;
  }
 }
 return attackers;
}
static int Attacked(const ExchangeBoard *b,int sq,int side) {
 /* Query attacks from the target square; preserve pseudo-attack semantics
    (including pinned defenders) used to validate exchange king safety. */
 int pawn=side==WHITE?wP:bP,back=side==WHITE?-10:10;
 if(b->pieces[sq+back-1]==pawn || b->pieces[sq+back+1]==pawn) return TRUE;
 int knight=side==WHITE?wN:bN,king=side==WHITE?wK:bK;
 for(int i=0;i<8;i++) {
  if(b->pieces[sq+KnDir[i]]==knight) return TRUE;
  if(b->pieces[sq+KiDir[i]]==king) return TRUE;
 }
 for(int i=0;i<8;i++) {
  int d=KiDir[i];
  for(int from=sq+d;b->pieces[from]!=OFFBOARD;from+=d) {
   int p=b->pieces[from];
   if(p==EMPTY) continue;
   if(PieceCol[p]==side && (i<4?IsRQ(p):IsBQ(p))) return TRUE;
   break;
  }
 }
 return FALSE;
}
static int Reply(ExchangeBoard *b,int to,int side,int depth) {
 if(depth>=30 || IsKi(b->pieces[to])) return 0;
 int bestFrom=NO_SQ,bestValue=100000,bestPiece=EMPTY;
 U64 attackers=Attackers(b,to,side);
 while(attackers) {
  int from=SQ120(POP(&attackers)),p=b->pieces[from];
  if(PieceVal[p]>=bestValue) continue;
  int victim=b->pieces[to],king=b->king[side];
  int placed=PiecePawn[p] && (RanksBrd[to]==RANK_1 || RanksBrd[to]==RANK_8) ? (side==WHITE?wQ:bQ):p;
  b->pieces[from]=EMPTY;b->pieces[to]=placed;if(IsKi(p)) b->king[side]=to;
  int legal=!Attacked(b,b->king[side],side^1);
  b->pieces[from]=p;b->pieces[to]=victim;b->king[side]=king;
  if(legal) {bestFrom=from;bestValue=PieceVal[p];bestPiece=placed;}
 }
 if(bestFrom==NO_SQ) return 0;
 int victim=b->pieces[to],p=b->pieces[bestFrom],king=b->king[side];
 int gain=PieceVal[victim]+PieceVal[bestPiece]-PieceVal[p];
 b->pieces[bestFrom]=EMPTY;b->pieces[to]=bestPiece;if(IsKi(p)) b->king[side]=to;
 gain-=Reply(b,to,side^1,depth+1);
 b->pieces[bestFrom]=p;b->pieces[to]=victim;b->king[side]=king;
 return gain>0?gain:0; /* Each recapturing side may decline the exchange. */
}
int SEE(const S_BOARD *pos,int move) {
 int from=FROMSQ(move),to=TOSQ(move),p=pos->pieces[from];
 int cap=(move&MFLAGEP)?(pos->side==WHITE?bP:wP):CAPTURED(move);
 int prom=PROMOTED(move);
 if(cap==EMPTY && !prom) return 0;
 ExchangeBoard b;memcpy(b.pieces,pos->pieces,sizeof b.pieces);memcpy(b.king,pos->KingSq,sizeof b.king);
 b.pieces[from]=EMPTY;b.pieces[to]=prom?prom:p;
 if(move&MFLAGEP) b.pieces[to+(pos->side==WHITE?-10:10)]=EMPTY;
 if(IsKi(p)) b.king[pos->side]=to;
 if(Attacked(&b,b.king[pos->side],pos->side^1)) return 0; /* Legality is enforced by MakeMove. */
 int gain=PieceVal[cap]+(prom?PieceVal[prom]-PieceVal[p]:0);
 return gain-Reply(&b,to,pos->side^1,0);
}
