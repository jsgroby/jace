"""Choose test fixtures first, then the conventional user Syzygy directory."""
from pathlib import Path
fixtures = Path(__file__).resolve().parent / 'syzygy-data'
local = Path.home() / 'Development/Chess/Tablebases/syzygy'
print(next((p for p in (fixtures, local) if (p / 'KQvK.rtbw').is_file()), fixtures))
