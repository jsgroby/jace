/* Standalone, single-threaded evaluator; never linked into the UCI engine. */
#include "defs.h"
#include <math.h>
#include <string.h>

#ifdef JACE_TUNING
#define PARAM(name, value, low, high) extern int name;
#include "eval_params.def"
#undef PARAM
#define PARAM(name, value, low, high) {#name, &name, low, high},
static struct {const char *name; int *value,low,high;} parameters[]={
#include "eval_params.def"
};
#undef PARAM
#endif

typedef struct {
 int pieces[BRD_SQ_NUM],pceNum[13],pList[13][10],KingSq[2],material[2],side;
 U64 pawns[3];
 double result;
} Sample;
#define COPY_FIELD(field) memcpy(sample->field, board.field, sizeof sample->field)
#define RESTORE_FIELD(field) memcpy(board.field, sample->field, sizeof sample->field)
int main(int argc,char **argv) {
 if(argc!=2) {fprintf(stderr,"usage: evaluator DATA.tsv\n");return 2;}
 AllInit();
 FILE *file=fopen(argv[1],"r");if(!file) {perror(argv[1]);return 2;}
 Sample *samples=NULL;size_t count=0,capacity=0;char line[512];S_BOARD board={0};
 while(fgets(line,sizeof line,file)) {
  char *fen=strchr(line,'\t'),*end=NULL;double result=strtod(line,&end);
  if(!fen || end!=fen || (result!=0.0 && result!=0.5 && result!=1.0)) {fprintf(stderr,"invalid label\n");return 2;}
  if(ParseFen(fen+1,&board)!=0 || board.pceNum[wK]!=1 || board.pceNum[bK]!=1) {fprintf(stderr,"invalid FEN\n");return 2;}
  if(count==capacity) {capacity=capacity?capacity*2:1024;Sample *next=realloc(samples,capacity*sizeof *samples);if(!next) return 2;samples=next;}
  Sample *sample=&samples[count++];sample->result=result;sample->side=board.side;
  COPY_FIELD(pieces);COPY_FIELD(pceNum);COPY_FIELD(pList);COPY_FIELD(KingSq);COPY_FIELD(material);COPY_FIELD(pawns);
 }
 fclose(file);if(!count) {fprintf(stderr,"empty dataset\n");return 2;}
 /* One request: sigmoid scale K followed by every integer weight. K=0 dumps scores. */
 while(fgets(line,sizeof line,stdin)) {
  char *end;double k=strtod(line,&end);if(end==line || !isfinite(k) || k<0.0 || k>10.0) return 2;
#ifdef JACE_TUNING
  for(size_t i=0;i<sizeof parameters/sizeof parameters[0];i++) {
   char *start=end;long value=strtol(start,&end,10);
   if(end==start || value<parameters[i].low || value>parameters[i].high) {fprintf(stderr,"invalid parameter %s\n",parameters[i].name);return 2;}
   *parameters[i].value=(int)value;
  }
#endif
  while(*end==' ' || *end=='\n' || *end=='\r') end++;
  if(*end) return 2;
  double error=0;
  for(size_t i=0;i<count;i++) {
   const Sample *sample=&samples[i];board.side=sample->side;
   RESTORE_FIELD(pieces);RESTORE_FIELD(pceNum);RESTORE_FIELD(pList);RESTORE_FIELD(KingSq);RESTORE_FIELD(material);RESTORE_FIELD(pawns);
   int score=EvalPositionForSearch(&board,NULL,TRUE)*(board.side==WHITE?1:-1);
   if(k==0) printf("%d\n",score);
   else {double prediction=1.0/(1.0+pow(10.0,-k*(double)score/400.0));double delta=prediction-sample->result;error+=delta*delta;}
  }
  if(k!=0) printf("%.17g\n",error/(double)count);
  fflush(stdout);
 }
 free(samples);return 0;
}
