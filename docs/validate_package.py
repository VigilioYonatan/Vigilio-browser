from pathlib import Path
import sys
root=Path(__file__).resolve().parent.parent
assert {p.name for p in root.iterdir()}=={'docs','apps'}
assert (root/'apps/vigilio-browser/assets/README.md').exists()
assert not (root/'openspec').exists()
for p in (root/'docs').rglob('*.md'):
    s=p.read_text(encoding='utf-8')
    if not s.strip(): raise AssertionError(f'empty: {p}')
    if s.count('```')%2: raise AssertionError(f'fences: {p}')
assert len(list((root/'docs/prompts').glob('[0-9][0-9][0-9][0-9].md'))) == 56
print('STRUCTURAL_PASS (not Qt/CLI validation)')
