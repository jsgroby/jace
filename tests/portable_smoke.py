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
        try:
            line = lines.get(timeout=max(.001, end-time.monotonic()))
        except queue.Empty:
            raise AssertionError('Timeout waiting for '+prefix+'; recent output: '+repr(seen[-5:])) from None
        assert line is not None, 'Engine exited: ' + ''.join(errors)
        seen.append(line)
        if line.startswith(prefix):
            return seen
        assert time.monotonic() < end, 'Timeout: ' + prefix
def ready():
    send('isready'); return until('readyok')
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
    assert 'id name Jace 1.4' in ids
    assert 'option name Clear Hash type button' in ids
    assert 'option name Move Overhead type spin default 50 min 0 max 5000' in ids
    for option in ('Book value false','MultiPV value 1','Ponder value false'):
        send('setoption name ' + option)
    ready()
    send('position startpos'); send('go depth 5')
    result=until('bestmove '); assert result[-1] != 'bestmove 0000'
    print('Depth-5 result:',result[-1])
    send('position startpos'); send('go depth 3 searchmoves e2e4')
    assert until('bestmove ')[-1].split()[1]=='e2e4'
    send('setoption name mUlTiPv value 3'); send('position startpos'); send('go depth 3')
    result=until('bestmove ')
    rows=[line.split() for line in result if line.startswith('info score ') and 'depth 3 ' in line]
    assert {int(row[row.index('multipv')+1]) for row in rows}=={1,2,3}
    assert len({row[row.index('pv')+1] for row in rows})==3
    send('setoption name MultiPV value 1')
    # Keep a completed bounded search waiting in infinite mode while injecting
    # commands that must not act like stop, on every native CI platform.
    send('position startpos');send('go infinite depth 2')
    until('info score ')
    send('debug on');until('info string debug on')
    send('not-a-uci-command');send('debug off')
    assert not any(x.startswith('bestmove') for x in ready())
    silence();send('stop');until('bestmove ')
    # Clear Hash must retain this non-starting position.
    send('position startpos moves e2e4')
    send('setoption name cLeAr hAsH')
    send('setoption name move overhead value 50')
    send('setoption name pOnDeR value FALSE')
    send('go depth 2 searchmoves e7e5')
    assert until('bestmove ')[-1].split()[1]=='e7e5'
    send('setoption name MultiPV value 1');send('position startpos');send('go infinite')
    send('unknown-during-search');send('debug on');send('debug off')
    assert not any(x.startswith('bestmove') for x in ready())
    ready();send('stop');until('bestmove ')
    send('position startpos');send('go ponder depth 2');until('info score ')
    silence();send('ponderhit');until('bestmove ')
    send('position startpos');send('go ponder depth 2');until('info score ')
    silence();send('stop');until('bestmove ')
    send('quit');p.wait(timeout=5);assert p.returncode==0
    print('PASS: UCI options, debug/unknown commands, Clear Hash, searchmoves, MultiPV, readiness during search, stop, ponderhit, ponder stop, quit')
finally:
    if p.poll() is None:
        p.kill();p.wait()
