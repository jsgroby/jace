from pathlib import Path
import subprocess,threading,queue,time,json,resource,hashlib,statistics,sys
root=Path(__file__).resolve().parents[2];out=root/'build/hash-trywrite-benchmark';out.mkdir(parents=True,exist_ok=True)
exe=root/'build/release/jace'
positions=json.loads((root/'validation/see-profile/positions.json').read_text())
rows=[];blocks=[]
def save():
 (out/'measurements.json').write_text(json.dumps(dict(rows=rows,blocks=blocks),indent=2))
class Engine:
 def __init__(self,n):
  self.before=resource.getrusage(resource.RUSAGE_CHILDREN);self.started=time.monotonic()
  self.p=subprocess.Popen([str(exe),'NoBook'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=(out/'engine-stderr.log').open('a'),text=True,bufsize=1)
  self.q=queue.Queue()
  def read():
   for l in self.p.stdout:self.q.put(l.strip())
   self.q.put(None)
  threading.Thread(target=read,daemon=True).start()
  self.send('uci');assert 'id name Jace 1.5.1' in self.until('uciok')
  for s in [f'Threads value {n}','Book value false','Hash value 64','MultiPV value 1','Ponder value false','SyzygyProbeLimit value 0']:self.send('setoption name '+s)
  self.ready();self.startup=time.monotonic()-self.started
 def send(self,s):self.p.stdin.write(s+'\n');self.p.stdin.flush()
 def until(self,prefix,timeout=15):
  end=time.monotonic()+timeout;ls=[]
  while True:
   l=self.q.get(timeout=max(.01,end-time.monotonic()));assert l is not None,'engine exited';ls.append(l)
   if l.startswith(prefix):return ls
 def ready(self):self.send('isready');self.until('readyok')
 def position(self,fen):self.send('setoption name Clear Hash');self.send('position fen '+fen);self.ready()
 def search(self,command):
  start=time.monotonic();self.send(command);ls=self.until('bestmove ');elapsed=time.monotonic()-start
  final=[l.split() for l in ls if l.startswith('info nodes ')];assert len(final)==1
  depth=max([int(l.split()[l.split().index('depth')+1]) for l in ls if l.startswith('info score ')] or [0])
  return dict(elapsed=elapsed,nodes=int(final[0][2]),depth=depth,move=ls[-1].split()[1])
 def close(self):
  self.send('quit');self.p.wait(timeout=5);assert self.p.returncode==0
  r=resource.getrusage(resource.RUSAGE_CHILDREN)
  return dict(cpu_seconds=r.ru_utime+r.ru_stime-self.before.ru_utime-self.before.ru_stime,maxrss_bytes=r.ru_maxrss,startup_seconds=self.startup)
