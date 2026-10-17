#include "defs.h"
#include <string.h>
#include <limits.h>

static int counterMoves[13][BRD_SQ_NUM];
static int rootBestMove;

/* Sample current-search TT entries, reported in UCI permille (0..1000). */
static int HashFull(const S_HASHTABLE *table) {
 if(!table->pTable || table->numEntries<=0) return 0;
 int samples=table->numEntries<1000?table->numEntries:1000,used=0;
 for(int i=0;i<samples;i++) {
  const S_HASHENTRY *entry=&table->pTable[i];
  if(entry->depth>0 && entry->generation==table->generation) used++;
 }
 return used*1000/samples;
}
static void CheckUp(S_SEARCHINFO *info) {
 if(info->timeset && GetTimeMs() >= info->stoptime) info->stopped=TRUE;
 ReadInput(info);
}
static void PickNextMove(int n,S_MOVELIST *list) {
 int best=n;
 for(int i=n+1;i<list->count;i++) if(list->moves[i].score>list->moves[best].score) best=i;
 S_MOVE tmp=list->moves[n];list->moves[n]=list->moves[best];list->moves[best]=tmp;
}
static int IsRepetition(const S_BOARD *pos) {
 U64 key=RepetitionKey(pos);
 int first=pos->hisPly-pos->fiftyMove;
 if(first<0) first=0;
 for(int i=pos->hisPly-1;i>=first;i--) {
  if(pos->history[i].move==NOMOVE) break; /* Never cross a synthetic null move. */
  if(pos->history[i].posKey==key) return TRUE;
 }
 return FALSE;
}
static void ClearForSearch(S_BOARD *pos,S_SEARCHINFO *info) {
 /* Keep enough real history for draw claims while reserving search stack space. */
 if(pos->hisPly>=MAXGAMEMOVES-MAXDEPTH) {
  int keep=pos->fiftyMove;if(keep>100) keep=100;if(keep>pos->hisPly) keep=pos->hisPly;
  memmove(pos->history,pos->history+pos->hisPly-keep,(size_t)keep*sizeof pos->history[0]);
  pos->hisPly=keep;
 }
 memset(counterMoves,0,sizeof counterMoves);
 memset(pos->searchHistory,0,sizeof pos->searchHistory);
 memset(pos->searchKillers,0,sizeof pos->searchKillers);
 memset(pos->PvArray,0,sizeof pos->PvArray);
 pos->HashTable->overWrite=pos->HashTable->hit=pos->HashTable->cut=0;
 pos->HashTable->generation++;
 pos->ply=0;info->stopped=0;info->nodes=0;info->seldepth=0;info->fh=info->fhf=0;info->nullCut=0;
 rootBestMove=NOMOVE;
}
static int AtLimit(const S_BOARD *pos) {
 return pos->ply>=MAXDEPTH-1 || pos->hisPly>=MAXGAMEMOVES-1;
}
static int DrawScore(S_BOARD *pos,int inCheck) {
 /* Checkmate takes precedence over a draw claim. */
 return inCheck && !HasLegalMove(pos) ? -MATE+pos->ply : 0;
}
/* Score quiet moves before searching: descendants may update counterMoves. */
static int OrderMoves(S_BOARD *pos,S_MOVELIST *list,int hashMove) {
 int deferred=FALSE;
 if(hashMove!=NOMOVE) for(int i=0;i<list->count;i++) {
  if(list->moves[i].move==hashMove) {deferred=TRUE;break;}
 }
 int reply=NOMOVE;
 if(pos->hisPly>0) {
  int prev=pos->history[pos->hisPly-1].move;
  if(prev!=NOMOVE) reply=counterMoves[pos->pieces[TOSQ(prev)]][TOSQ(prev)];
 }
 for(int i=0;i<list->count;i++) {
  int move=list->moves[i].move;
  list->moves[i].see=INT_MAX;
  if(move==hashMove) list->moves[i].score=2000000;
  else if(PROMOTED(move)) list->moves[i].score=1500000+PieceVal[PROMOTED(move)];
  else if(move&MFLAGCAP) {
   /* Preserve MVV/LVA within each exchange category. */
   if(!deferred) {
    list->moves[i].see=SEE(pos,move);
    if(list->moves[i].see<0) list->moves[i].score-=1500000;
   }
  } else if(move==reply && list->moves[i].score<700000) list->moves[i].score=700000;
 }
 return deferred;
}
/* The hash move has already been selected and swapped into slot zero. Keep
   that exact swap and the original scores to preserve ordering of tied moves. */
