import os,pathlib,subprocess,sys,time,select,re
env=dict(os.environ)
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
baseline=pathlib.Path(sys.argv[1]).resolve()
fens=re.findall(r'^"(.*)",$',(root/'tests/qsearch_positions.inc').read_text(),re.M)
def run(path,new):
 e=Engine(str(path));e.send('uci')
 if new:
  assert e.until('id name ')=='id name Jace 1.3'
  assert e.until('id author ')=='id author Jeffrey S. Groby'
 e.until('uciok');e.send('setoption name Book value false')
 results=[]
 for fen in fens:
  e.send('ucinewgame');e.send('position fen '+fen);e.send('go depth 5')
  rows=[]
  while True:
   line=e.until('')
   if line.startswith('bestmove '):
    results.append((rows,line));break
   if not line.startswith('info score '):continue
   tokens=line.split();depth=int(tokens[tokens.index('depth')+1]);nodes=int(tokens[tokens.index('nodes')+1])
   if new:
    sd=int(tokens[tokens.index('seldepth')+1]);nps=int(tokens[tokens.index('nps')+1]);hf=int(tokens[tokens.index('hashfull')+1]);ms=int(tokens[tokens.index('time')+1])
    assert 0<=sd<64 and 0<=hf<=1000 and ms>=0 and nps>=0
    assert abs(nps-nodes*1000/max(1,ms))<=1
   rows.append((tokens[2],tokens[3],depth,nodes,tokens[tokens.index('pv')+1:]))
 e.close();return results
old=run(baseline,False);new=run(root/'jace-baseline',True)
assert old==new,'Fixed-depth scores, nodes, PV or best move changed'
print(f'UCI reporting validated; identical scores, nodes, PV and best moves across {len(fens)} positions at depth 5.')
