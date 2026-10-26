#include "defs.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static void TestInput(S_SEARCHINFO *info) {(void)info;}
#define ReadInput TestInput
#include "search.c"
#include "qsearch_reference.inc"
#include "evaluate_reference.inc"
#undef ReadInput
/* Keep the prior SEE implementation as a differential oracle. */
#define ExchangeBoard ReferenceExchangeBoard
#define Attacks ReferenceAttacks
#define Attacked ReferenceAttacked
#define Reply ReferenceReply
#define SEE ReferenceSEE
#include "see_reference.inc"
#undef ExchangeBoard
#undef Attacks
#undef Attacked
#undef Reply
#undef SEE
#define SEE IncludedSEE
#include "see.c"
#undef SEE
#define LoopSlidePce ReferenceLoopSlidePce
#define LoopNonSlidePce ReferenceLoopNonSlidePce
#define LoopSlideIndex ReferenceLoopSlideIndex
#define LoopNonSlideIndex ReferenceLoopNonSlideIndex
#define PceDir ReferencePceDir
#define NumDir ReferenceNumDir
#define VictimScore ReferenceVictimScore
#define InitMvvLva ReferenceInitMvvLva
#define MoveExists ReferenceMoveExists
#define GenerateAllMoves ReferenceGenerateAllMoves
#define GenerateAllCaps ReferenceGenerateAllCaps
#define HasLegalMove ReferenceHasLegalMove
void ReferenceGenerateAllMoves(const S_BOARD *pos,S_MOVELIST *list);
#include "movegen_reference.inc"
#undef LoopSlidePce
#undef LoopNonSlidePce
#undef LoopSlideIndex
#undef LoopNonSlideIndex
#undef PceDir
#undef NumDir
#undef VictimScore
#undef InitMvvLva
#undef MoveExists
#undef GenerateAllMoves
#undef GenerateAllCaps
#undef HasLegalMove
#undef MOVE
#undef SQOFFBOARD
static unsigned long legalComparisons;
static void CompareLegalProbe(S_BOARD *pos) {
 S_MOVELIST a,b;
 ReferenceGenerateAllMoves(pos,&a);GenerateAllMoves(pos,&b);
 assert(a.count==b.count);
 for(int i=0;i<a.count;i++) {
  assert(a.moves[i].move==b.moves[i].move);
  assert(a.moves[i].score==b.moves[i].score);
 }
 S_BOARD original=*pos,reference=*pos,candidate=*pos;
 int expected=ReferenceHasLegalMove(&reference);
 assert(HasLegalMove(&candidate)==expected);
 /* Include piece-list order and overwritten undo slots, not just the board key. */
 assert(memcmp(&reference,&candidate,sizeof reference)==0);
 assert(candidate.posKey==original.posKey && CheckBoard(&candidate));
 assert(memcmp(candidate.pieces,original.pieces,sizeof candidate.pieces)==0);
 legalComparisons++;
}
static void TestLegalProbe(void) {
 const char *positions[]={
  START_FEN,
  "7k/6Q1/5K2/8/8/8/8/8 b - - 0 1",
  "7k/5K2/6Q1/8/8/8/8/8 b - - 0 1",
  "k3r3/8/8/3pP3/8/8/8/4K3 w - d6 0 1",
  "7k/8/8/3pP3/8/8/K7/8 w - d6 0 1",
  "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
  "7r/1P3P1k/8/8/8/8/8/7K w - - 0 1",
  "r6k/1P6/8/8/8/8/8/7K w - - 0 1",
  "7k/P7/8/8/8/8/8/7K w - - 0 1",
#include "qsearch_positions.inc"
 };
 for(size_t i=0;i<sizeof positions/sizeof positions[0];i++) {
  S_BOARD pos={0};assert(ParseFen((char*)positions[i],&pos)==0);
  CompareLegalProbe(&pos);MirrorBoard(&pos);CompareLegalProbe(&pos);
 }
}
static unsigned long seeComparisons,attackComparisons;
static void CompareSee(S_BOARD *p) {
 ExchangeBoard b;ReferenceExchangeBoard ref;
 memcpy(b.pieces,p->pieces,sizeof b.pieces);memcpy(b.king,p->KingSq,sizeof b.king);
 memcpy(ref.pieces,p->pieces,sizeof ref.pieces);memcpy(ref.king,p->KingSq,sizeof ref.king);
 for(int sq=0;sq<64;sq++) for(int side=0;side<2;side++) {
  assert(Attacked(&b,SQ120(sq),side)==ReferenceAttacked(&ref,SQ120(sq),side));attackComparisons++;
 }
 S_MOVELIST list;GenerateAllCaps(p,&list);
 for(int i=0;i<list.count;i++) {
  int move=list.moves[i].move;
  assert(SEE(p,move)==ReferenceSEE(p,move));seeComparisons++;
 }
}
static U64 perft(S_BOARD *p,int depth) {
 if(!depth) return 1;
 S_MOVELIST list;GenerateAllMoves(p,&list);U64 total=0;
 for(int i=0;i<list.count;i++) {
  int pieces[120];memcpy(pieces,p->pieces,sizeof pieces);
  U64 key=p->posKey;int fifty=p->fiftyMove,ep=p->enPas,castle=p->castlePerm,side=p->side;
  if(MakeMove(p,list.moves[i].move)) {assert(CheckBoard(p));assert(p->posKey==GeneratePosKey(p));total+=perft(p,depth-1);TakeMove(p);}
  assert(key==p->posKey && key==GeneratePosKey(p));assert(memcmp(pieces,p->pieces,sizeof pieces)==0);
  assert(fifty==p->fiftyMove && ep==p->enPas && castle==p->castlePerm && side==p->side);
 }
 return total;
}
static void fen(S_BOARD *p,const char *s) {assert(ParseFen((char*)s,p)==0);assert(CheckBoard(p));}
static void see(S_BOARD *p,const char *f,char *m,int expected) {
 fen(p,f);int move=ParseMove(m,p);assert(move!=NOMOVE);int actual=SEE(p,move);assert(actual==ReferenceSEE(p,move));CompareSee(p);
 printf("SEE %s = %d expected %d\n",m,actual,expected);assert(actual==expected);
}
static void TestQSearchEquivalence(void) {
 const char *positions[]={
#include "qsearch_positions.inc"
 };
 for(size_t i=0;i<sizeof positions/sizeof positions[0];i++) {
  for(int window=0;window<5;window++) {
   S_BOARD reference={0},candidate={0};S_SEARCHINFO ri={0},ci={0};
   fen(&reference,positions[i]);CompareSee(&reference);candidate=reference;
   int alpha=window==0?-INFINITE:(window-3)*200;
   int beta=window==0?INFINITE:alpha+1;
   int expected=ReferenceQuiescence(alpha,beta,&reference,&ri);
   int actual=Quiescence(alpha,beta,&candidate,&ci);
   assert(expected==actual);
   assert(candidate.posKey==reference.posKey && candidate.posKey==GeneratePosKey(&candidate));
   assert(memcmp(candidate.pieces,reference.pieces,sizeof candidate.pieces)==0);
   assert(candidate.ply==0 && candidate.hisPly==0 && CheckBoard(&candidate));
  }
 }
 puts("QUIESCENCE DIFFERENTIAL TESTS PASS (340 position/window cases)");
}
static void TestHashFull(void) {
 S_HASHENTRY entries[4]={0};S_HASHTABLE table={0};
 assert(HashFull(&table)==0);
 table.pTable=entries;table.numEntries=4;table.generation=7;
 assert(HashFull(&table)==0);
 entries[0].depth=1;entries[0].generation=7;assert(HashFull(&table)==250);
 entries[1].depth=1;entries[1].generation=6;assert(HashFull(&table)==250);
 for(int i=0;i<4;i++){entries[i].depth=1;entries[i].generation=7;}
 assert(HashFull(&table)==1000);
 table.generation=8;assert(HashFull(&table)==0);
 puts("HASH REPORTING TESTS PASS");
}
static void TestRestrictedRoot(void) {
 S_BOARD p={0};S_SEARCHINFO info={0};fen(&p,START_FEN);InitHashTable(p.HashTable,1);
 ClearForSearch(&p,&info);info.restricted=TRUE;info.searchMoveCount=1;
 int move=ParseMove("a2a3",&p);info.searchMoves[0]=move;
 (void)AlphaBeta(-INFINITE,INFINITE,1,&p,&info,TRUE);
 assert(rootBestMove==move && ProbePvMove(&p)==NOMOVE);
 /* No restricted root score may be reused as an unrestricted bound. */
 int hm=NOMOVE,score=0;
 assert(!ProbeHashEntry(&p,&hm,&score,-INFINITE,INFINITE,1));
 ClearForSearch(&p,&info);info.restricted=FALSE;info.searchMoveCount=0;
 EngineOptions->MultiPV=3;info.excludedCount=1;info.excludedMoves[0]=move;
 (void)AlphaBeta(-INFINITE,INFINITE,1,&p,&info,TRUE);
 assert(rootBestMove!=move && rootBestMove!=NOMOVE && ProbePvMove(&p)==NOMOVE);
 EngineOptions->MultiPV=1;free(p.HashTable->pTable);
 puts("RESTRICTED ROOT TT TESTS PASS");
}
static void TestPassedKings(void) {
 const struct {const char *fen;int delta;} cases[]={
  {"7k/8/8/4P3/4K3/8/8/8 w - - 0 1",4},
  {"7k/8/8/3KP3/8/8/8/8 w - - 0 1",8},
  {"8/4k3/8/4P3/4K3/8/8/8 w - - 0 1",-4},
  {"8/8/4k3/4P3/4K3/8/8/8 w - - 0 1",-8},
  {"7k/8/8/4P3/4P3/4K3/8/8 w - - 0 1",0},
  {"7k/8/8/8/8/4K3/4P3/8 w - - 0 1",0},
  {"7k/8/4p3/4P3/4K3/8/8/8 w - - 0 1",0},
  {"7k/P7/1K6/8/8/8/8/8 w - - 0 1",40},
  {"7k/8/8/8/8/8/8/K7 w - - 0 1",0}
 };
 for(size_t i=0;i<sizeof cases/sizeof cases[0];i++) {
  S_BOARD p={0};fen(&p,cases[i].fen);
  EngineOptions->TaperedEval=TRUE;referenceKingProximity=FALSE;
  int old=ReferenceEvalPosition(&p);referenceKingProximity=TRUE;
  assert(EvalPosition(&p)-old==cases[i].delta);
  assert(EvalPosition(&p)==ReferenceEvalPosition(&p));
  S_BOARD mirror=p;MirrorBoard(&mirror);assert(EvalPosition(&mirror)==EvalPosition(&p));
  EngineOptions->TaperedEval=FALSE;referenceKingProximity=FALSE;
  assert(EvalPosition(&p)==ReferenceEvalPosition(&p));referenceKingProximity=TRUE;
 }
 /* A king move must change this term even on a pawn-cache hit, then undo. */
 S_BOARD p={0};fen(&p,cases[0].fen);EngineOptions->TaperedEval=TRUE;
 int before=EvalPosition(&p);U64 pawns=p.pawns[BOTH];
 int move=ParseMove("e4d5",&p);assert(MakeMove(&p,move));
 assert(p.pawns[BOTH]==pawns);assert(EvalPosition(&p)==ReferenceEvalPosition(&p));
 referenceKingProximity=FALSE;assert(EvalPosition(&p)-ReferenceEvalPosition(&p)==-8);
 referenceKingProximity=TRUE;TakeMove(&p);assert(EvalPosition(&p)==before);
 /* Full middlegame material suppresses the endgame-only adjustment. */
 fen(&p,"rnbqkbnr/pp3ppp/8/3P4/8/8/PPP1PPPP/RNBQKBNR w KQkq - 0 1");
 assert(ReferencePassedKings(&p)!=0);
 referenceKingProximity=FALSE;assert(EvalPosition(&p)==ReferenceEvalPosition(&p));referenceKingProximity=TRUE;
 puts("PASSED PAWN KING PROXIMITY TESTS PASS");
}
static void TestPawnEvaluation(void) {
 /* Different structures mapping to cache slot 522 at the current table size. */
 S_BOARD collisionA={0},collisionB={0};
 fen(&collisionA,"7k/8/1p6/8/8/8/P7/K7 w - - 0 1");
 fen(&collisionB,"7k/8/p7/8/8/8/1P6/K7 w - - 0 1");
 for(int mode=0;mode<2;mode++) {
  EngineOptions->TaperedEval=mode;
  assert(ReferenceEvalPosition(&collisionA)!=ReferenceEvalPosition(&collisionB));
  for(int repeat=0;repeat<3;repeat++) {
   assert(EvalPosition(&collisionA)==ReferenceEvalPosition(&collisionA));
   assert(EvalPosition(&collisionB)==ReferenceEvalPosition(&collisionB));
  }
 }

 const char *positions[]={START_FEN,
  "7k/8/8/3pP3/8/8/K7/3r4 w - d6 0 1",
  "1r5k/P7/8/8/8/8/8/K7 w - - 0 1",
  "7k/8/8/8/8/8/8/KR6 w - - 0 1",
  "7k/8/8/8/8/8/PP6/KR6 w - - 0 1",
  "6k1/8/8/8/8/8/PP6/1K1Q4 b - - 0 1"};
 for(size_t k=0;k<sizeof positions/sizeof positions[0];k++) {
  S_BOARD p={0};fen(&p,positions[k]);S_MOVELIST list;GenerateAllMoves(&p,&list);
  for(int mode=0;mode<2;mode++) {
   EngineOptions->TaperedEval=mode;
   int expected=ReferenceEvalPosition(&p);
   assert(EvalPosition(&p)==expected);assert(EvalPosition(&p)==expected);
   for(int i=0;i<list.count;i++) if(MakeMove(&p,list.moves[i].move)) {
    assert(EvalPosition(&p)==ReferenceEvalPosition(&p));
    TakeMove(&p);assert(EvalPosition(&p)==expected);
   }
   S_BOARD mirror=p;MirrorBoard(&mirror);
   assert(EvalPosition(&mirror)==ReferenceEvalPosition(&mirror));
  }
 }
 EngineOptions->TaperedEval=TRUE;puts("PAWN EVALUATION DIFFERENTIAL TESTS PASS");
}
static void TestDeferredOrdering(void) {
 const char *positions[]={START_FEN,
  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
  "7k/8/8/3pP3/8/8/K7/3r4 w - d6 0 1",
  "1r5k/P7/8/8/8/8/8/K7 w - - 0 1"};
 for(size_t k=0;k<sizeof positions/sizeof positions[0];k++) {
  S_BOARD p={0};fen(&p,positions[k]);S_MOVELIST generated;
  GenerateAllMoves(&p,&generated);
  for(int h=-1;h<generated.count;h++) {
   int hash=h<0?0x7fffffff:generated.moves[h].move;
   S_MOVELIST eager=generated,lazy=generated;
   assert(!OrderMoves(&p,&eager,NOMOVE));
   if(h>=0) {eager.moves[h].score=2000000;eager.moves[h].see=INT_MAX;}
   int deferred=OrderMoves(&p,&lazy,hash);
   assert(deferred==(h>=0));
   if(deferred) for(int i=0;i<lazy.count;i++) assert(lazy.moves[i].see==INT_MAX);
   for(int i=0;i<generated.count;i++) {
    if(i==1 && deferred) ScoreRemainingCaptures(&p,&lazy);
    PickNextMove(i,&eager);PickNextMove(i,&lazy);
    assert(eager.moves[i].move==lazy.moves[i].move);
    assert(eager.moves[i].score==lazy.moves[i].score);
    assert(eager.moves[i].see==lazy.moves[i].see);
   }
  }
 }
 puts("DEFERRED ORDERING TESTS PASS");
}
static void TestSeeCache(void) {
 const char *positions[]={
  "7k/8/4p3/3p4/8/8/8/K2Q4 w - - 0 1",
  "7k/8/8/3pP3/8/8/K7/3r4 w - d6 0 1",
  "1r5k/P7/8/8/8/8/8/K7 w - - 0 1"
 };
 for(size_t k=0;k<sizeof positions/sizeof positions[0];k++) {
  S_BOARD p={0};fen(&p,positions[k]);S_MOVELIST list;GenerateAllCaps(&p,&list);
  for(int useHash=0;useHash<2;useHash++) {
   GenerateAllCaps(&p,&list);
   int hash=useHash?list.moves[0].move:NOMOVE;
   int deferred=OrderMoves(&p,&list,hash);
   for(int i=0;i<list.count;i++) {
    if(i==1 && deferred) ScoreRemainingCaptures(&p,&list);
    PickNextMove(i,&list);int move=list.moves[i].move;
    if(move==hash || PROMOTED(move)) assert(list.moves[i].see==INT_MAX);
    else assert(list.moves[i].see==SEE(&p,move));
   }
  }
 }
 /* A losing hash capture bypasses ordering SEE and still needs pruning SEE. */
 S_BOARD p={0};fen(&p,positions[0]);InitHashTable(p.HashTable,1);
 int move=ParseMove("d1d5",&p);assert(SEE(&p,move)<0);
 StoreHashEntry(&p,move,0,HFEXACT,1);
 S_SEARCHINFO info={0};int expected=ReferenceQuiescence(-INFINITE,INFINITE,&p,&info);
 memset(&info,0,sizeof info);assert(Quiescence(-INFINITE,INFINITE,&p,&info)==expected);
 assert(CheckBoard(&p));free(p.HashTable->pTable);puts("SEE CACHE TESTS PASS");
}
static void TestPvDrawTermination(void) {
 S_BOARD p={0};InitHashTable(p.HashTable,1);
 const char *cycle[]={"g1f3","g8f6","f3g1","f6g8"};
 fen(&p,START_FEN);
 for(int i=0;i<4;i++) {
  int move=ParseMove((char*)cycle[i],&p);
  StoreHashEntry(&p,move,0,HFEXACT,12);assert(MakeMove(&p,move));
 }
 assert(!PvDrawReached(&p)); /* Two occurrences are not a threefold claim. */
 while(p.ply) TakeMove(&p);
 U64 key=p.posKey;int pv[MAXDEPTH];int first=ParseMove("g1f3",&p);
 assert(GetPvLine(12,&p)==8);assert(p.ply==0 && p.hisPly==0 && p.posKey==key);
 assert(RootPV(&p,first,12,pv)==8);assert(p.hisPly==0 && p.posKey==key);
 for(int i=0;i<4;i++) assert(MakeMove(&p,ParseMove((char*)cycle[i],&p)));
 /* Extraction at a nonzero ply must restore the caller, not undo its history. */
 assert(GetPvLine(12,&p)==4);assert(p.ply==4 && p.hisPly==4 && p.posKey==key);
 assert(RootPV(&p,first,12,pv)==4);assert(p.ply==4 && p.hisPly==4);
 MakeNullMove(&p);assert(!PvDrawReached(&p));TakeNullMove(&p);
 fen(&p,"7k/8/8/8/8/8/8/KR6 w - - 99 50");ClearHashTable(p.HashTable);
 first=ParseMove("b1b2",&p);StoreHashEntry(&p,first,0,HFEXACT,12);
 assert(GetPvLine(12,&p)==1);assert(p.fiftyMove==99 && p.hisPly==0);
 assert(RootPV(&p,first,12,pv)==1);assert(p.fiftyMove==99 && p.hisPly==0);
 fen(&p,"7k/8/8/8/8/8/8/K7 w - - 0 1");assert(PvDrawReached(&p));
 /* Checkmate still overrides the fifty-move threshold in both search paths. */
 fen(&p,"7k/6Q1/5K2/8/8/8/8/8 b - - 100 51");p.ply=1;
 S_SEARCHINFO info={0};assert(Quiescence(-INFINITE,INFINITE,&p,&info)==-MATE+1);
 assert(AlphaBeta(-INFINITE,INFINITE,2,&p,&info,TRUE)==-MATE+1);
 free(p.HashTable->pTable);puts("PV DRAW TERMINATION TESTS PASS");
}

