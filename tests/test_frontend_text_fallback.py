#!/usr/bin/env python3
"""Keep the reVC menu fallback in sync with the English GXT source."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
source = (root / 'utils/gxt/american.txt').read_text(encoding='utf-8')
additions = source.split('{ reVC updates }', 1)[1].split('{ end of file }', 1)[0]
entries = dict(re.findall(r'^\[([^\]]+)\][^\n]*\n([^\n]+)', additions, re.MULTILINE))
code = (root / 'src/text/Text.cpp').read_text()
fallbacks = dict(re.findall(r'\{ "([A-Z0-9_]+)", "([^"]+)" \}', code))
assert entries, 'no reVC entries found'
for key, value in entries.items():
    assert fallbacks.get(key) == value.strip(), f'{key}: missing or incorrect fallback'

lookup = code.split('CText::Get(const char *key)', 1)[1].split('wchar UpperCaseTable', 1)[0]
assert lookup.index('keyArray.Search') < lookup.index('mission_keyArray.Search') < lookup.index('LookupFrontendFallbackText')
print(f'{len(entries)} reVC fallbacks match the English source; loaded text takes precedence')
