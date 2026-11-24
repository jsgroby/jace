#!/usr/bin/env python3
"""PGN ingestion checks; run in the pinned python-chess environment."""
import json,pathlib,subprocess,sys,tempfile
import chess,chess.pgn
root=pathlib.Path(__file__).resolve().parents[2]
board=chess.Board('r6k/pp6/6Q1/5K2/8/8/P7/R7 w - - 0 20')
game=chess.pgn.Game();game.setup(board);node=game
for uci in ('f5f6','b7b6','g6g7'):
    move=chess.Move.from_uci(uci);assert move in board.legal_moves
    node=node.add_variation(move);board.push(move)
assert board.is_checkmate();game.headers['Result']='1-0'
with tempfile.TemporaryDirectory() as tmp:
    p=pathlib.Path(tmp);old=p/'a-old.pgn';fresh=p/'z-fresh.pgn'
    text=str(game)+'\n\n';old.write_text(text);fresh.write_text(text)
    command=[sys.executable,str(root/'tools/tuning/prepare.py'),'--allow-captures',
             '--verify-missing-termination','--holdout-pgn',str(fresh),'--test-source',str(fresh),
             '--output',str(p/'data'),str(old),str(fresh)]
    subprocess.run(command,check=True,stdout=subprocess.DEVNULL)
    manifest=json.loads((p/'data/manifest.json').read_text())
    assert manifest['datasets']['test']['positions']>0
    assert manifest['datasets']['train']['positions']==manifest['datasets']['validation']['positions']==0
    assert manifest['statistics']['games_accepted']==1
    assert manifest['statistics']['untagged_games_verified']==2
    # A false untagged outcome must be rejected, even with an otherwise valid PGN.
    old.write_text(text.replace('1-0','0-1'))
    command[command.index('--output')+1]=str(p/'invalid');command=command[:-1]
    subprocess.run(command,check=True,stdout=subprocess.DEVNULL)
    manifest=json.loads((p/'invalid/manifest.json').read_text())
    assert manifest['statistics']['excluded_games']==1
print('PASS: full-opening holdout isolation, cross-source duplicate handling, verified untagged outcomes')
