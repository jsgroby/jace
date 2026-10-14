from pathlib import Path
import subprocess,json,argparse
parser=argparse.ArgumentParser()
parser.add_argument('--baseline-ref',required=True)
args=parser.parse_args()
p=Path(__file__).resolve().parents[1];out=p/'build/legal-profile';out.mkdir(parents=True,exist_ok=True)
s=subprocess.check_output(['git','show',args.baseline_ref+':movegen.c'],cwd=p,text=True)
s=s.replace(' S_MOVELIST list; GenerateAllMoves(pos,&list);',' requests++; unsigned long long begin=NowNs();\n S_MOVELIST list; GenerateAllMoves(pos,&list); generated+=(unsigned)list.count;')
s=s.replace('for(int i=0;i<list.count;i++) if(MakeMove(pos,list.moves[i].move)) { TakeMove(pos); return TRUE; }','for(int i=0;i<list.count;i++) {tried++;if(MakeMove(pos,list.moves[i].move)) { TakeMove(pos); elapsed+=NowNs()-begin;return TRUE; }}')
s=s[:s.rindex(' return FALSE;')] + s[s.rindex(' return FALSE;'):].replace(' return FALSE;',' elapsed+=NowNs()-begin;return FALSE;',1)
(out/'profile-movegen.c').write_text(s)
h='''#include "defs.h"
#include <time.h>
static unsigned long long requests,generated,tried,elapsed;
static unsigned long long NowNs(void) {struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (unsigned long long)t.tv_sec*1000000000ULL+(unsigned long long)t.tv_nsec;}
static void NoInput(S_SEARCHINFO *i){(void)i;}
#define ReadInput NoInput
#define WaitForSearchSignal NoInput
#include "profile-movegen.c"
#include "search.c"
int main(int argc,char **argv) {
 if(argc!=2)return 1;AllInit();EngineOptions->UseBook=FALSE;
 S_BOARD p={0};S_SEARCHINFO info={0};info.GAME_MODE=UCIMODE;InitHashTable(p.HashTable,64);
 FILE *f=fopen(argv[1],"r");char fen[256];int n=0;
 while(fgets(fen,sizeof fen,f)) {
  if(ParseFen(fen,&p))return 2;ClearHashTable(p.HashTable);
  requests=generated=tried=elapsed=0;info.depth=n>=20?8:6;info.starttime=GetTimeMs();
  printf("POSITION %d\\n",n++);unsigned long long start=NowNs();SearchPosition(&p,&info);
  printf("PROFILE %llu %llu %llu %llu %llu\\n",requests,generated,tried,elapsed,NowNs()-start);
 }
 fclose(f);free(p.HashTable->pTable);CleanPolyBook();return 0;
}'''
(out/'profile.c').write_text(h)
positions=json.loads((p/'validation/see-profile/positions.json').read_text())
(out/'positions.fen').write_text(''.join(x['fen']+'\n' for x in positions))
src=[str(x) for x in p.glob('*.c') if x.name not in ('jace.c','search.c','movegen.c')]
subprocess.run(['cc','-std=c11','-O3','-DNDEBUG','-D_POSIX_C_SOURCE=200809L','-D_DARWIN_C_SOURCE','-I'+str(p),str(out/'profile.c'),*src,'-o',str(out/'profile')],check=True)
r=subprocess.run([str(out/'profile'),str(out/'positions.fen')],capture_output=True,text=True,check=True)
(out/'profile.log').write_text(r.stdout)
rows=[list(map(int,x.split()[1:])) for x in r.stdout.splitlines() if x.startswith('PROFILE ')]
assert len(rows)==len(positions)
summary={}
for cat in ['opening','tactical','endgame','all']:
 ids=[i for i,x in enumerate(positions) if cat=='all' or x['category']==cat]
 a=[sum(rows[i][j] for i in ids) for j in range(5)]
 summary[cat]=dict(zip(['calls','generated','tried','probe_ns','search_ns'],a))
 summary[cat]['instrumented_time_pct']=100*a[3]/a[4]
 summary[cat]['generated_per_call']=a[1]/a[0]
 summary[cat]['tried_per_call']=a[2]/a[0]
(p/'validation/legal-probe/profile.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps(summary,indent=2))
