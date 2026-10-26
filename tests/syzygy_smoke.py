"""Portable Syzygy protocol/integration checks. Optional real-table directory."""
import pathlib,queue,subprocess,sys,threading,time
exe=pathlib.Path(sys.argv[1]).resolve()
tables=pathlib.Path(sys.argv[2]).resolve() if len(sys.argv)>2 else pathlib.Path(__file__).resolve().parent/'syzygy-data'
p=subprocess.Popen([str(exe)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
q=queue.Queue();errors=[]
def reader():
 for l in p.stdout:q.put(l.strip())
 q.put(None)
threading.Thread(target=reader,daemon=True).start()
threading.Thread(target=lambda:errors.append(p.stderr.read()),daemon=True).start()
def send(s):p.stdin.write(s+'\n');p.stdin.flush()
def until(prefix):
 end=time.monotonic()+20;seen=[]
 while True:
  l=q.get(timeout=max(.001,end-time.monotonic()));assert l is not None,errors;seen.append(l)
  if l.startswith(prefix):return seen
  assert time.monotonic()<end,seen

def ready():send('isready');until('readyok')
def option(name,value):send('setoption name '+name+' value '+str(value));ready()
def search(fen,go='go depth 3'):
 send('position fen '+fen);send(go);lines=until('bestmove ');return lines,lines[-1].split()[1]
def hits(lines):return max([int(l.split()[l.split().index('tbhits')+1]) for l in lines if ' tbhits ' in l] or [0])
def root(lines):return any('depth 0 ' in l and 'tbhits 1' in l for l in lines)
fen='7k/8/8/8/8/8/8/KQ6 w - - 0 1'
try:
 send('uci');lines=until('uciok');assert any('SyzygyPath' in l for l in lines);assert any('SyzygyProbeLimit' in l for l in lines)
 option('Book','false');option('SyzygyPath',str(tables/'missing-directory'))
 lines,move=search(fen);assert move!='0000' and hits(lines)==0
 option('SyzygyPath','<empty>');lines,move=search(fen);assert hits(lines)==0
 if (tables/'KQvK.rtbw').exists():
  option('SyzygyPath',tables)
  # No castling rights may be passed to Fathom.
  lines,move=search('4k3/8/8/8/8/8/8/R3K3 w Q - 0 1','go depth 1');assert not root(lines)
  option('SyzygyProbeLimit',0);lines,move=search(fen);assert hits(lines)==0
  option('SyzygyProbeLimit',5)
  # Four-piece root lacks a file; capturing the pawn reaches a WDL-covered node.
  lines,move=search('7k/8/8/3p4/8/8/K7/3R4 w - - 0 1','go depth 3');assert hits(lines)>0,lines
  if (tables/'KQvK.rtbz').exists():
   lines,move=search(fen);assert root(lines) and move!='0000',lines
   assert any('score cp 20000 ' in l for l in lines)
   option('MultiPV',3);lines,move=search(fen)
   rows=[l for l in lines if l.startswith('info score ')];assert len(rows)==3 and len({l.split()[-1] for l in rows})==3
   option('MultiPV',1)
   lines,move=search(fen,'go depth 3 searchmoves b1b2');assert root(lines) and move=='b1b2'
   # At halfmove 99 a non-zeroing, non-mating move is a draw; mate takes priority.
   mate='k7/2Q5/2K5/8/8/8/8/8 w - - 99 1'
   lines,move=search(mate,'go depth 3 searchmoves c7d7');assert root(lines) and move=='c7d7' and any('score cp 0 ' in l for l in lines),lines
   lines,move=search(mate,'go depth 3 searchmoves c7b7');assert root(lines) and move=='c7b7' and any('score cp 20000 ' in l for l in lines),lines
   # Correct promotion mapping includes underpromotions.
   promo='7k/P7/8/8/8/8/8/7K w - - 0 1'
   for m in ('a7a8q','a7a8r','a7a8b','a7a8n'):
    lines,move=search(promo,'go depth 3 searchmoves '+m);assert root(lines) and move==m,lines
   # A tablebase result must still wait for ponderhit / stop in infinite mode.
   for command,finish in [('go ponder wtime 1000 btime 1000','ponderhit'),('go infinite','stop')]:
    send('position fen '+fen);send(command);until('info score ');send('isready');seen=until('readyok');assert not any(l.startswith('bestmove') for l in seen)
    time.sleep(.05)
    while not q.empty():assert not q.get().startswith('bestmove')
    send(finish);until('bestmove ')
   print('PASS: real WDL/DTZ, root restrictions, MultiPV, fifty-move/mate precedence, promotions, ponder/infinite')
  else:print('DTZ tests NOT RUN: no KQvK.rtbz; WDL fallback/search tests passed')
 else:print('Real-table tests NOT RUN: supply a fixture directory; missing/disabled tests passed')
 option('SyzygyPath','<empty>');lines,move=search(fen);assert hits(lines)==0
 send('quit');p.wait(timeout=5);assert p.returncode==0,errors
 assert not any('runtime error:' in x or 'AddressSanitizer' in x for x in errors),errors
 print('SYZYGY SMOKE PASS')
finally:
 if p.poll() is None:p.kill();p.wait()
