#!/usr/bin/env python3
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src")
NAMESPACE_CLOSER = re.compile(r"^\}\s*//\s*namespace(\s+[\w:]+)?$")
STRING_LITERAL = re.compile(r'"(\\.|[^"\\])*"')

violations = []
scanned = 0

for directory, _, files in os.walk(ROOT):
    for name in sorted(files):
        if not name.endswith((".cpp", ".hpp")):
            continue
        path = os.path.join(directory, name)
        scanned += 1
        with open(path, encoding="utf-8") as handle:
            for number, line in enumerate(handle, 1):
                stripped = line.strip()
                if NAMESPACE_CLOSER.match(stripped):
                    continue
                code = STRING_LITERAL.sub("", line)
                if "//" in code or "/*" in code or "*/" in code:
                    violations.append((os.path.relpath(path, ROOT), number, stripped))

print(f"scanned {scanned} files")
for path, number, text in violations:
    print(f"  {path}:{number}: {text}")

print("OK: no comments" if not violations else f"FAILED: {len(violations)} comment(s)")
sys.exit(1 if violations else 0)
