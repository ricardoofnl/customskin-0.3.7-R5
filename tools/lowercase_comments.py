#!/usr/bin/env python3
"""lowercase comment prose in c/c++/pawn sources, while keeping code identifiers
(function/type names, Scoped::Names, snake_case, mixedCase, hex, call sites) as-is,
and dropping a trailing '.' at the end of each comment line.

only comments are touched - string/char literals and code are left untouched.

usage:
  python3 tools/lowercase_comments.py [--write] [paths ...]

  default paths: src  (recurses for .h/.hpp/.cpp/.cc/.pwn/.inc; vendor/ is skipped)
  without --write it is a dry run: prints a unified diff and changes nothing.

what counts as an identifier (kept, not lowercased):
  - contains '::' or '_'                         (Foo::Bar, ms_modelInfoPtrs)
  - has both a letter and a digit               (R5, CRC32, 0xAE64)
  - mixedCase / CamelCase (not plain Titlecase) (RwStreamOpen, setPlayerSkin)
  - immediately followed by '('                 (Init(), Base())
  - preceded by '.' or '::'                      (member / scope tail)
plain words and simple Titlecase words (Skin, Model, The) are lowercased.
"""
import difflib
import os
import re
import sys

TOKEN = re.compile(r'0[xX][0-9A-Fa-f]+|[A-Za-z0-9_]+(?:::[A-Za-z0-9_]+)*')


def is_identifier_like(t):
    if '::' in t or '_' in t:
        return True
    if re.search(r'[A-Za-z]', t) and re.search(r'[0-9]', t):
        return True  # r5, crc32, ae64, 0.3dl parts
    has_up = any(c.isupper() for c in t)
    has_lo = any(c.islower() for c in t)
    if has_up and has_lo and not re.fullmatch(r'[A-Z][a-z]+', t):
        return True  # camelCase / MixedCase, but not a plain Titlecase word
    return False


def xform_word(m):
    t = m.group(0)
    s, b, e = m.string, m.start(), m.end()
    if t[:2] in ('0x', '0X'):
        return t
    if e < len(s) and s[e] == '(':          # function call -> keep the name
        return t
    if (b > 0 and s[b - 1] == '.') or s[max(0, b - 2):b].endswith('::'):
        return t                             # member / scope tail
    return t if is_identifier_like(t) else t.lower()


def strip_trailing_period(line):
    m = re.match(r'^(.*?)\.(\s*)$', line, re.S)
    if m and not m.group(1).endswith('.'):   # keep an ellipsis '...'
        return m.group(1) + m.group(2)
    return line


def xform_content(text):
    text = TOKEN.sub(xform_word, text)
    return '\n'.join(strip_trailing_period(ln) for ln in text.split('\n'))


def process(src):
    out = []
    i, n = 0, len(src)
    while i < n:
        c = src[i]
        if c == '"' or c == "'":             # string / char literal - skip verbatim
            j = i + 1
            while j < n:
                if src[j] == '\\':
                    j += 2
                    continue
                if src[j] == c:
                    j += 1
                    break
                j += 1
            out.append(src[i:j])
            i = j
            continue
        if c == '/' and i + 1 < n and src[i + 1] == '/':    # line comment
            j = i + 2
            while j < n and src[j] != '\n':
                j += 1
            out.append('//' + xform_content(src[i + 2:j]))
            i = j
            continue
        if c == '/' and i + 1 < n and src[i + 1] == '*':    # block comment
            j = i + 2
            while j < n and not (src[j] == '*' and j + 1 < n and src[j + 1] == '/'):
                j += 1
            end = min(j + 2, n)
            out.append('/*' + xform_content(src[i + 2:j]) + src[j:end])
            i = end
            continue
        out.append(c)
        i += 1
    return ''.join(out)


EXTS = ('.h', '.hpp', '.cpp', '.cc', '.pwn', '.inc')


def gather(paths):
    files = []
    for p in paths:
        if os.path.isdir(p):
            for root, _, fs in os.walk(p):
                if 'vendor' in root.split(os.sep):
                    continue
                for f in sorted(fs):
                    if f.endswith(EXTS):
                        files.append(os.path.join(root, f))
        elif os.path.isfile(p):
            files.append(p)
    return files


def main():
    args = sys.argv[1:]
    write = '--write' in args
    paths = [a for a in args if not a.startswith('--')] or ['src']
    changed = 0
    for fn in gather(paths):
        with open(fn, encoding='utf-8') as fh:
            s = fh.read()
        o = process(s)
        if o == s:
            continue
        changed += 1
        if write:
            with open(fn, 'w', encoding='utf-8') as fh:
                fh.write(o)
            print('wrote', fn)
        else:
            sys.stdout.writelines(
                difflib.unified_diff(s.splitlines(True), o.splitlines(True),
                                     fn + ' (before)', fn + ' (after)'))
    verb = 'changed' if write else 'would change'
    print(f'\n{changed} file(s) {verb}. {"" if write else "(dry run; pass --write to apply)"}')


if __name__ == '__main__':
    main()
