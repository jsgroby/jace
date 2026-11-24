#!/usr/bin/env python3
"""Bounded coordinate descent using JACE's exact integer evaluator."""
import argparse, hashlib, json, pathlib, re, subprocess

class Evaluator:
    def __init__(self, exe, dataset):
        self.p = subprocess.Popen([str(exe.resolve()), str(dataset.resolve())],
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
    def loss(self, k, weights):
        self.p.stdin.write(' '.join(map(str, [k, *weights])) + '\n'); self.p.stdin.flush()
        line = self.p.stdout.readline()
        if not line: raise RuntimeError('Evaluator stopped unexpectedly')
        return float(line)
    def close(self):
        self.p.stdin.close()
        if self.p.wait(timeout=10): raise RuntimeError('Evaluator failed')

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--data', type=pathlib.Path, required=True)
    ap.add_argument('--output', type=pathlib.Path, required=True)
    ap.add_argument('--exe', type=pathlib.Path, default=pathlib.Path('build/release/eval-tune'))
    ap.add_argument('--params', type=pathlib.Path, default=pathlib.Path('eval_params.def'))
    ap.add_argument('--only-prefix', default='', help='Fit only matching parameters; freeze all others')
    ap.add_argument('--monotonic-prefix', action='append', default=[], help='Require nondecreasing weights in each named rank group')
    args = ap.parse_args()
    if args.output.exists(): raise SystemExit('Output already exists')
    params = [(n, int(v), int(lo), int(hi)) for n,v,lo,hi in re.findall(
        r'PARAM\((\w+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\)', args.params.read_text())]
    initial = [p[1] for p in params]; weights = initial.copy()
    monotonic = [[i for i,p in enumerate(params) if p[0].startswith(prefix)] for prefix in args.monotonic_prefix]
    if any(len(group)<2 for group in monotonic): raise SystemExit('Monotonic group must contain at least two parameters')
    def ordered(w):
        return all(all(w[a]<=w[b] for a,b in zip(group,group[1:])) for group in monotonic)
    if not ordered(initial): raise SystemExit('Initial weights violate monotonic constraint')
    active = [i for i,p in enumerate(params) if p[0].startswith(args.only_prefix) and p[2]<p[3]]
    if not active: raise SystemExit('No matching parameters')
    train = Evaluator(args.exe, args.data / 'train.tsv')
    valid = Evaluator(args.exe, args.data / 'validation.tsv')
    # Calibrate score/result scale on training only, then freeze it.
    k = min((i / 20 for i in range(2, 61)), key=lambda x: train.loss(x, initial))
    regularization = lambda w: 0.002 * sum(((w[i]-initial[i])/20)**2 for i in active)/len(active)
    baseline = {'train': train.loss(k, initial), 'validation': valid.loss(k, initial)}
    best = initial.copy(); best_validation = baseline['validation']; history = []
    print('Baseline', k, baseline, flush=True)
    # Predetermined schedule; validation selects a checkpoint, never individual moves.
    for step in (4, 2, 1, 1, 1, 1):
        objective = train.loss(k, weights) + regularization(weights)
        for i, (name, default, low, high) in enumerate(params):
            if i not in active: continue
            winner = weights.copy(); value = objective
            for delta in (-step, step):
                trial = weights.copy(); trial[i] += delta
                if not low <= trial[i] <= high or not ordered(trial): continue
                trial_value = train.loss(k, trial) + regularization(trial)
                if trial_value < value - 1e-12: winner, value = trial, trial_value
            weights, objective = winner, value
        validation = valid.loss(k, weights)
        record = {'step': step, 'train': train.loss(k, weights), 'validation': validation,
                  'weights': dict(zip((p[0] for p in params), weights))}
        history.append(record); print(json.dumps(record), flush=True)
        if validation < best_validation: best, best_validation = weights.copy(), validation
    final = {'train': train.loss(k, best), 'validation': valid.loss(k, best)}
    train.close(); valid.close()
    # Open held-out test only after selecting and freezing the candidate.
    test = Evaluator(args.exe, args.data / 'test.tsv')
    baseline['test'] = test.loss(k, initial); final['test'] = test.loss(k, best); test.close()
    report = {'monotonic_groups': [[params[i][0] for i in group] for group in monotonic], 'active_parameters': [params[i][0] for i in active], 'k': k, 'baseline_mse': baseline, 'candidate_mse': final, 'history': history,
              'weights': dict(zip((p[0] for p in params), best)),
              'data_manifest_sha256': hashlib.sha256((args.data/'manifest.json').read_bytes()).hexdigest(),
              'evaluator_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
              'parameters_sha256': hashlib.sha256(args.params.read_bytes()).hexdigest(),
              'strength_claim': 'None. Held-out prediction loss is not match Elo.'}
    args.output.mkdir(parents=True)
    (args.output/'result.json').write_text(json.dumps(report, indent=2)+'\n')
    (args.output/'eval_params.def').write_text('/* Offline tuned candidate; requires match validation. */\n'+''.join(
        f'PARAM({n}, {v}, {lo}, {hi})\n' for (n,_,lo,hi),v in zip(params,best)))
    print(json.dumps({'baseline': baseline, 'candidate': final}, indent=2))

if __name__ == '__main__': main()
