import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cs_types import build_index, resolve  # noqa: E402

D = os.path.join('C:' + os.sep, 'Projects', 'KSP-UE5', 'decompiled', 'KSP-Decompiled')
idx = build_index([os.path.join(D, 'Assembly-CSharp'), os.path.join(D, 'Assembly-CSharp-firstpass')])
for n in sys.argv[1:]:
    cands = resolve(idx, n)
    print(n, '->', [(c[0], c[1], c[2], c[4], os.path.relpath(c[3], D)) for c in cands][:4])
