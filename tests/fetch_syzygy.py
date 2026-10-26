"""Fetch small upstream test fixtures, pinned by Git blob ID; never packaged.
Source: python-chess test data. These are test inputs, not JACE source or assets.
"""
import hashlib, urllib.request
from pathlib import Path
files={
 'KBvK.rtbw':'95919ca67c5255bfad2f93e820824ffe7a8c6ecd',
 'KBvK.rtbz':'d48c792039bcc55e2d91e209637fe8e14310cf3f',
 'KNvK.rtbw':'c2d9633254334a912dd546b313073215811e3fc1',
 'KNvK.rtbz':'98383d656af5c441b64d2f967f6fcf1c20f8c3c9',
 'KPvK.rtbw':'1af4d64c187464d79749bd7a9ef4e3c017d2508c',
 'KPvK.rtbz':'06e9217b4e0c4c9fe10affd5d4039a6b0971795a',
 'KQvK.rtbw':'0f4b29211eddd62f33b88471420f2674b35a5164',
 'KQvK.rtbz':'507469523978f81d52701a12cddb47545865f242',
 'KRvK.rtbw':'2a2855ae9521d28365f91fd611c55e828dbd0267',
 'KRvK.rtbz':'6cd8607aacddaca6285ca5ec3904e71f2012411b',
}
out=Path(__file__).resolve().parent/'syzygy-data';out.mkdir(exist_ok=True)
for name,sha in files.items():
 path=out/name
 if path.exists():data=path.read_bytes()
 else:
  url='https://raw.githubusercontent.com/niklasf/python-chess/master/data/syzygy/regular/'+name
  data=urllib.request.urlopen(url,timeout=30).read()
 assert hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()==sha,name
 path.write_bytes(data)
print('Verified 10 Syzygy test fixtures')
