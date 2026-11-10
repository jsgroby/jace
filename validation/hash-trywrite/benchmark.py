from pathlib import Path
exec(compile(Path(__file__).with_name('common.py').read_text(),str(Path(__file__).with_name('common.py')),'exec'))
import statistics
import argparse
parser=argparse.ArgumentParser()
parser.add_argument('--baseline',required=True)
parser.add_argument('--candidate',default=str(root/'build/release/jace'))
args=parser.parse_args()
binaries={'baseline':Path(args.baseline).resolve(),'candidate':Path(args.candidate).resolve()}
ids=[0,3,7,11,12,13,20,22,23,24,25,26]
for rep in range(5):
 for label in (['baseline','candidate'] if rep%2==0 else ['candidate','baseline']):
  exe=binaries[label];e=Engine(4)
  for i in ids:
   e.position(positions[i]['fen']);r=e.search('go movetime 1500');r.update(label=label,rep=rep,position=i,category=positions[i]['category']);rows.append(r)
  e.close();save();print(rep,label,flush=True)
summary={}
for cat in ['opening','tactical','endgame','all']:
 sums={}
 for label in binaries:
  sums[label]=[sum(r['nodes'] for r in rows if r['label']==label and r['rep']==rep and (cat=='all' or r['category']==cat))/sum(r['elapsed'] for r in rows if r['label']==label and r['rep']==rep and (cat=='all' or r['category']==cat)) for rep in range(5)]
 summary[cat]={'nps':sums,'median_gain_pct':100*(statistics.median(sums['candidate'])/statistics.median(sums['baseline'])-1)}
(out/'benchmark-summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))
