#include "defs.h"
#include <assert.h>
#include <stdio.h>
int ReferenceDeadMaterial(const S_BOARD *pos);
int ReferenceEvalPosition(const S_BOARD *pos);
#include "evaluate_reference.inc"
#define PARAM(name, value, low, high) extern int name;
#include "eval_params.def"
#undef PARAM
static unsigned state=20261122;
static unsigned Next(void) {state=state*1664525u+1013904223u;return state;}
static void SetWeights(int mode) {
#define PARAM(name, value, low, high) name=Reference##name=value;
#include "eval_params.def"
#undef PARAM
 KingKnightAttack=ReferenceKingKnightAttack=0;
 KingBishopAttack=ReferenceKingBishopAttack=0;
 KingRookAttack=ReferenceKingRookAttack=0;
 KingQueenAttack=ReferenceKingQueenAttack=0;
 KingCoordination=ReferenceKingCoordination=0;
 if(mode) {
  KingKnightAttack=ReferenceKingKnightAttack=3;
  KingBishopAttack=ReferenceKingBishopAttack=5;
  KingRookAttack=ReferenceKingRookAttack=7;
  KingQueenAttack=ReferenceKingQueenAttack=11;
  KingCoordination=ReferenceKingCoordination=13;
 }
}
static void Check(S_BOARD *p) {
 for(int mode=0;mode<2;mode++) {
  SetWeights(mode);
  for(int tapered=0;tapered<2;tapered++) {
   EngineOptions->TaperedEval=tapered;
   int score=EvalPosition(p);assert(score==ReferenceEvalPosition(p));
   S_BOARD mirror=*p;MirrorBoard(&mirror);assert(EvalPosition(&mirror)==score);
  }
 }
 EngineOptions->TaperedEval=TRUE;
}
int main(void) {
 AllInit();S_BOARD p={0};int checked=0;
 char *fixtures[]={
  "6k1/5ppp/8/4N2Q/2B5/8/5PPP/6K1 w - - 0 1",
  "6k1/5ppp/8/3pN2Q/2B5/8/5PPP/6K1 w - - 0 1",
  "6k1/5ppp/8/4N3/2B5/8/5PPP/6K1 w - - 0 1",
  "6k1/5ppp/8/7Q/8/8/5PPP/6K1 w - - 0 1",
  "k7/8/8/8/8/8/7P/7K w - - 0 1",
  "6k1/5ppp/4NN2/4N2Q/2B5/8/5PPP/6K1 b - - 0 1"};
 for(size_t i=0;i<sizeof fixtures/sizeof fixtures[0];i++) {assert(ParseFen(fixtures[i],&p)==0);Check(&p);checked++;}
 for(int game=0;game<40;game++) {
  assert(ParseFen(START_FEN,&p)==0);
  for(int ply=0;ply<120;ply++) {
   Check(&p);checked++;
   S_MOVELIST list;GenerateAllMoves(&p,&list);int found=FALSE;
   unsigned start=Next();
   for(int i=0;i<list.count;i++) {
    int move=list.moves[(start+(unsigned)i)%(unsigned)list.count].move;
    if(MakeMove(&p,move)) {found=TRUE;p.ply=0;break;}
   }
   if(!found) break;
  }
 }
 printf("PASS: king pressure oracle, blockers, queenless, coordination cap, mirrors and legacy mode across %d positions\n",checked);
 return 0;
}
