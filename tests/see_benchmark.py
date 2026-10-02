from pathlib import Path
import subprocess,json,statistics
src=Path(__file__).resolve().parents[1];r=src/'build/see-benchmark';r.mkdir(parents=True,exist_ok=True)
positions=json.loads((src/'validation/see-profile/positions.json').read_text())
(r/'positions.fen').write_text(''.join(p['fen']+'\n' for p in positions))
harness=(src/'validation/see-profile/bench-template.c').read_text().replace('printf("POSITION %d\\n",n++);','info.depth=n>=20?8:6;printf("POSITION %d\\n",n++);')
(r/'bench.c').write_text(harness)
(r/'baseline-see.c').write_text((src/'tests/see_reference.inc').read_text())
common=['clang','-std=c11','-O3','-DNDEBUG','-D_POSIX_C_SOURCE=200809L','-D_DARWIN_C_SOURCE','-I'+str(src),str(r/'bench.c')]
sources=[str(p) for p in src.glob('*.c') if p.name not in ['jace.c','search.c','see.c']]
for label in ['baseline','candidate']:
 see=r/'baseline-see.c' if label=='baseline' else src/'see.c'
 subprocess.run(common+sources+[str(see),'-o',str(r/(label+'-bench'))],check=True)
for rep in range(5):
 for label in (['baseline','candidate'] if rep%2==0 else ['candidate','baseline']):
  with (r/f'{label}-{rep}.log').open('w') as log:subprocess.run([str(r/(label+'-bench')),str(r/'positions.fen')],stdout=log,check=True)
def parse(path):
 rows=[]
 for chunk in path.read_text().split('POSITION ')[1:]:
  row={'search':[]}
  for line in chunk.splitlines()[1:]:
   t=line.split()
   if line.startswith('info score '):
    row['search'].append((t[2],t[3],t[t.index('depth')+1],t[t.index('nodes')+1],t[t.index('pv')+1:]))
    row['ms']=int(t[t.index('time')+1]);row['nodes']=int(t[t.index('nodes')+1])
   if line.startswith('bestmove '):row['bestmove']=line
  rows.append(row)
 return rows
runs={label:[parse(r/f'{label}-{i}.log') for i in range(5)] for label in ['baseline','candidate']}
for label in runs:
 for run in runs[label]:
  assert len(run)==28
  for a,b in zip(runs['baseline'][0],run):assert (a['search'],a['bestmove'])==(b['search'],b['bestmove'])
summary={}
for cat in ['opening','tactical','endgame','all']:
 ids=[i for i,p in enumerate(positions) if cat=='all' or p['category']==cat]
 d={label:[sum(run[i]['ms'] for i in ids) for run in runs[label]] for label in runs}
 d['reduction_pct']=100*(1-statistics.median(d['candidate'])/statistics.median(d['baseline']))
 d['nodes']=sum(runs['baseline'][0][i]['nodes'] for i in ids);summary[cat]=d
(r/'summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))
