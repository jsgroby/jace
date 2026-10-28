"""Bound native thread tests so a deadlock fails CI instead of hanging it."""
import subprocess,sys
subprocess.run(sys.argv[1:],check=True,timeout=60)
