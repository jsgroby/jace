#!/usr/bin/env python3
"""Independent all-region tests: each adjustment must affect only its own squares."""
import pathlib,re,subprocess,sys,tempfile
root=pathlib.Path(__file__).resolve().parents[2]
bindir=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root/'build/release'
suffix='.exe' if (bindir/'eval-tune.exe').exists() else ''
params=re.findall(r'PARAM\((\w+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\)',(root/'eval_params.def').read_text())
weights=[int(v) if not name.startswith('Pst') else 0 for name,v,lo,hi in params]
positions=[];identities=[]
def fen(pieces):
    rows=[]
    for rank in range(7,-1,-1):
        row='';empty=0
        for file in range(8):
            piece=pieces.get(rank*8+file)
            if piece:
                if empty:row+=str(empty);empty=0
                row+=piece
            else:empty+=1
        if empty:row+=str(empty)
        rows.append(row)
    return '/'.join(rows)+' w - - 0 1'
for kind,letter in [('Knight','N'),('Bishop','B')]:
    for square in range(64):
        rank,file=divmod(square,8)
        if square in (0,8,55,63):continue
        if kind=='Knight' and (7-rank)*(7-file)==2:continue
        if kind=='Bishop' and 7-rank==7-file:continue
        pieces={0:'K',8:'P',55:'p',63:'k',square:letter}
        name=f'Pst{kind}R{rank//2}F{min(file,7-file)}'
        positions.append(fen(pieces));identities.append((name,1))
        positions.append(fen({sq^56:pc.swapcase() for sq,pc in pieces.items()}));identities.append((name,-1))
with tempfile.TemporaryDirectory() as tmp:
    path=pathlib.Path(tmp)/'positions.tsv';path.write_text(''.join('0.5\t'+f+'\n' for f in positions))
    engine=subprocess.Popen([str(bindir/('eval-tune'+suffix)),str(path)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
    def scores(w):
        engine.stdin.write(' '.join(map(str,[0,*w]))+'\n');engine.stdin.flush()
        return [int(engine.stdout.readline()) for _ in positions]
    base=scores(weights);count=0
    compiled=list(map(int,subprocess.check_output([str(bindir/('eval-fixed'+suffix)),str(path)],input='0\n',text=True).splitlines()))
    defaults={n:int(v) for n,v,lo,hi in params}
    assert compiled==[v+sign*defaults[group] for v,(group,sign) in zip(base,identities)], 'Compiled table differs from independent regional mapping'
    for i,(name,value,low,high) in enumerate(params):
        if not name.startswith('Pst') or int(high)==0:continue
        trial=weights.copy();trial[i]=7;actual=scores(trial)
        expected=[v+(sign*7 if group==name else 0) for v,(group,sign) in zip(base,identities)]
        assert actual==expected,name
        assert any(group==name for group,sign in identities),name
        count+=1
    engine.stdin.close();assert engine.wait()==0
print(f'PASS: {count} placement adjustments across {len(positions)} legal square/color cases; exact independent score deltas')