static void ScoreRemainingCaptures(S_BOARD *pos,S_MOVELIST *list) {
 for(int i=1;i<list->count;i++) {
  int move=list->moves[i].move;
  if((move&MFLAGCAP) && !PROMOTED(move)) {
   list->moves[i].see=SEE(pos,move);
   if(list->moves[i].see<0) list->moves[i].score-=1500000;
  }
 }
}
static int Quiescence(int alpha,int beta,S_BOARD *pos,S_SEARCHINFO *info) {
 ASSERT(CheckBoard(pos));
 if((info->nodes&255)==0) CheckUp(info);
 if(info->nodeLimit>0 && info->nodes>=info->nodeLimit) info->stopped=TRUE;
 if(info->stopped) return 0;
 info->nodes++;
 if(pos->ply>info->seldepth) info->seldepth=pos->ply;
 if(AtLimit(pos)) return EvalPosition(pos);
 int inCheck=SqAttacked(pos->KingSq[pos->side],pos->side^1,pos);
 if(IsRepetition(pos) || pos->fiftyMove>=100 || DeadMaterial(pos)) return DrawScore(pos,inCheck);
 if(!inCheck) {
  int stand=EvalPosition(pos);
  /* A stand-pat cutoff still needs an explicit stalemate check. */
  if(stand>=beta) return HasLegalMove(pos) ? beta : 0;
  if(stand>alpha) alpha=stand;
 }
 S_MOVELIST list;
 int legal=0;
 if(inCheck) GenerateAllMoves(pos,&list); else GenerateAllCaps(pos,&list);
 int deferred=OrderMoves(pos,&list,ProbePvMove(pos));
 for(int i=0;i<list.count;i++) {
  if(i==1 && deferred) ScoreRemainingCaptures(pos,&list);
  PickNextMove(i,&list);
  int move=list.moves[i].move;
  int losing=!inCheck && !PROMOTED(move) && (move&MFLAGCAP) && (list.moves[i].see==INT_MAX?SEE(pos,move):list.moves[i].see)<0;
  if(!MakeMove(pos,move)) continue;
  legal++; /* Even a SEE-pruned capture proves this is not stalemate. */
  int givesCheck=SqAttacked(pos->KingSq[pos->side],pos->side^1,pos);
  if(losing && !givesCheck) {TakeMove(pos);continue;}
  int score=-Quiescence(-beta,-alpha,pos,info);
  TakeMove(pos);
  if(info->stopped) return 0;
  if(score>=beta) return beta;
  if(score>alpha) alpha=score;
 }
 if(!legal) {
  if(inCheck) return -MATE+pos->ply;
  /* Only positions with no legal tactical move need the quiet-move probe. */
  if(!HasLegalMove(pos)) return 0;
 }
 return alpha;
}
static int RootAllowed(const S_SEARCHINFO *info,int move) {
 if(info->restricted) {
  int found=FALSE;
  for(int i=0;i<info->searchMoveCount;i++) if(info->searchMoves[i]==move) found=TRUE;
  if(!found) return FALSE;
 }
 for(int i=0;i<info->excludedCount;i++) if(info->excludedMoves[i]==move) return FALSE;
 return TRUE;
}
static int AlphaBeta(int alpha,int beta,int depth,S_BOARD *pos,S_SEARCHINFO *info,int doNull) {
 ASSERT(CheckBoard(pos));
 if(depth<=0) return Quiescence(alpha,beta,pos,info);
 if((info->nodes&255)==0) CheckUp(info);
 if(info->nodeLimit>0 && info->nodes>=info->nodeLimit) info->stopped=TRUE;
 if(info->stopped) return 0;
 info->nodes++;
 if(pos->ply>info->seldepth) info->seldepth=pos->ply;
 if(AtLimit(pos)) return EvalPosition(pos);
 int inCheck=SqAttacked(pos->KingSq[pos->side],pos->side^1,pos);
 if(pos->ply && (IsRepetition(pos) || pos->fiftyMove>=100 || DeadMaterial(pos))) return DrawScore(pos,inCheck);
 if(inCheck && depth<MAXDEPTH-1) depth++;
 if(depth>MAXDEPTH-1) depth=MAXDEPTH-1;
 const int pvNode=beta-alpha>1;
 int score=-INFINITE,hashMove=NOMOVE;
 int hit=ProbeHashEntry(pos,&hashMove,&score,alpha,beta,depth);
 if(hit && pos->ply) {pos->HashTable->cut++;return score;}
 if(doNull && !inCheck && pos->ply && beta-alpha==1 && pos->bigPce[pos->side]>1 && depth>=4 && pos->fiftyMove<90) {
  MakeNullMove(pos);score=-AlphaBeta(-beta,-beta+1,depth-4,pos,info,FALSE);TakeNullMove(pos);
  if(info->stopped) return 0;
  if(score>=beta && abs(score)<ISMATE) {info->nullCut++;return beta;}
 }
 S_MOVELIST list;GenerateAllMoves(pos,&list);
 int deferred=OrderMoves(pos,&list,hashMove);
 int legal=0,oldAlpha=alpha,best=-INFINITE,bestMove=NOMOVE;
 for(int i=0;i<list.count;i++) {
  if(i==1 && deferred) ScoreRemainingCaptures(pos,&list);
  PickNextMove(i,&list);int move=list.moves[i].move;
  if(!pos->ply && !RootAllowed(info,move)) continue;
  int quiet=!(move&(MFLAGCAP|MFLAGPROM));
  int killer=move==pos->searchKillers[0][pos->ply] || move==pos->searchKillers[1][pos->ply];
  if(!MakeMove(pos,move)) continue;
  legal++;
  int childDepth=depth-1,reduced=childDepth;
  int givesCheck=SqAttacked(pos->KingSq[pos->side],pos->side^1,pos);
  if(!inCheck && quiet && !givesCheck && !killer && depth>=3 && legal>=4) {
   int reduction=depth>=6 && legal>=8 ? 2:1;
   /* Keep one extra ply on wide-window nodes establishing the principal line. */
   if(pvNode) reduction--;
   reduced-=reduction;
   if(reduced<1) reduced=1;
  }
  if(legal==1) score=-AlphaBeta(-beta,-alpha,childDepth,pos,info,TRUE);
  else {
   score=-AlphaBeta(-alpha-1,-alpha,reduced,pos,info,TRUE);
   if(!info->stopped && score>alpha && reduced!=childDepth) score=-AlphaBeta(-alpha-1,-alpha,childDepth,pos,info,TRUE);
   if(!info->stopped && score>alpha && score<beta) score=-AlphaBeta(-beta,-alpha,childDepth,pos,info,TRUE);
  }
  TakeMove(pos);
  if(info->stopped) return 0;
  if(score>best) {best=score;bestMove=move;if(!pos->ply) rootBestMove=move;}
  if(score>alpha) {
   if(score>=beta) {
    info->fh++;if(legal==1) info->fhf++;
    if(quiet) {
     pos->searchKillers[1][pos->ply]=pos->searchKillers[0][pos->ply];pos->searchKillers[0][pos->ply]=move;
     int *history=&pos->searchHistory[pos->pieces[FROMSQ(move)]][TOSQ(move)];
     int bonus=depth*depth;if(bonus>512) bonus=512;
     *history+=bonus-(*history*bonus)/16384;
     if(pos->hisPly>0) {
      int prev=pos->history[pos->hisPly-1].move;
      if(prev!=NOMOVE) counterMoves[pos->pieces[TOSQ(prev)]][TOSQ(prev)]=move;
     }
    }
    if(pos->ply || (!info->restricted && EngineOptions->MultiPV<=1)) StoreHashEntry(pos,move,beta,HFBETA,depth);
    return beta;
   }
   alpha=score;
  }
 }
 if(!legal) return inCheck ? -MATE+pos->ply : 0;
 if(pos->ply || (!info->restricted && EngineOptions->MultiPV<=1)) StoreHashEntry(pos,bestMove,alpha,alpha>oldAlpha?HFEXACT:HFALPHA,depth);
 return alpha;
}
typedef struct {
 int score, count, seldepth;
 int pv[MAXDEPTH];
} S_ROOTLINE;

