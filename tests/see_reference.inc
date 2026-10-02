#include "defs.h"
#include <string.h>

/* SEE uses only a mailbox and king squares; no history/search-table copies. */
typedef struct {int pieces[120];int king[2];} ExchangeBoard;
static int Attacks(const ExchangeBoard *b,int from,int to) {
 int p=b->pieces[from],delta=to-from;
 if(p==wP) return delta==9 || delta==11;
 if(p==bP) return delta==-9 || delta==-11;
 if(IsKn(p)) {for(int i=0;i<8;i++) if(delta==KnDir[i]) return TRUE;return FALSE;}
 if(IsKi(p)) {for(int i=0;i<8;i++) if(delta==KiDir[i]) return TRUE;return FALSE;}
 for(int i=0;i<8;i++) {
  int d=KiDir[i];
  if(i<4 ? !IsRQ(p) : !IsBQ(p)) continue;
  for(int sq=from+d;b->pieces[sq]!=OFFBOARD;sq+=d) {
   if(sq==to) return TRUE;
   if(b->pieces[sq]!=EMPTY) break;
  }
 }
 return FALSE;
}
static int Attacked(const ExchangeBoard *b,int sq,int side) {
 for(int i=0;i<64;i++) {int from=SQ120(i),p=b->pieces[from];if(p!=EMPTY && PieceCol[p]==side && Attacks(b,from,sq)) return TRUE;}
 return FALSE;
}
static int Reply(ExchangeBoard *b,int to,int side,int depth) {
 if(depth>=30 || IsKi(b->pieces[to])) return 0;
 int bestFrom=NO_SQ,bestValue=100000,bestPiece=EMPTY;
 for(int i=0;i<64;i++) {
  int from=SQ120(i),p=b->pieces[from];
  if(p==EMPTY || PieceCol[p]!=side || PieceVal[p]>=bestValue || !Attacks(b,from,to)) continue;
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
