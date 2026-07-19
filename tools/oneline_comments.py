#!/usr/bin/env python3
"""collapse c/c++/pawn comments to a single short line and drop phase references.

for a block of full-line `//` comments it keeps only the first paragraph (the lines
before the first blank comment line), joins it into one line, and throws the rest away.
trailing `//` comments on a code line are kept in place. phase mentions ("phase 4.3",
"(phase 2b, ...)") and a trailing '.' are stripped. string/char literals are never touched,
and `/* */` blocks are left alone.

usage:
  python3 tools/oneline_comments.py [--write] [paths ...]

  default paths: src  (recurses for .h/.hpp/.cpp/.cc/.pwn/.inc; vendor/ is skipped)
  without --write it is a dry run: prints a unified diff and changes nothing.
"""
import difflib
import os
import re
import sys


def split_comment(line):
    """return (code, comment) where comment is the '//...' tail (or None), skipping
    over string/char literals so a '//' inside a literal is not treated as a comment."""
    in_str = None
    i, n = 0, len(line)
    while i < n:
        c = line[i]
        if in_str:
            if c == '\\':
                i += 2
                continue
            if c == in_str:
                in_str = None
            i += 1
            continue
        if c in '"\'':
            in_str = c
        elif c == '/' and i + 1 < n and line[i + 1] == '/':
            return line[:i], line[i:]
        elif c == '/' and i + 1 < n and line[i + 1] == '*':
            return line, None  # leave block comments alone
        i += 1
    return line, None


def clean_comment(text):
    """drop phase references and tidy up the prose."""
    text = re.sub(r'\s*\([^)]*\bphase\b[^)]*\)', '', text, flags=re.I)  # (phase ...)
    text = re.sub(r'\bphase\s*[0-9][0-9a-z.+]*\s*[:.\-]*\s*', '', text, flags=re.I)
    text = re.sub(r'\bphase\b\s*', '', text, flags=re.I)
    text = re.sub(r'\s{2,}', ' ', text).strip()
    text = re.sub(r'^[,;:\-\s]+', '', text)
    if text.endswith('.') and not text.endswith('..'):
        text = text[:-1].rstrip()
    return text


def process(text):
    lines = text.split('\n')
    out = []
    i, n = 0, len(lines)
    while i < n:
        code, comment = split_comment(lines[i])
        if comment is not None and code.strip() == '':
            indent = code
            para = []
            j = i
            while j < n:
                c2, cm2 = split_comment(lines[j])
                if cm2 is None or c2.strip() != '':
                    break
                body = cm2[2:].strip()
                if body == '':
                    j += 1
                    break
                para.append(body)
                j += 1
            while j < n:  # skip the rest of the comment block
                c3, cm3 = split_comment(lines[j])
                if cm3 is None or c3.strip() != '':
                    break
                j += 1
            cleaned = clean_comment(' '.join(para))
            if cleaned:
                out.append(f'{indent}// {cleaned}')
            i = j
            continue
        if comment is not None:
            body = comment[2:].strip()
            cleaned = clean_comment(body)
            if cleaned == body:
                out.append(lines[i])  # unchanged - keep verbatim (preserves alignment)
            elif cleaned:
                out.append(f'{code}// {cleaned}')  # keep the original gap before //
            else:
                out.append(code.rstrip())
        else:
            out.append(lines[i])
        i += 1
    return '\n'.join(out)


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
