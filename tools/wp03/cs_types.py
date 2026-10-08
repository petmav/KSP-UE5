"""Index of C# type declarations in the decompiled KSP assemblies (used by gen_game_events.py)."""
import os, re

DECL = re.compile(r'^(\s*)(?:(?:public|internal|private|protected|static|sealed|abstract|partial|new|readonly|unsafe)\s+)*'
                  r'(class|struct|enum|interface|delegate)\s+(?:(?:[\w<>\[\],\. ]+?)\s+)?(\w+)(?:<[^>]*>)?\s*(?::\s*([\w\.]+))?')
NS = re.compile(r'^namespace\s+([\w\.]+)')

def build_index(roots):
    """-> dict simple_name -> list of (kind, namespace, outer_chain, file, underlying)"""
    idx = {}
    for root in roots:
        for dp, dn, fn in os.walk(root):
            for f in fn:
                if not f.endswith('.cs'):
                    continue
                p = os.path.join(dp, f)
                try:
                    lines = open(p, encoding='utf-8-sig', errors='replace').read().splitlines()
                except OSError:
                    continue
                ns = ''
                stack = []  # (name, depth_at_open)
                depth = 0
                pending = None
                for line in lines:
                    m = NS.match(line)
                    if m:
                        ns = m.group(1)
                    d = DECL.match(line)
                    if d and '(' not in line.split(d.group(3))[0] and not line.strip().startswith('//'):
                        kind, name, under = d.group(2), d.group(3), d.group(4)
                        if kind == 'delegate':
                            m2 = re.match(r'^\s*(?:(?:public|internal|private|protected)\s+)*delegate\s+(.+?)\s+(\w+)(?:<[^>]*>)?\s*\((.*)\)\s*;', line)
                            if m2:
                                name = m2.group(2)
                                under = (m2.group(1), m2.group(3))
                        outer = [s[0] for s in stack]
                        idx.setdefault(name, []).append((kind, ns, '.'.join(outer), p, under))
                        if kind != 'delegate':
                            pending = (name, depth)
                    opens = line.count('{'); closes = line.count('}')
                    if pending and opens:
                        stack.append((pending[0], depth))
                        pending = None
                    depth += opens - closes
                    while stack and depth <= stack[-1][1]:
                        stack.pop()
    return idx


def resolve(idx, name, prefer_ns=None):
    """Candidates for a C# type reference `name` (simple, Nested.Type or Namespace.Type)."""
    parts = name.split('.')
    simple = parts[-1]
    cands = idx.get(simple, [])
    if len(parts) > 1:
        qual = '.'.join(parts[:-1])
        c2 = [c for c in cands if c[2] == qual or c[2].endswith('.' + qual) or c[1] == qual or c[1].endswith('.' + qual)]
        return c2 or cands
    c2 = [c for c in cands if c[2] == '']
    if prefer_ns is not None and c2:
        c3 = [c for c in c2 if c[1] in prefer_ns]
        if c3:
            return c3
    return c2 or cands
