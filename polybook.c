#include "defs.h"
#include "polykeys.h"

typedef struct {
	U64 key;
	unsigned short move;
	unsigned short weight;
	unsigned int learn;
	
} S_POLY_BOOK_ENTRY;

static size_t NumEntries;

static S_POLY_BOOK_ENTRY *entries;

const int PolyKindOfPiece[13] = {
	-1, 1, 3, 5, 7, 9, 11, 0, 2, 4, 6, 8, 10
};

static U64 ReadBigEndian(const unsigned char *p,int n) {
 U64 v=0;for(int i=0;i<n;i++) v=(v<<8)|p[i];return v;
}
void InitPolyBook(void) {
 EngineOptions->UseBook=FALSE;CleanPolyBook();
 FILE *file=fopen("performance.bin","rb");if(!file) return;
 if(fseek(file,0,SEEK_END)!=0) {fclose(file);return;}
 long length=ftell(file);
 if(length<16 || length%16) {fclose(file);return;}
 size_t count=(size_t)length/16;
 if(count>SIZE_MAX/sizeof *entries) {fclose(file);return;}
 entries=malloc(count*sizeof *entries);
 if(!entries) {fclose(file);return;}
 rewind(file);
 for(size_t i=0;i<count;i++) {
  unsigned char bytes[16];
  if(fread(bytes,1,16,file)!=16) {CleanPolyBook();fclose(file);return;}
  entries[i].key=ReadBigEndian(bytes,8);
  entries[i].move=(unsigned short)ReadBigEndian(bytes+8,2);
  entries[i].weight=(unsigned short)ReadBigEndian(bytes+10,2);
  entries[i].learn=0;
  if(i && entries[i].key<entries[i-1].key) {CleanPolyBook();fclose(file);return;}
 }
 fclose(file);NumEntries=count;EngineOptions->UseBook=TRUE;
}
void CleanPolyBook(void) {free(entries);entries=NULL;NumEntries=0;}

static int HasPawnForCapture(const S_BOARD *board) {
	int sqWithPawn = 0;
	int targetPce = (board->side == WHITE) ? wP : bP;
	if(board->enPas != NO_SQ) {
		if(board->side == WHITE) {
			sqWithPawn = board->enPas - 10;
		} else {
			sqWithPawn = board->enPas + 10;
		}
		
		if(board->pieces[sqWithPawn + 1] == targetPce) {
			return TRUE;
		} else if(board->pieces[sqWithPawn - 1] == targetPce) {
			return TRUE;
		} 
	}
	return FALSE;
}

static U64 PolyKeyFromBoard(const S_BOARD *board) {

	int sq = 0, rank = 0, file = 0;
	U64 finalKey = 0;
	int piece = EMPTY;
	int polyPiece = 0;
	int offset = 0;
	
	for(sq = 0; sq < BRD_SQ_NUM; ++sq) {
		piece = board->pieces[sq];
		if(piece != NO_SQ && piece != EMPTY && piece != OFFBOARD) {
			ASSERT(piece >= wP && piece <= bK);
			polyPiece = PolyKindOfPiece[piece];
			rank = RanksBrd[sq];
			file = FilesBrd[sq];
			finalKey ^= Random64Poly[(64 * polyPiece) + (8 * rank) + file];
		}
	}
	
	// castling
	offset = 768;
	
	if(board->castlePerm & WKCA) finalKey ^= Random64Poly[offset + 0];
	if(board->castlePerm & WQCA) finalKey ^= Random64Poly[offset + 1];
	if(board->castlePerm & BKCA) finalKey ^= Random64Poly[offset + 2];
	if(board->castlePerm & BQCA) finalKey ^= Random64Poly[offset + 3];
	
	// enpassant
	offset = 772;
	if(HasPawnForCapture(board) == TRUE) {
		file = FilesBrd[board->enPas];
		finalKey ^= Random64Poly[offset + file];
	}
	
	if(board->side == WHITE) {
		finalKey ^= Random64Poly[780];
	}
	return finalKey;
}

static int ConvertPolyMoveToInternalMove(unsigned short polyMove, S_BOARD *board) {
	
	int ff = (polyMove >> 6) & 7;
	int fr = (polyMove >> 9) & 7;
	int tf = (polyMove >> 0) & 7;
	int tr = (polyMove >> 3) & 7;
	int pp = (polyMove >> 12) & 7;
	
	if(pp>4) return NOMOVE;
    int piece=board->pieces[FR2SQ(ff,fr)];
    if(IsKi(piece) && ff==4 && fr==tr) {if(tf==7) tf=6;else if(tf==0) tf=2;}
    char moveString[6];
	if(pp == 0) {
		snprintf(moveString,sizeof moveString, "%c%c%c%c",
		FileChar[ff],
		RankChar[fr],
		FileChar[tf],
		RankChar[tr]);
	} else {
		char promChar = 'q';
		switch(pp) {
			case 1: promChar = 'n'; break;
			case 2: promChar = 'b'; break;
			case 3: promChar = 'r'; break;
		}
		snprintf(moveString,sizeof moveString, "%c%c%c%c%c",
		FileChar[ff],
		RankChar[fr],
		FileChar[tf],
		RankChar[tr],
		promChar);
	}
	
	return ParseMove(moveString, board);
}

int GetBookMove(S_BOARD *board) {
 if(!entries || !NumEntries) return NOMOVE;
 U64 key=PolyKeyFromBoard(board);size_t lo=0,hi=NumEntries;
 while(lo<hi) {size_t mid=lo+(hi-lo)/2;if(entries[mid].key<key) lo=mid+1;else hi=mid;}
 U64 total=0;int chosen=NOMOVE;
 for(size_t i=lo;i<NumEntries && entries[i].key==key;i++) {
  int move=ConvertPolyMoveToInternalMove(entries[i].move,board);
  if(move==NOMOVE || !MoveExists(board,move)) continue;
  U64 weight=entries[i].weight?entries[i].weight:1;
  total+=weight;
  U64 random=((U64)(unsigned)rand()<<32) ^ (U64)(unsigned)rand();
  if(random%total<weight) chosen=move;
 }
 return chosen;
}
