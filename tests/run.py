#!/usr/bin/env python3
"""Build isolated sanitizer tests and exercise the actual UCI process."""
import sys

# Stop before importing Unix test helpers or invoking any build commands.
if sys.platform.startswith(("win32", "cygwin", "msys")):
 print("Full regression suite NOT RUN: make test is supported on macOS and Linux.")
 print("It requires sanitizer runtimes and Unix pipe handling unavailable in this Windows setup.")
 print("On Windows ARM64, use MSYS2 CLANGARM64 and run:")
 print("  make CC=clang LDFLAGS=-static PYTHON=python smoke")
 print("Run make test on macOS or Ubuntu for full regression coverage.")
 sys.exit(2)

import os, pathlib, subprocess, time, select, struct, tempfile, re
root=pathlib.Path(__file__).resolve().parents[1]
os.chdir(root)
subprocess.run(['make','release'],check=True)
subprocess.run(['make','debug-sanitize'],check=True)
sources=[str(p) for p in root.glob('*.c') if p.name not in ('jace.c','search.c')]
cmd=['cc','-std=c11','-D_POSIX_C_SOURCE=200809L','-D_DARWIN_C_SOURCE','-DDEBUG','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I.','tests/core.c',*sources,'-o','build/sanitize/core-tests']
subprocess.run(cmd,check=True)
env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1',ASAN_OPTIONS='detect_leaks=0')
subprocess.run(['build/sanitize/core-tests'],check=True,env=env,timeout=120)
class Engine:
 def __init__(self,path,cwd=None):
  self.p=subprocess.Popen([path,'NoBook'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env,cwd=cwd)
  self.buffer=b''
 def send(self,s):
  self.p.stdin.write((s+'\n').encode());self.p.stdin.flush()
 def until(self,prefix,timeout=15,contains=False):
  end=time.monotonic()+timeout
  while time.monotonic()<end:
   while b'\n' in self.buffer:
    line,self.buffer=self.buffer.split(b'\n',1)
    text=line.decode('ascii')
    if (prefix in text if contains else text.startswith(prefix)):return text
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
for path in ('build/release/jace','build/sanitize/jace'):
 e=Engine(path);e.send('uci');e.until('uciok')
 e.send('setoption name Book value false')
 e.send('position startpos');e.send('go depth 4');line=e.until('bestmove ');assert line!='bestmove 0000'
 e.send('setoption name TaperedEval value false');e.send('position startpos');e.send('go depth 1');classical=e.until('info score cp ');e.until('bestmove ')
 e.send('setoption name TaperedEval value true');e.send('position startpos');e.send('go depth 1');tapered=e.until('info score cp ');e.until('bestmove ')
 assert classical.split()[3]!=tapered.split()[3]
 e.send('position startpos');e.send('go nodes 1');assert e.until('bestmove ')!='bestmove 0000'
 e.send('position startpos');e.send('go movetime 1');line=e.until('bestmove ');assert len(line.split()[1]) in (4,5) and line!='bestmove 0000'
 for fen in ('7k/6Q1/5K2/8/8/8/8/8 b - - 0 1','7k/5K2/6Q1/8/8/8/8/8 b - - 0 1'):
  e.send('position fen '+fen);e.send('go depth 4');assert e.until('bestmove ')=='bestmove 0000'
 e.send('position startpos');e.send('go infinite');e.send('isready');assert e.until('readyok')=='readyok'
 e.send('stop');assert e.until('bestmove ')!='bestmove 0000'
 # Both commands survive a single write while searching.
 e.send('position startpos');e.send('go infinite');e.send('stop\nisready');e.until('readyok');e.until('bestmove ')
 e.send('position fen 8/8/8/8/8/8/8/8 w - - 0 1');e.until('info string invalid FEN')
 e.send('position startpos moves e2e5');e.until('info string illegal move')
 e.close();print(path,'PROTOCOL TESTS PASS')
# Exercise >32 matching book entries and Polyglot castling under sanitizers.
keys=[int(x,16) for x in re.findall(r'0x[0-9A-Fa-f]+',(root/'polykeys.c').read_text())]
assert len(keys)==781
castle_key=keys[64*11+4]^keys[64*10+60]^keys[64*7+7]^keys[768]^keys[780]
records=[(0x463b96181691fc9c,(4<<6)|(1<<9)|4|(3<<3),1,0)]*40
records.append((castle_key,(4<<6)|7,1,0))
with tempfile.TemporaryDirectory() as td:
 pathlib.Path(td,'performance.bin').write_bytes(b''.join(struct.pack('>QHHI',*row) for row in sorted(records)))
 e=Engine(str(root/'build/sanitize/jace'),td);e.send('uci');e.until('uciok');e.send('setoption name Book value true')
 e.send('position startpos');e.send('go depth 1');assert e.until('bestmove ')=='bestmove e2e4'
 e.send('position fen 4k3/8/8/8/8/8/8/4K2R w K - 0 1');e.send('go depth 1');assert e.until('bestmove ')=='bestmove e1g1'
 e.close();print('BOOK TESTS PASS')
# Console remains supported after removing the XBoard interface.
e=Engine(str(root/'build/sanitize/jace'));e.send('jace');e.until('Welcome to Jace In Console Mode!')
e.send('force');e.send('setboard 7k/6Q1/5K2/8/8/8/8/8 b - - 0 1');e.send('go')
e.until('1-0 {checkmate}',contains=True)
e.send('force');e.send('new');e.send('depth 1');e.send('go')
e.until('Jace plays ',contains=True)
e.close();print('CONSOLE TESTS PASS')
subprocess.run([sys.executable,'tests/features.py'],check=True,env=env,timeout=120)
for mode in ('release','sanitize'):
 subprocess.run([sys.executable,'tests/perft_input.py',str(root/'build'/mode/'jace')],check=True,env=env,timeout=120)
# Leave the convenient executable as the release build.
subprocess.run(['make','release'],check=True)
print('ALL TESTS PASS')
