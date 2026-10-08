"""Compare current search with a baseline revision; other engine files must match.
Run from the repository: python3 tests/ordering_benchmark.py --baseline-ref <sha>
Counters measure SEE calls; timings include their identical overhead in both builds.
"""
import argparse
import json
import os
from pathlib import Path
import statistics
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--baseline-ref', required=True)
parser.add_argument('--repeats', type=int, default=5)
args = parser.parse_args()
assert args.repeats > 0
src = Path(__file__).resolve().parents[1]
out = src/'build/ordering-benchmark'
out.mkdir(parents=True, exist_ok=True)
positions = json.loads((src/'validation/see-profile/positions.json').read_text())
(out/'positions.fen').write_text(''.join(p['fen']+'\n' for p in positions))
baseline = subprocess.check_output(['git', 'show', args.baseline_ref+':search.c'], cwd=src, text=True)
(out/'baseline-search.c').write_text(baseline)
(out/'candidate-search.c').write_text((src/'search.c').read_text())
harness = r'''
#include "defs.h"
#include <stdio.h>
static unsigned long long calls;
static int CountSEE(const S_BOARD *p,int m) {calls++;return SEE(p,m);}
static void NoInput(S_SEARCHINFO *i) {(void)i;}
#define ReadInput NoInput
#define WaitForSearchSignal NoInput
#define SEE CountSEE
#include "SEARCH_FILE"
int main(int argc,char **argv) {
 if(argc!=2)return 1;
 AllInit();EngineOptions->UseBook=FALSE;EngineOptions->TaperedEval=TRUE;
 EngineOptions->MultiPV=1;EngineOptions->Ponder=FALSE;
 S_BOARD p={0};S_SEARCHINFO info={0};info.GAME_MODE=UCIMODE;
 InitHashTable(p.HashTable,64);FILE *f=fopen(argv[1],"r");if(!f)return 2;
 char fen[256];int n=0;
 while(fgets(fen,sizeof fen,f)) {
  if(ParseFen(fen,&p))return 3;
  ClearHashTable(p.HashTable);calls=0;info.depth=n>=20?8:6;
  printf("POSITION %d\n",n++);info.starttime=GetTimeMs();SearchPosition(&p,&info);
  printf("SEE_CALLS %llu\n",calls);
 }
 fclose(f);free(p.HashTable->pTable);CleanPolyBook();return 0;
}
'''
common = [os.environ.get('CC', 'cc'), '-std=c11', '-O3', '-DNDEBUG',
          '-D_POSIX_C_SOURCE=200809L', '-D_DARWIN_C_SOURCE', '-I'+str(src)]
sources = [str(p) for p in src.glob('*.c') if p.name not in ('jace.c', 'search.c')]
for label in ('baseline', 'candidate'):
    h = out/(label+'-bench.c')
    h.write_text(harness.replace('SEARCH_FILE', label+'-search.c'))
    subprocess.run(common+[str(h)]+sources+['-o', str(out/(label+'-bench'))], check=True)
for rep in range(args.repeats):
    for label in (('baseline','candidate') if rep%2==0 else ('candidate','baseline')):
        with (out/f'{label}-{rep}.log').open('w') as log:
            subprocess.run([str(out/(label+'-bench')), str(out/'positions.fen')], stdout=log, check=True)

def parse(path):
    rows = []
    for chunk in path.read_text().split('POSITION ')[1:]:
        row = {'search': []}
        for line in chunk.splitlines()[1:]:
            t = line.split()
            if line.startswith('info score '):
                row['search'].append((t[2], t[3], t[t.index('depth')+1], t[t.index('nodes')+1], t[t.index('pv')+1:]))
                row['ms'] = int(t[t.index('time')+1])
                row['nodes'] = int(t[t.index('nodes')+1])
            elif line.startswith('bestmove '): row['bestmove'] = line
            elif line.startswith('SEE_CALLS '): row['see_calls'] = int(t[1])
        rows.append(row)
    return rows
runs = {label: [parse(out/f'{label}-{i}.log') for i in range(args.repeats)] for label in ('baseline','candidate')}
for label, trials in runs.items():
    for trial in trials:
        assert len(trial) == len(positions)
        for i, (a,b) in enumerate(zip(runs['baseline'][0], trial)):
            assert (a['search'],a['bestmove']) == (b['search'],b['bestmove']), (label,i,a,b)
summary = {}
for category in ('opening','tactical','endgame','all'):
    ids = [i for i,p in enumerate(positions) if category=='all' or p['category']==category]
    d = {label: {'milliseconds': [sum(run[i]['ms'] for i in ids) for run in trials],
                 'see_calls': sum(trials[0][i]['see_calls'] for i in ids)} for label,trials in runs.items()}
    d['time_reduction_pct'] = 100*(1-statistics.median(d['candidate']['milliseconds'])/statistics.median(d['baseline']['milliseconds']))
    d['see_reduction_pct'] = 100*(1-d['candidate']['see_calls']/d['baseline']['see_calls'])
    d['nodes'] = sum(runs['baseline'][0][i]['nodes'] for i in ids)
    summary[category] = d
(out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
print('PASS: all iterations match scores, nodes, PVs and best moves')
print(json.dumps(summary,indent=2))
