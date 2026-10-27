/* Frozen baseline quiescence for differential regression tests. */
static int ReferenceQuiescence(int alpha,int beta,S_BOARD *pos,S_SEARCHINFO *info) {
 ASSERT(CheckBoard(pos));
 if((info->nodes&255)==0) CheckUp(info);
 if(info->nodeLimit>0 && info->nodes>=info->nodeLimit) info->stopped=TRUE;
 if(info->stopped) return 0;
 info->nodes++;
 if(AtLimit(pos)) return EvalPosition(pos);
 int inCheck=SqAttacked(pos->KingSq[pos->side],pos->side^1,pos);
 if(IsRepetition(pos) || pos->fiftyMove>=100 || DeadMaterial(pos)) return DrawScore(pos,inCheck);
 /* Stalemate must be recognized before stand-pat can cut off. */
 if(!HasLegalMove(pos)) return inCheck ? -MATE+pos->ply : 0;
 if(!inCheck) {
  int stand=EvalPosition(pos);
  if(stand>=beta) return beta;
  if(stand>alpha) alpha=stand;
 }
 S_MOVELIST list;
 if(inCheck) GenerateAllMoves(pos,&list); else GenerateAllCaps(pos,&list);
 OrderMoves(pos,&list,ProbePvMove(pos),info);
 for(int i=0;i<list.count;i++) {
  PickNextMove(i,&list);
  int move=list.moves[i].move;
  int losing=!inCheck && !PROMOTED(move) && (move&MFLAGCAP) && SEE(pos,move)<0;
  if(!MakeMove(pos,move)) continue;
  int givesCheck=SqAttacked(pos->KingSq[pos->side],pos->side^1,pos);
  if(losing && !givesCheck) {TakeMove(pos);continue;}
  int score=-ReferenceQuiescence(-beta,-alpha,pos,info);
  TakeMove(pos);
  if(info->stopped) return 0;
  if(score>=beta) return beta;
  if(score>alpha) alpha=score;
 }
 return alpha;
}
