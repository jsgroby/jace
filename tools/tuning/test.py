#!/usr/bin/env python3
"""Check score orientation, runtime/compiled agreement and parameter isolation."""
import math, pathlib, re, subprocess, tempfile
root = pathlib.Path(__file__).resolve().parents[2]
params = re.findall(r'PARAM\((\w+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\)', (root/'eval_params.def').read_text())
weights = [int(p[1]) for p in params]
fens = re.findall(r'^"(.*)",$', (root/'tests/qsearch_positions.inc').read_text(), re.M)
# Quiet legal positions with either side to move test score orientation.
paired = fens.copy()
quiet = 'rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2'
paired += [quiet, quiet.replace(' w ', ' b ')]
with tempfile.TemporaryDirectory() as tmp:
    dataset = pathlib.Path(tmp)/'sample.tsv'
    dataset.write_text(''.join(f'{[0,0.5,1][i%3]}\t{fen}\n' for i,fen in enumerate(paired)))
    def run(name, requests):
        return subprocess.check_output([str(root/'build/release'/name), str(dataset)], input=requests, text=True).splitlines()
    fixed = list(map(int, run('eval-fixed', '0\n')))
    request = ' '.join(map(str, [0,*weights]))+'\n'
    actual = list(map(int, run('eval-tune', request)))
    assert actual == fixed, 'Mutable/default evaluation differs from production'
    assert actual[-1] == actual[-2], 'White score orientation changed'
    changed = weights.copy(); changed[0] = max(int(params[0][2]), weights[0]-4)
    requests = ' '.join(map(str,[0,*changed]))+'\n'+request
    values = list(map(int, run('eval-tune', requests)))
    assert values[:len(actual)] != actual, 'Parameter perturbation ineffective'
    assert values[len(actual):] == actual, 'Weights or pawn scores leaked between requests'
    mse = float(run('eval-tune', ' '.join(map(str,[1,*weights]))+'\n')[0])
    expected = sum((1/(1+10**(-score/400))-[0,0.5,1][i%3])**2 for i,score in enumerate(actual))/len(actual)
    assert math.isclose(mse,expected,rel_tol=1e-12)
print(f'PASS: {len(actual)} scores; compiled/runtime equality, White orientation, reset/cache isolation, exact MSE')