static void TestAdaptiveTime(void) {
 S_SEARCHINFO info={0};info.GAME_MODE=UCIMODE;info.adaptiveTime=TRUE;
 info.timeset=TRUE;info.softBudget=1000;info.timeStart=10000;info.stoptime=12000;
 assert(!SoftTimeReached(&info,4,3,FALSE,0,10749));
 assert(SoftTimeReached(&info,4,3,FALSE,0,10750));
 assert(!SoftTimeReached(&info,4,0,FALSE,0,10999));
 assert(SoftTimeReached(&info,4,0,FALSE,0,11000));
 assert(!SoftTimeReached(&info,4,0,TRUE,0,11499));
 assert(SoftTimeReached(&info,4,0,TRUE,0,11500));
 assert(!SoftTimeReached(&info,4,0,FALSE,35,11499));
 assert(SoftTimeReached(&info,4,0,FALSE,35,11500));
 assert(SoftTimeReached(&info,4,0,FALSE,34,11000));
 /* Clock cap takes priority over the unstable-position extension. */
 info.stoptime=11100;assert(SoftTimeReached(&info,4,0,TRUE,0,11100));
 assert(!SoftTimeReached(&info,3,3,FALSE,0,12000));
 info.pondering=TRUE;assert(!SoftTimeReached(&info,4,3,FALSE,0,12000));
 info.pondering=FALSE;info.infinite=TRUE;assert(!SoftTimeReached(&info,4,3,FALSE,0,12000));
 info.infinite=FALSE;info.timeset=FALSE;assert(!SoftTimeReached(&info,4,3,FALSE,0,12000));
 info.timeset=TRUE;info.adaptiveTime=FALSE;assert(!SoftTimeReached(&info,4,3,FALSE,0,12000));
 info.adaptiveTime=TRUE;info.GAME_MODE=CONSOLEMODE;
 assert(!SoftTimeReached(&info,4,3,FALSE,0,12000));
 puts("ADAPTIVE TIME TARGET TESTS PASS");
}

