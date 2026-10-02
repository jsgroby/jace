import os,pathlib,subprocess,sys,time,select,re
env=dict(os.environ,UBSAN_OPTIONS="halt_on_error=1",ASAN_OPTIONS="detect_leaks=0")
class Engine:
 def __init__(self,path,cwd=None):
  self.p=subprocess.Popen([path,'NoBook'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env,cwd=cwd)
  self.buffer=b''
 def send(self,s):
  self.p.stdin.write((s+'\n').encode());self.p.stdin.flush()
 def until(self,prefix,timeout=15):
  end=time.monotonic()+timeout
  while time.monotonic()<end:
   while b'\n' in self.buffer:
    line,self.buffer=self.buffer.split(b'\n',1)
    text=line.decode('ascii')
    if text.startswith(prefix):return text
   if select.select([self.p.stdout],[],[],max(0,end-time.monotonic()))[0]:
    data=os.read(self.p.stdout.fileno(),65536)
    if not data:raise AssertionError(self.p.stderr.read().decode())
    self.buffer+=data
  raise AssertionError('timeout waiting for '+prefix)
 def close(self):
  self.send('quit');self.p.wait(timeout=3)
  err=self.p.stderr.read().decode()
  assert self.p.returncode==0,err
  assert 'runtime error:' not in err and 'AddressSanitizer' not in err,err


root=pathlib.Path(__file__).resolve().parents[1]
def finish(e):
 lines=[]
 while True:
  line=e.until('')
  if line.startswith('bestmove '):return lines,line
  lines.append(line)
def silence(e,seconds=.12):
 end=time.monotonic()+seconds
 while time.monotonic()<end:
  while b'\n' in e.buffer:
   line,e.buffer=e.buffer.split(b'\n',1)
   assert not line.startswith(b'bestmove '),line
  if select.select([e.p.stdout],[],[],max(0,end-time.monotonic()))[0]:
   data=os.read(e.p.stdout.fileno(),65536);assert data;e.buffer+=data
 # Check data read at the end of the interval, too.
 assert b'bestmove ' not in e.buffer

def position(e,fen=None):
 e.send('position '+('fen '+fen if fen else 'startpos'))
def search(e,command,fen=None):
 position(e,fen);e.send(command);return finish(e)
def pvrows(lines):return [l.split() for l in lines if l.startswith('info score ')]
def check_pv(e,fen,pv):
 e.send('position '+('fen '+fen if fen else 'startpos')+' moves '+' '.join(pv));e.send('isready')
 while True:
  line=e.until('');assert 'invalid' not in line and 'illegal' not in line,line
  if line=='readyok':break

for mode in ('release','sanitize'):
 e=Engine(str(root/'build'/mode/'jace'));e.send('uci')
 options=[]
 while True:
  line=e.until('');options.append(line)
  if line=='uciok':break
 assert 'option name MultiPV type spin default 1 min 1 max 16' in options
 assert 'option name Ponder type check default false' in options
 # Even with Book enabled, root restrictions take precedence.
 lines,best=search(e,'go searchmoves e2e4 g1f3 e2e4 depth 3')
 assert best.split()[1] in ('e2e4','g1f3')
 assert all(t[t.index('pv')+1] in ('e2e4','g1f3') for t in pvrows(lines))
 _,best=search(e,'go nodes 1 searchmoves h2h3');assert best=='bestmove h2h3'
 _,best=search(e,'go depth 2 searchmoves e2e5');assert best=='bestmove 0000'
 _,best=search(e,'go searchmoves depth 2');assert best=='bestmove 0000'
 _,best=search(e,'go depth 2 searchmoves e2e4x');assert best=='bestmove 0000'
 _,best=search(e,'  go\tdepth  2\tsearchmoves h2h3  ');assert best=='bestmove h2h3'
 cases=[('4k3/8/8/8/8/8/8/4K2R w K - 0 1','e1g1'),
        ('7k/P7/8/8/8/8/8/K7 w - - 0 1','a7a8n'),
        ('7k/8/8/3pP3/8/8/K7/8 w - d6 0 1','e5d6')]
 for fen,move in cases:
  _,best=search(e,'go depth 2 searchmoves '+move,fen);assert best=='bestmove '+move
 e.send('setoption name Book value false');e.send('setoption name MultiPV value 3')
 lines,best=search(e,'go depth 3')
 rows=pvrows(lines)
 for depth in (1,2,3):
  group=[t for t in rows if int(t[t.index('depth')+1])==depth]
  assert len(group)==3,group
  assert [int(t[t.index('multipv')+1]) for t in group]==[1,2,3]
  assert len({t[t.index('pv')+1] for t in group})==3
  assert [int(t[3]) for t in group]==sorted([int(t[3]) for t in group],reverse=True)
  for t in group:check_pv(e,None,t[t.index('pv')+1:])
 assert best.split()[1]==group[0][group[0].index('pv')+1]
 e.send('setoption name MultiPV value 99')
 lines,best=search(e,'go depth 2 searchmoves d2d4 e2e4')
 assert len(pvrows(lines))==4
 assert {t[t.index('pv')+1] for t in pvrows(lines)}=={'d2d4','e2e4'}
 lines,best=search(e,'go depth 2 searchmoves h2h3');assert best=='bestmove h2h3'
 assert all(t[t.index('multipv')+1]=='1' for t in pvrows(lines))
 e.send('setoption name MultiPV value 1');e.send('setoption name Ponder value true')
 lines,best=search(e,'go depth 3');tokens=best.split();assert tokens[2]=='ponder'
 check_pv(e,None,[tokens[1],tokens[3]])
 # Completed ponder searches remain silent until ponderhit or stop.
 position(e);e.send('go ponder depth 2');e.until('info score ');silence(e)
 e.send('isready');e.until('readyok');silence(e,.03)
 e.send('  ponderhit  ');_,best=finish(e);assert best.startswith('bestmove ')
 # Ponder time is not charged against the post-hit move budget.
 position(e);e.send('go ponder movetime 80');silence(e,.20)
 hit=time.monotonic();e.send('ponderhit');finish(e)
 assert .025<time.monotonic()-hit<3
 position(e);e.send('go ponder nodes 1');silence(e)
 e.send('stop');finish(e)
 # Mate/terminal positions still wait in ponder and infinite modes.
 terminal='7k/6Q1/5K2/8/8/8/8/8 b - - 0 1'
 position(e,terminal);e.send('go ponder depth 1');silence(e)
 e.send('ponderhit');_,best=finish(e);assert best=='bestmove 0000'
 position(e);e.send('go infinite depth 1');silence(e)
 e.send('stop');finish(e)
 # Batched commands are preserved at the ponder transition.
 position(e);e.send('go ponder depth 1');silence(e,.05)
 e.send('ponderhit\nstop\nisready')
 got_best=got_ready=False
 while not (got_best and got_ready):
  line=e.until('');got_best|=line.startswith('bestmove ');got_ready|=line=='readyok'
 # Interrupt an unfinished MultiPV search: keep a legal permitted fallback.
 e.send('setoption name MultiPV value 3');position(e)
 e.send('go infinite searchmoves d2d4 e2e4 g1f3');e.send('stop')
 _,best=finish(e);assert best.split()[1] in ('d2d4','e2e4','g1f3')
 e.send('setoption name MultiPV value 1')
 # A later normal search does not retain root filters or ponder state.
 e.send('setoption name Ponder value false')
 _,best=search(e,'go depth 2');assert len(best.split())==2 and best!='bestmove 0000'
 e.close()
 e=Engine(str(root/'build'/mode/'jace'));e.send('uci');e.until('uciok');position(e)
 e.send('go ponder depth 1');silence(e,.05);e.p.stdin.close();e.p.wait(timeout=3)
 assert e.p.returncode==0
 err=e.p.stderr.read().decode();assert 'runtime error:' not in err and 'AddressSanitizer' not in err
 print(mode,'SEARCHMOVES / MULTIPV / PONDER TESTS PASS')
