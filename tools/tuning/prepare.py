#!/usr/bin/env python3
"""Prepare outcome-labelled quiet positions; split by opening, never by position."""
import argparse, collections, hashlib, json, pathlib, random
import chess
import chess.pgn

def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()

def key(board):
    return ' '.join(board.fen().split()[:4])

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--output', type=pathlib.Path, required=True)
    ap.add_argument('pgn', nargs='+', type=pathlib.Path)
    args = ap.parse_args()
    if args.output.exists():
        raise SystemExit('Output directory already exists; use a fresh destination.')
    rows = {name: {} for name in ('train', 'validation', 'test')}
    games_seen = set(); position_splits = collections.defaultdict(set)
    stats = collections.Counter(); sources = []
    for path in sorted(args.pgn):
        sources.append({'path': str(path.resolve()), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
        with path.open(errors='strict') as stream:
            while (game := chess.pgn.read_game(stream)) is not None:
                stats['games_read'] += 1
                result = {'1-0': 1.0, '0-1': 0.0, '1/2-1/2': 0.5}.get(game.headers.get('Result'))
                if game.errors or result is None or game.headers.get('Termination', '').lower() != 'normal':
                    stats['excluded_games'] += 1; continue
                board = game.board(); opening = key(board)
                moves = list(game.mainline_moves())
                identity = digest(opening + ' ' + ' '.join(m.uci() for m in moves))
                if identity in games_seen:
                    stats['duplicate_games'] += 1; continue
                games_seen.add(identity)
                # Standard-start games group by the board after eight full moves.
                if board.fullmove_number == 1 and len(moves) >= 16:
                    prefix = board.copy()
                    for move in moves[:16]: prefix.push(move)
                    opening = key(prefix)
                bucket = int(digest(opening)[:8], 16) % 10
                split = 'test' if bucket == 0 else 'validation' if bucket == 1 else 'train'
                candidates = []
                for move in moves:
                    capture = board.is_capture(move)
                    board.push(move)
                    if (board.ply() < 24 or capture or move.promotion or board.is_check()
                            or board.is_game_over() or len(board.piece_map()) <= 6
                            or any(board.generate_legal_captures())
                            or any(m.promotion for m in board.legal_moves)):
                        continue
                    candidates.append((board.ply(), board.fen()))
                # Limit correlated observations and long-game dominance.
                random.Random(int(identity[:16], 16)).shuffle(candidates)
                chosen = []
                for ply, fen in candidates:
                    if any(abs(ply - other) < 8 for other in chosen): continue
                    chosen.append(ply)
                    position = ' '.join(fen.split()[:4])
                    position_splits[position].add(split)
                    rows[split].setdefault(position, []).append((result, fen, identity, digest(opening)))
                    if len(chosen) == 8: break
                stats['games_accepted'] += 1
    args.output.mkdir(parents=True)
    manifest = {'sources': sources, 'statistics': dict(stats), 'split': 'opening SHA256, 80/10/10',
                'filters': 'normal games; ply>=24; >6 pieces; no check, captures or promotions available; max8/game, >=8 ply apart', 'datasets': {}}
    for split, positions in rows.items():
        output = []; groups = set(); labels = collections.Counter()
        for position, observations in sorted(positions.items()):
            if len(position_splits[position]) != 1: continue
            # One observation per unique position; deterministic, not result-selected.
            result, fen, identity, opening = min(observations, key=lambda x: x[2])
            output.append(f'{result}\t{fen}\n'); groups.add(opening); labels[str(result)] += 1
        path = args.output / f'{split}.tsv'; path.write_text(''.join(output))
        manifest['datasets'][split] = {'positions': len(output), 'opening_groups': len(groups),
            'outcomes': dict(labels), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
    manifest['cross_split_positions_removed'] = sum(len(s) > 1 for s in position_splits.values())
    (args.output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps(manifest['datasets'], indent=2))

if __name__ == '__main__': main()
