"""Native Windows/macOS/Linux UCI smoke test; no select() on Windows pipes."""
import pathlib, queue, subprocess, sys, threading, time
exe = pathlib.Path(sys.argv[1]).resolve()
p = subprocess.Popen([str(exe)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                     stderr=subprocess.PIPE, text=True, bufsize=1)
lines = queue.Queue()
errors = []
def reader():
    for line in p.stdout:
        lines.put(line.strip())
    lines.put(None)
threading.Thread(target=reader, daemon=True).start()
threading.Thread(target=lambda: errors.append(p.stderr.read()), daemon=True).start()
def send(command):
    p.stdin.write(command + '\n'); p.stdin.flush()
def until(prefix, timeout=20):
    end = time.monotonic() + timeout
    seen = []
    while True:
        line = lines.get(timeout=max(.001, end-time.monotonic()))
        assert line is not None, 'Engine exited: ' + ''.join(errors)
        seen.append(line)
        if line.startswith(prefix):
            return seen
        assert time.monotonic() < end, 'Timeout: ' + prefix
def ready():
    send('isready'); until('readyok')
def silence():
    end = time.monotonic() + .2
    while time.monotonic() < end:
        try:
            line = lines.get(timeout=max(.001, end-time.monotonic()))
        except queue.Empty:
            return
        assert line is not None and not line.startswith('bestmove'), line
try:
    send('uci'); ids=until('uciok')
    assert 'id name Jace 1.3' in ids
    for option in ('Book value false','MultiPV value 1','Ponder value false'):
        send('setoption name ' + option)
    ready()
    send('position startpos'); send('go depth 5')
    result=until('bestmove '); assert result[-1] != 'bestmove 0000'
    print('Depth-5 result:',result[-1])
    send('position startpos'); send('go depth 3 searchmoves e2e4')
    assert until('bestmove ')[-1].split()[1]=='e2e4'
    send('setoption name MultiPV value 3'); send('position startpos'); send('go depth 3')
    result=until('bestmove ')
    rows=[line.split() for line in result if line.startswith('info score ') and 'depth 3 ' in line]
    assert {int(row[row.index('multipv')+1]) for row in rows}=={1,2,3}
    assert len({row[row.index('pv')+1] for row in rows})==3
    send('setoption name MultiPV value 1');send('position startpos');send('go infinite')
    ready();send('stop');until('bestmove ')
    send('position startpos');send('go ponder depth 2');until('info score ')
    silence();send('ponderhit');until('bestmove ')
    send('position startpos');send('go ponder depth 2');until('info score ')
    silence();send('stop');until('bestmove ')
    send('quit');p.wait(timeout=5);assert p.returncode==0
    print('PASS: UCI, searchmoves, MultiPV, readiness during search, stop, ponderhit, ponder stop, quit')
finally:
    if p.poll() is None:
        p.kill();p.wait()
