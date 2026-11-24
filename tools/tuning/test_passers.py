#!/usr/bin/env python3
"""Exact phase/rank/color tests, including rear doubled and blocked non-passers."""
import pathlib,re,subprocess,sys,tempfile
root=pathlib.Path(__file__).resolve().parents[2]
bindir=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root/'build/release'
suffix='.exe' if (bindir/'eval-tune.exe').exists() else ''
params=re.findall(r'PARAM\((\w+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\)',(root/'eval_params.def').read_text())
weights=[int(v) for n,v,lo,hi in params];fens=[];tags=[]
def fen(pieces):
    rows=[]
    for rank in range(7,-1,-1):
        row='';empty=0
        for file in range(8):
            pc=pieces.get(rank*8+file)
            if pc:
                if empty:row+=str(empty);empty=0
                row+=pc
            else:empty+=1
        if empty:row+=str(empty)
        rows.append(row)
    return '/'.join(rows)+' w - - 0 1'
for phase in ('MG','EG'):
    for rank in range(2,8):
        for kind in ('front','doubled','blocked'):
            if kind=='doubled' and rank==2:continue
            if kind=='blocked' and rank==7:continue
            if phase=='MG':
                pieces={i:pc for i,pc in enumerate('RNBQKBNR')}
                pieces.update({56+i:pc for i,pc in enumerate('rnbqkbnr')})
                pieces.update({8+i:'P' for i in range(8) if i!=4})
                pieces.update({48+i:'p' for i in (0,1,2,6,7)})
            else:pieces={0:'K',63:'k'}
            pieces[(rank-1)*8+4]='P'
            if kind=='doubled':
                pieces[12]='P'
                if phase=='MG':pieces.pop(8)
            if kind=='blocked':pieces[rank*8+4]='p'
            for mirrored in (False,True):
                board={sq^56:pc.swapcase() for sq,pc in pieces.items()} if mirrored else pieces
                fens.append(fen(board));tags.append((phase,rank,kind,-1 if mirrored else 1))
with tempfile.TemporaryDirectory() as tmp:
    path=pathlib.Path(tmp)/'positions.tsv';path.write_text(''.join('0.5\t'+f+'\n' for f in fens))
    engine=subprocess.Popen([str(bindir/('eval-tune'+suffix)),str(path)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
    def scores(w):
        engine.stdin.write(' '.join(map(str,[0,*w]))+'\n');engine.stdin.flush()
        return [int(engine.stdout.readline()) for _ in fens]
    base=scores(weights)
    fixed=list(map(int,subprocess.check_output([str(bindir/('eval-fixed'+suffix)),str(path)],input='0\n',text=True).splitlines()))
    assert base==fixed
    count=0
    for i,(name,value,low,high) in enumerate(params):
        if not name.startswith('Pass'):continue
        phase=name[4:6];rank=int(name[6:]);trial=weights.copy()
        change=min(7,int(high)-trial[i]) or -min(7,trial[i]-int(low));trial[i]+=change
        actual=scores(trial)
        expected=[v+(sign*change if p==phase and r==rank and kind!='blocked' else 0) for v,(p,r,kind,sign) in zip(base,tags)]
        assert actual==expected,(name,[(tag,a,e) for tag,a,e in zip(tags,actual,expected) if a!=e])
        count+=1
    engine.stdin.close();assert engine.wait()==0
print(f'PASS: {count} phase/rank bonuses across {len(fens)} exact score cases; colors, front/rear doubled pawns and blocked non-passers')
