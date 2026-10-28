"""Portable actual-search SMP lifecycle regression (also run under TSan)."""
import pathlib, queue, subprocess, sys, threading, time
exe = str(pathlib.Path(sys.argv[1]).resolve())
p = subprocess.Popen([exe, 'NoBook'], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                     stderr=subprocess.PIPE, text=True, bufsize=1)
q = queue.Queue()
errors = []
def read():
    for line in p.stdout:
        q.put(line.strip())
    q.put(None)
t = threading.Thread(target=read); t.start()
e = threading.Thread(target=lambda: errors.append(p.stderr.read())); e.start()
def send(s):
    p.stdin.write(s+'\n'); p.stdin.flush()
def until(prefix, timeout=30):
    end = time.monotonic()+timeout; seen=[]
    while True:
        line=q.get(timeout=max(.001,end-time.monotonic()))
        assert line is not None, errors
        seen.append(line)
        if line.startswith(prefix): return seen
        assert time.monotonic()<end, seen[-5:]
def ready():
    send('isready'); rows=until('readyok')
    assert not any(x.startswith('bestmove') for x in rows), rows
    return rows
def search(command, move=None):
    send('position startpos'); send(command); rows=until('bestmove ')
    assert rows[-1].split()[1] != '0000', rows
    if move: assert rows[-1].split()[1] == move, rows
    ready() # catches a duplicate bestmove before the readiness fence
    return rows
try:
    send('uci'); assert 'option name Threads type spin default 1 min 1 max 4' in until('uciok')
    send('setoption name Book value false')
    for n in (2,4,1,4):
        send(f'setoption name Threads value {n}')
        for unused in range(3): search('go depth 4 searchmoves e2e4', 'e2e4')
        rows=search('go nodes 20000')
        if n>1:
            totals=[int(x.split()[2]) for x in rows if x.startswith('info nodes ')]
            assert len(totals)==1 and 20000<=totals[0]<=20000+n*256, totals
        start=time.monotonic(); search('go movetime 100')
        assert time.monotonic()-start<5
        for command, finish in [('go infinite','stop'),('go ponder wtime 1000 btime 1000','ponderhit'),
                                ('go ponder depth 2','ponderhit'),('go ponder depth 2','stop')]:
            send('position startpos');send(command);until('info score ')
            ready();send(finish);until('bestmove ');ready()
        send('position startpos');send('go infinite');until('info score ')
        send('setoption name Hash value 8');until('bestmove ');ready()
        search('go depth 3')
    for fen in ('7k/6Q1/5K2/8/8/8/8/8 b - - 0 1','7k/5K2/6Q1/8/8/8/8/8 b - - 0 1'):
        send('position fen '+fen);send('go depth 3');assert until('bestmove ')[-1]=='bestmove 0000'
    send('setoption name MultiPV value 3')
    rows=search('go depth 3')
    assert {int(x.split('multipv ')[1].split()[0]) for x in rows if 'depth 3 ' in x and 'multipv ' in x}=={1,2,3}
    assert not any(x.startswith('info nodes ') for x in rows) # serial analysis fallback
    send('setoption name MultiPV value 1')
    send('position startpos');send('go infinite');until('info score ');send('quit')
    p.wait(timeout=5);t.join();e.join()
    assert p.returncode==0 and not ''.join(errors), errors
    assert not any(x and x.startswith('bestmove') for x in list(q.queue))
    print('PASS: SMP 1/2/4 threads, restricted roots, node budgets, time, stop, pondering, resize, MultiPV, quit')
finally:
    if p.poll() is None: p.kill();p.wait()
    t.join();e.join()

# EOF while a parallel search is active must join workers and exit cleanly.
closed=subprocess.run([exe,'NoBook'],input='uci\nsetoption name Threads value 4\nposition startpos\ngo infinite\n',capture_output=True,text=True,timeout=10)
assert closed.returncode==0 and not closed.stderr, closed.stderr
assert 'bestmove ' not in closed.stdout, closed.stdout
print('PASS: SMP EOF shutdown')
