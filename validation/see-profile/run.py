"""Reproducible SEE operation profile. Run on macOS/Linux from any directory."""
from pathlib import Path
import subprocess, json, re, statistics, sys
r=Path(__file__).resolve().parent
src=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else next((p for p in r.parents if (p/'defs.h').exists()), r.parents[1]/'jace-baseline')
positions=json.loads((r/'positions.json').read_text())
(r/'positions.json').write_text(json.dumps(positions,indent=2))
(r/'positions.fen').write_text(''.join(p['fen']+'\n' for p in positions))
search=(src/'search.c').read_text().replace('info->nodes++;','info->nodes++;nodeId[pos->ply]=info->nodes;')
(r/'search-profile.inc').write_text(search)
see=(src/'see.c').read_text().replace('int SEE(', 'int RawSEE(')
see=see.replace('/* SEE uses', 'unsigned long long seeOps[9];\n/* SEE uses')
see=see.replace('int p=b->pieces[from],delta=to-from;', 'seeOps[0]++;int p=b->pieces[from],delta=to-from;')
see=see.replace('if(sq==to) return TRUE;', 'seeOps[1]++;if(sq==to) return TRUE;')
see=see.replace('static int Attacked(const ExchangeBoard *b,int sq,int side) {', 'static int Attacked(const ExchangeBoard *b,int sq,int side) {\n seeOps[2]++;')
see=see.replace('{int from=SQ120(i),p=b->pieces[from];', '{seeOps[3]++;int from=SQ120(i),p=b->pieces[from];')
see=see.replace('if(depth>=30', 'seeOps[4]++;if((unsigned long long)depth>seeOps[8]) seeOps[8]=(unsigned long long)depth;\n if(depth>=30')
see=see.replace('int from=SQ120(i),p=b->pieces[from];\n', 'seeOps[5]++;int from=SQ120(i),p=b->pieces[from];\n')
see=see.replace('int legal=!Attacked', 'seeOps[6]++;int legal=!Attacked')
see=see.replace('return 0; /* Legality is enforced by MakeMove. */', '{seeOps[7]++;return 0;} /* Legality is enforced by MakeMove. */')
(r/'raw-see.c').write_text(see)
bench=(r/'bench-template.c').read_text().replace('static unsigned long long calls,duplicates,seeNs;', 'extern unsigned long long seeOps[9];\nstatic unsigned long long calls,duplicates,seeNs;')
bench=bench.replace('calls=duplicates=seeNs=0;', 'memset(seeOps,0,sizeof seeOps);calls=duplicates=seeNs=0;')
bench=bench.replace('printf("POSITION %d\\n",n++);', 'info.depth=n>=20?8:6;printf("POSITION %d\\n",n++);')
bench=bench.replace('printf("PROFILE %llu %llu %llu\\n",calls,duplicates,seeNs);', 'printf("PROFILE %llu %llu %llu\\n",calls,duplicates,seeNs);printf("OPS");for(int j=0;j<9;j++) printf(" %llu",seeOps[j]);printf("\\n");')
(r/'bench.c').write_text(bench)
base=['cc','-std=c11','-O3','-DNDEBUG','-D_POSIX_C_SOURCE=200809L','-D_DARWIN_C_SOURCE','-I'+str(src),'-I'+str(r),str(r/'bench.c')]
for mode in ['bench','profile']:
 sources=[str(p) for p in src.glob('*.c') if p.name not in ['jace.c','search.c'] and not (mode=='profile' and p.name=='see.c')]
 args=base+sources+(['-DPROFILE',str(r/'raw-see.c')] if mode=='profile' else [])+['-o',str(r/mode)]
 subprocess.run(args,check=True,capture_output=True)
 for rep in range(1 if mode=='profile' else 3):
  with (r/f'{mode}-{rep}.log').open('w') as log:
   subprocess.run([str(r/mode),str(r/'positions.fen')],stdout=log,check=True,timeout=180)
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
   if line.startswith('PROFILE '):row['calls'],row['duplicates'],row['see_ns']=map(int,t[1:])
   if line.startswith('OPS '):row['ops']=list(map(int,t[1:]))
  rows.append(row)
 return rows
prof=parse(r/'profile-0.log'); benches=[parse(r/f'bench-{i}.log') for i in range(3)]
for rows in benches:
 assert len(rows)==len(prof)==28
 for a,b in zip(rows,prof):assert (a['search'],a['bestmove'])==(b['search'],b['bestmove'])
summary={}
for cat in ['opening','tactical','endgame','all']:
 ids=[i for i,p in enumerate(positions) if cat=='all' or p['category']==cat]
 summary[cat]={'positions':len(ids),'nodes':sum(prof[i]['nodes'] for i in ids),'calls':sum(prof[i]['calls'] for i in ids),'duplicates':sum(prof[i]['duplicates'] for i in ids),'see_ms':sum(prof[i]['see_ns'] for i in ids)/1e6,'profile_ms':sum(prof[i]['ms'] for i in ids),'median_unprofiled_ms':statistics.median(sum(b[i]['ms'] for i in ids) for b in benches),'ops':[sum(prof[i]['ops'][j] for i in ids) for j in range(8)]+[max(prof[i]['ops'][8] for i in ids)]}
(r/'summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))
