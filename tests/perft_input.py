"""Check the interactive perft command's invalid-input and EOF paths."""
import pathlib, subprocess, sys
exe=str(pathlib.Path(sys.argv[1]).resolve())
fen='rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'
def run(commands):
    p=subprocess.run([exe],input=commands,text=True,capture_output=True,timeout=10)
    assert p.returncode==0,(p.returncode,p.stderr)
    return p.stdout,p.stderr
out,err=run('perft\n'+fen+'\n2\nquit\n')
assert '400 nodes visited' in out,(out,err)
for depth in ['abc','2junk','0','-1','64','99999999999999999999999999999']:
    out,err=run('perft\n'+fen+'\n'+depth+'\nuci\nquit\n')
    assert 'Invalid perft depth' in err and 'uciok' in out,(depth,out,err)
out,err=run('perft\ninvalid\nuci\nquit\n')
assert 'Invalid FEN' in err and 'uciok' in out
run('perft\n');run('perft\n'+fen+'\n')
print('PERFT INPUT TESTS PASS')
