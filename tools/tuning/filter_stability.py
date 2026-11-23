#!/usr/bin/env python3
"""Filter each pre-grouped split with a frozen baseline qsearch executable."""
import argparse, hashlib, json, pathlib, subprocess

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--input',type=pathlib.Path,required=True)
    ap.add_argument('--output',type=pathlib.Path,required=True)
    ap.add_argument('--exe',type=pathlib.Path,required=True)
    args=ap.parse_args()
    args.output.mkdir(exist_ok=False)
    manifest=json.loads((args.input/'manifest.json').read_text())
    manifest['stability_filter']={'executable_sha256':hashlib.sha256(args.exe.read_bytes()).hexdigest(),
        'threshold_cp':25,'node_limit':10000,'source_manifest_sha256':hashlib.sha256((args.input/'manifest.json').read_bytes()).hexdigest()}
    for split in ('train','validation','test'):
        source=args.input/f'{split}.tsv';target=args.output/f'{split}.tsv'
        with source.open() as inp,target.open('w') as out:
            result=subprocess.run([str(args.exe.resolve())],stdin=inp,stdout=out,stderr=subprocess.PIPE,text=True,check=True)
        rows=target.read_text().splitlines()
        assert rows and all(len(row.split('\t'))==2 for row in rows)
        assert set(rows)<=set(source.read_text().splitlines())
        manifest['datasets'][split]['raw_positions']=manifest['datasets'][split]['positions']
        manifest['datasets'][split]['positions']=len(rows)
        manifest['datasets'][split]['sha256']=hashlib.sha256(target.read_bytes()).hexdigest()
        manifest['datasets'][split]['filter_log']=result.stderr.strip()
        # Counts/groups recorded by prepare.py refer to the raw sample, not filtered rows.
        manifest['datasets'][split]['raw_outcomes']=manifest['datasets'][split].pop('outcomes')
        manifest['datasets'][split]['raw_opening_groups']=manifest['datasets'][split].pop('opening_groups')
        manifest['datasets'][split]['outcomes']={label:sum(row.startswith(label+'\t') for row in rows) for label in ('0.0','0.5','1.0')}
        print(split,result.stderr.strip(),flush=True)
    (args.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')

if __name__=='__main__': main()
