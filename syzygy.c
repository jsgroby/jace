#include "defs.h"
#include <string.h>
#include "vendor/fathom/tbprobe.h"

/* This adapter owns Fathom. JACE is single-threaded; probe/init/free must remain
   serialized if SMP is added. Direct probe returns bypass TT insertion; parent search bounds use normal TT rules. */
static unsigned largest;
unsigned SyzygyInit(const char *path) {
 largest=0;
 if(tb_init(!strcmp(path,"<empty>")?"":path)) largest=TB_LARGEST;
 return largest;
}
void SyzygyFree(void) {tb_free();largest=0;}
static int Eligible(const S_BOARD *p) {
 if(!largest || !EngineOptions->SyzygyProbeLimit || p->castlePerm || p->fiftyMove>=100) return FALSE;
 int count=0;for(int piece=wP;piece<=bK;piece++)count+=p->pceNum[piece];
 return count<=EngineOptions->SyzygyProbeLimit && (unsigned)count<=largest;
}
typedef struct {uint64_t white,black,kings,queens,rooks,bishops,knights,pawns;unsigned ep;} TB_POSITION;
static TB_POSITION Convert(const S_BOARD *p) {
 TB_POSITION t={0};
 for(int piece=wP;piece<=bK;piece++)for(int i=0;i<p->pceNum[piece];i++) {
  uint64_t bit=UINT64_C(1)<<(unsigned)SQ64(p->pList[piece][i]);
  if(PieceCol[piece]==WHITE)t.white|=bit;else t.black|=bit;
  switch(piece) {
   case wP:case bP:t.pawns|=bit;break;case wN:case bN:t.knights|=bit;break;
   case wB:case bB:t.bishops|=bit;break;case wR:case bR:t.rooks|=bit;break;
   case wQ:case bQ:t.queens|=bit;break;case wK:case bK:t.kings|=bit;break;
  }
 }
 t.ep=p->enPas==NO_SQ?0u:(unsigned)SQ64(p->enPas);return t;
}
#define TB_ARGS(t) (t).white,(t).black,(t).kings,(t).queens,(t).rooks,(t).bishops,(t).knights,(t).pawns
int SyzygyWdl(const S_BOARD *p,int *score) {
 /* Fathom WDL has no rule50 context. Only exact reset positions are eligible;
    never turn a cursed win or blessed loss into a decisive search score. */
 if(!Eligible(p) || p->fiftyMove) return FALSE;
 TB_POSITION t=Convert(p);
 unsigned wdl=tb_probe_wdl(TB_ARGS(t),0,0,t.ep,p->side==WHITE);
 if(wdl==TB_RESULT_FAILED)return FALSE;
 *score=wdl==TB_WIN?20000:wdl==TB_LOSS?-20000:0;return TRUE;
}
int SyzygyRoot(S_BOARD *p,S_TBROOT *out) {
 out->count=0;if(!Eligible(p))return FALSE;
 TB_POSITION t=Convert(p);struct TbRootMoves roots;
 /* Conservative repetition flag ranks wins by progress toward a zeroing move,
    even when a winning line has already appeared in the real game history. */
 if(!tb_probe_root_dtz(TB_ARGS(t),(unsigned)p->fiftyMove,0,t.ep,p->side==WHITE,true,true,&roots))return FALSE;
 S_MOVELIST list;GenerateAllMoves(p,&list);
 for(unsigned i=0;i<roots.size;i++) {
  unsigned from=TB_MOVE_FROM(roots.moves[i].move),to=TB_MOVE_TO(roots.moves[i].move),promo=TB_MOVE_PROMOTES(roots.moves[i].move);
  static const int promotion[5]={EMPTY,wQ,wR,wB,wN};
  if(promo>4)return FALSE;
  int promoted=promotion[promo];if(promoted && p->side==BLACK)promoted+=6;
  int move=NOMOVE,claim=FALSE;
  for(int j=0;j<list.count;j++) {
   int m=list.moves[j].move;
   if((unsigned)SQ64(FROMSQ(m))==from && (unsigned)SQ64(TOSQ(m))==to && PROMOTED(m)==promoted && MakeMove(p,m)) {claim=PvDrawReached(p) && (HasLegalMove(p) || !SqAttacked(p->KingSq[p->side],p->side^1,p));TakeMove(p);move=m;break;}
  }
  if(move==NOMOVE || out->count>=MAXPOSITIONMOVES){out->count=0;return FALSE;}
  int n=out->count++;out->moves[n]=move;out->ranks[n]=claim?0:roots.moves[i].tbRank;
  out->scores[n]=claim?0:roots.moves[i].tbRank>=900?20000:roots.moves[i].tbRank<=-900?-20000:0;
 }
 return out->count>0;
}
