// perft.c

#include "defs.h"
#include "stdio.h"

//long leafNodes;
U64 leafNodes;

static void Perft(int depth, S_BOARD *pos) {

    ASSERT(CheckBoard(pos));  

	if(depth == 0) {
        leafNodes++;
        return;
    }	

    S_MOVELIST list[1];
    GenerateAllMoves(pos,list);
      
    int MoveNum = 0;
	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {	
       
        if ( !MakeMove(pos,list->moves[MoveNum].move))  {
            continue;
        }
        Perft(depth - 1, pos);
        TakeMove(pos);
    }

    return;
}


void PerftTest(int depth, S_BOARD *pos) {

    ASSERT(CheckBoard(pos));

	PrintBoard(pos);
	printf("\nStarting Test To Depth: %d\n\n",depth);	
	leafNodes = 0;
	if(depth<1 || depth>=MAXDEPTH) return;
	int64_t start = GetTimeMs();
    S_MOVELIST list[1];
    GenerateAllMoves(pos,list);	
    
    int move;	    
    int MoveNum = 0;
	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {
        move = list->moves[MoveNum].move;
        if ( !MakeMove(pos,move))  {
            continue;
        }
        U64 cumnodes = leafNodes;
        Perft(depth - 1, pos);
        TakeMove(pos);        
        U64 oldnodes = leafNodes - cumnodes;
        printf("move %2d : %s : %llu\n",MoveNum+1,PrMove(move),oldnodes);
    }
	
	printf("\nTest Complete : %llu nodes visited in %" PRId64 " ms\n",leafNodes,GetTimeMs() - start);

    return;
}