int main(void) {
 ReferenceInitMvvLva();
 setbuf(stdout,NULL);AllInit();TestLegalProbe();TestPvDrawTermination();EngineOptions->UseBook=FALSE;
 S_BOARD p={0};S_SEARCHINFO info={0};info.GAME_MODE=UCIMODE;
 TestAdaptiveTime();TestQSearchEquivalence();TestHashFull();TestRestrictedRoot();TestPassedKings();TestPawnEvaluation();TestDeferredOrdering();TestSeeCache();
 const char *fens[]={START_FEN,"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1","8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1","r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"};
 U64 expected[]={197281,97862,43238,9467};int depths[]={4,3,4,3};
 for(int i=0;i<4;i++){fen(&p,fens[i]);U64 n=perft(&p,depths[i]);printf("perft %d: %llu\n",i,n);assert(n==expected[i]);int ev=EvalPosition(&p);MirrorBoard(&p);assert(EvalPosition(&p)==ev);}
 see(&p,"7k/8/4p3/3p4/8/8/8/K2Q4 w - - 0 1","d1d5",-900);
 see(&p,"7k/8/8/3p4/8/8/8/K2Q4 w - - 0 1","d1d5",100);
 see(&p,"7k/8/4p3/3q4/2P5/8/8/K7 w - - 0 1","c4d5",900);
 see(&p,"7k/8/8/3pP3/8/8/K7/3r4 w - d6 0 1","e5d6",0);
 see(&p,"1r5k/P7/8/8/8/8/8/K7 w - - 0 1","a7b8q",1450);
 see(&p,"7k/8/8/8/8/8/pR6/1b5K w - - 0 1","b2b1",-1125);
 see(&p,"4k3/8/4p3/3p4/8/8/8/K2QR3 w - - 0 1","d1d5",100);
 see(&p,"8/8/4k3/3p4/8/8/8/K2Q1R2 w - - 0 1","d1d5",-900);
 fen(&p,"7k/6Q1/5K2/8/8/8/8/8 b - - 0 1");assert(Quiescence(-INFINITE,INFINITE,&p,&info)==-MATE);
 p.fiftyMove=100;assert(Quiescence(-INFINITE,INFINITE,&p,&info)==-MATE);
 fen(&p,"7k/5K2/6Q1/8/8/8/8/8 b - - 0 1");assert(Quiescence(-INFINITE,INFINITE,&p,&info)==0);
 fen(&p,"7k/P7/8/8/8/8/8/K7 w - - 0 1");S_MOVELIST list;GenerateAllCaps(&p,&list);assert(list.count==4);assert(Quiescence(-INFINITE,INFINITE,&p,&info)>800);
 fen(&p,"7k/8/8/8/8/8/8/KR6 w - - 99 50");assert(p.fiftyMove==99);assert(!IsRepetition(&p));
 InitHashTable(p.HashTable,1);fen(&p,"7k/8/8/8/8/8/8/KR6 w - - 0 1");ClearForSearch(&p,&info);
 int score=AlphaBeta(-INFINITE,INFINITE,1,&p,&info,TRUE);assert(score>0);
 p.fiftyMove=99;assert(AlphaBeta(-INFINITE,INFINITE,1,&p,&info,TRUE)==0);
 fen(&p,START_FEN);U64 key=p.posKey;MakeNullMove(&p);assert(p.posKey==GeneratePosKey(&p));assert(!IsRepetition(&p));TakeNullMove(&p);assert(p.posKey==key);
 const char *moves[]={"g1f3","g8f6","f3g1","f6g8"};
 for(int i=0;i<4;i++){int m=ParseMove((char*)moves[i],&p);assert(MakeMove(&p,m));p.ply=0;}assert(IsRepetition(&p));
 MakeNullMove(&p);assert(!IsRepetition(&p));TakeNullMove(&p);
 fen(&p,START_FEN);p.ply=MAXDEPTH-1;int m=ParseMove("e2e4",&p);assert(!MakeMove(&p,m));assert(AlphaBeta(-INFINITE,INFINITE,MAXDEPTH-1,&p,&info,TRUE)==EvalPosition(&p));p.ply=0;
 key=p.posKey;assert(ParseFen("8/8/8/8/8/8/8/8 w - - 0 1",&p)==-1);assert(p.posKey==key);
 /* Repetition canonicalizes unavailable and pinned EP rights. */
 fen(&p,"7k/8/8/3p4/8/8/K7/8 w - d6 0 1");U64 rep=RepetitionKey(&p);
 fen(&p,"7k/8/8/3p4/8/8/K7/8 w - - 0 1");assert(RepetitionKey(&p)==rep);
 fen(&p,"k3r3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");rep=RepetitionKey(&p);
 fen(&p,"k3r3/8/8/3pP3/8/8/8/4K3 w - - 0 1");assert(RepetitionKey(&p)==rep);
 fen(&p,"7k/8/8/3pP3/8/8/K7/8 w - d6 0 1");rep=RepetitionKey(&p);
 fen(&p,"7k/8/8/3pP3/8/8/K7/8 w - - 0 1");assert(RepetitionKey(&p)!=rep);
 /* A legal king cannot recapture onto the defending bishop's diagonal. */
 see(&p,"8/8/4k3/3p4/8/8/6B1/K2Q4 w - - 0 1","d1d5",100);
 /* Seeded random legal play: mirror symmetry and complete undo invariants. */
 unsigned seed=42;
 for(int game=0;game<40;game++) {
  fen(&p,START_FEN);
  for(int turn=0;turn<120;turn++) {
   assert(CheckBoard(&p));assert(GeneratePosKey(&p)==p.posKey);CompareSee(&p);CompareLegalProbe(&p);
   for(int mode=0;mode<=1;mode++) {EngineOptions->TaperedEval=mode;int eval=EvalPosition(&p);assert(eval==ReferenceEvalPosition(&p));S_BOARD mirror=p;MirrorBoard(&mirror);assert(EvalPosition(&mirror)==eval);}
   GenerateAllMoves(&p,&list);int legal[256],n=0;
   for(int i=0;i<list.count;i++) if(MakeMove(&p,list.moves[i].move)) {TakeMove(&p);legal[n++]=list.moves[i].move;}
   if(!n) break;
   seed=seed*1664525u+1013904223u;assert(MakeMove(&p,legal[seed%(unsigned)n]));p.ply=0;
  }
  while(p.hisPly) {p.ply=1;TakeMove(&p);assert(CheckBoard(&p));}
  S_BOARD initial={0};fen(&initial,START_FEN);assert(p.posKey==initial.posKey);assert(memcmp(p.pieces,initial.pieces,sizeof p.pieces)==0);
 }
 EngineOptions->TaperedEval=TRUE;
 /* Mate score normalization survives a different probing ply. */
 fen(&p,START_FEN);ClearHashTable(p.HashTable);m=ParseMove("e2e4",&p);p.ply=5;StoreHashEntry(&p,m,MATE-10,HFEXACT,4);p.ply=2;int probed=0,pm=0;assert(ProbeHashEntry(&p,&pm,&probed,-INFINITE,INFINITE,4));assert(probed==MATE-7);p.ply=0;
 fen(&p,START_FEN);p.hisPly=MAXGAMEMOVES-1;ClearForSearch(&p,&info);assert(p.hisPly==0);assert(HasLegalMove(&p));
 /* A complete search must still select a legal move with no table. */
 free(p.HashTable->pTable);p.HashTable->pTable=NULL;p.HashTable->numEntries=0;info.depth=2;info.starttime=GetTimeMs();SearchPosition(&p,&info);assert(rootBestMove!=NOMOVE && MoveExists(&p,rootBestMove));
 fen(&p,"7k/6Q1/5K2/8/8/8/8/8 b - - 0 1");SearchPosition(&p,&info);
 printf("SEE differential: %lu captures, %lu attack queries PASS\n",seeComparisons,attackComparisons);CleanPolyBook();printf("Legal probe differential: %lu positions PASS\n",legalComparisons);puts("CORE TESTS PASS");return 0;
}