/* Restricted roots are not stored as full-position TT bounds. Build their PV
   explicitly from the selected root move, then follow legal child hash moves. */
static int RootPV(S_BOARD *pos,int first,int depth,int *pv) {
 int count=0,move=first,startPly=pos->ply;
 while(move!=NOMOVE && count<depth) {
  if(!MoveExists(pos,move) || !MakeMove(pos,move)) break;
  pv[count++]=move;
  if(PvDrawReached(pos)) break;
  move=ProbePvMove(pos);
 }
 while(pos->ply>startPly) TakeMove(pos);
 return count;
}
static void PrintInfo(S_BOARD *pos,S_SEARCHINFO *info,int depth,int score,
                      int rank,int count,const int *pv,int seldepth) {
 if(info->GAME_MODE==UCIMODE) {
  if(abs(score)>ISMATE) printf("info score mate %d ",score>0?(MATE-score+1)/2:-(MATE+score+1)/2);
  else printf("info score cp %d ",score);
  int64_t elapsed=GetTimeMs()-info->starttime;if(elapsed<0) elapsed=0;
  double nps=(double)info->nodes*1000.0/(double)(elapsed>0?elapsed:1);
  printf("depth %d seldepth %d nodes %ld nps %.0f hashfull %d time %" PRId64,
         depth,seldepth,info->nodes,nps,HashFull(pos->HashTable),elapsed);
  if(rank) printf(" multipv %d",rank);
  printf(" pv");
 } else if(info->POST_THINKING) printf("%d %d %" PRId64 " %ld",depth,score,(GetTimeMs()-info->starttime)/10,info->nodes);
 if(info->GAME_MODE==UCIMODE || info->POST_THINKING) {
  for(int i=0;i<count;i++) {
   printf(" %s",PrMove(pv[i]));
  }
  printf("\n");
 }
}
void SearchPosition(S_BOARD *pos,S_SEARCHINFO *info) {
 ClearForSearch(pos,info);info->excludedCount=0;
 int bestMove=NOMOVE,bestScore=0,ponderMove=NOMOVE;
 int multi=info->GAME_MODE==UCIMODE?EngineOptions->MultiPV:1;
 if(multi<1) multi=1;
 if(multi>16) multi=16;
 S_MOVELIST list;GenerateAllMoves(pos,&list);
 int available=0;
 for(int i=0;i<list.count;i++) if(RootAllowed(info,list.moves[i].move) && MakeMove(pos,list.moves[i].move)) {
  TakeMove(pos);if(bestMove==NOMOVE) bestMove=list.moves[i].move;available++;
  if(multi==1) break; /* Preserve the normal single-PV search path. */
 }
 if(multi>available && available) multi=available;
 int book=EngineOptions->UseBook && bestMove!=NOMOVE && !info->restricted &&
          EngineOptions->MultiPV<=1 && !info->pondering && !info->infinite ? GetBookMove(pos):NOMOVE;
 if(book!=NOMOVE && MoveExists(pos,book)) bestMove=book;else book=NOMOVE;
 int maxDepth=info->depth;
 if(maxDepth<1) maxDepth=1;
 if(maxDepth>=MAXDEPTH) maxDepth=MAXDEPTH-1;
 if(bestMove!=NOMOVE && book==NOMOVE) for(int depth=1;depth<=maxDepth;depth++) {
  info->seldepth=0;
  if(multi>1) {
   S_ROOTLINE lines[16];int completed=0;info->excludedCount=0;
   for(int k=0;k<multi;k++) {
    info->seldepth=0;rootBestMove=NOMOVE;
    int score=AlphaBeta(-INFINITE,INFINITE,depth,pos,info,TRUE);
    if(info->stopped || rootBestMove==NOMOVE) break;
    lines[k].score=score;lines[k].seldepth=info->seldepth;
    lines[k].count=RootPV(pos,rootBestMove,depth,lines[k].pv);
    info->excludedMoves[info->excludedCount++]=rootBestMove;completed++;
   }
   info->excludedCount=0;
   if(completed!=multi) break; /* Retain the previous complete result on stop. */
   for(int a=1;a<multi;a++) for(int b=a;b>0 && lines[b].score>lines[b-1].score;b--) {
    S_ROOTLINE tmp=lines[b];lines[b]=lines[b-1];lines[b-1]=tmp;
   }
   bestMove=lines[0].pv[0];bestScore=lines[0].score;
   ponderMove=lines[0].count>1?lines[0].pv[1]:NOMOVE;
   for(int k=0;k<multi;k++) PrintInfo(pos,info,depth,lines[k].score,k+1,lines[k].count,lines[k].pv,lines[k].seldepth);
   continue;
  }
  int a=-INFINITE,b=INFINITE;
  if(depth>1 && abs(bestScore)<ISMATE) {a=bestScore-50;b=bestScore+50;}
  int score=AlphaBeta(a,b,depth,pos,info,TRUE);
  if(!info->stopped && (score<=a || score>=b) && (a!=-INFINITE || b!=INFINITE)) score=AlphaBeta(-INFINITE,INFINITE,depth,pos,info,TRUE);
  if(info->stopped) break;
  if(rootBestMove!=NOMOVE) bestMove=rootBestMove;
  bestScore=score;
  int pvCount=info->restricted?RootPV(pos,bestMove,depth,pos->PvArray):GetPvLine(depth,pos);
  if(pvCount==0 || pos->PvArray[0]!=bestMove) {pos->PvArray[0]=bestMove;pvCount=1;}
  ponderMove=pvCount>1?pos->PvArray[1]:NOMOVE;
  PrintInfo(pos,info,depth,score,EngineOptions->MultiPV>1?1:0,pvCount,pos->PvArray,info->seldepth);
  if(abs(score)>ISMATE) break;
 }
 if(info->GAME_MODE==UCIMODE) {
  WaitForSearchSignal(info);
  if(!info->quit) {
   printf("bestmove %s",PrMove(bestMove));
   if(EngineOptions->Ponder && ponderMove!=NOMOVE) printf(" ponder %s",PrMove(ponderMove));
   printf("\n");
  }
 } else if(bestMove!=NOMOVE) {
  printf("Jace plays %s\n",PrMove(bestMove));
  (void)MakeMove(pos,bestMove);pos->ply=0;
 }
}
