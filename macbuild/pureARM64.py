#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
File: "macbuild/pureARM64.py"

Author: ChatGPT + Kazzz-S

------------

Goal:
  - Verify that Mach-O binaries inside a .app bundle (and optionally their external
    dependencies) are *pure arm64* (no arm64e / no x86_64).
  - Summarize results as OK / WARN / FAIL with an appropriate exit code.

Why:
  - On macOS/Apple Silicon, mixing arm64e or x86_64 into a process that expects arm64
    can trigger dyld / code-signing validation issues (e.g., SIGKILL: Invalid Page).

Requirements:
  - macOS with /usr/bin/file, /usr/bin/otool, /usr/bin/lipo available.

Usage:
  $ python3 pureARM64.py "/Applications/klayout.app"
  # Do not follow external dependencies:
  $ python3 pureARM64.py "/Applications/klayout.app" --no-deps
  # Limit which external paths are checked (default covers Homebrew/MacPorts/Conda):
  $ python3 pureARM64.py "/Applications/klayout.app" \
      --deps-filter '^/(opt/homebrew|opt/local|usr/local|Users/.*/(mambaforge|miniforge|anaconda3))'
  # Also show whether each Mach-O has LC_CODE_SIGNATURE:
  $ python3 pureARM64.py "/Applications/klayout.app" --show-code-sign

Exit codes:
  0 = all OK (pure arm64)
  1 = warnings present (fat mix includes arm64; investigate)
  2 = failures present (no arm64 slice, or only arm64e/x86_64)
"""

import argparse
import os
import re
import shlex
import subprocess
import sys

# Default external dependency filter (you can customize this per project)
DEFAULT_DEPS_FILTER = r'^/(opt/homebrew|opt/local|usr/local|Users/.*/(mambaforge|miniforge|anaconda3))'

# File name patterns that are likely to be Mach-O payloads
MACHO_EXTS = ('.dylib', '.so', '.bundle')

# ANSI color if stdout is a TTY
COLOR = sys.stdout.isatty()


def color(text: str, code: str) -> str:
    if not COLOR:
        return text
    return f'\033[{code}m{text}\033[0m'


OKC  = lambda s: color(s, '32')   # green
WRNC = lambda s: color(s, '33')   # yellow
ERRC = lambda s: color(s, '31')   # red
DIM  = lambda s: color(s, '2')    # dim


def run_cmd(cmd: list[str]) -> tuple[int, str]:
    """Run a command and capture stdout+stderr as text."""
    try:
        out = subprocess.check_output(cmd, stderr=subprocess.STDOUT, text=True)
        return 0, out
    except subprocess.CalledProcessError as e:
        return e.returncode, e.output
    except FileNotFoundError:
        return 127, f"command not found: {' '.join(map(shlex.quote, cmd))}"


def is_executable(path: str) -> bool:
    """Return True if file is executable by mode bit."""
    try:
        st = os.stat(path)
        return bool(st.st_mode & 0o111)
    except FileNotFoundError:
        return False


def looks_like_macho(path: str) -> bool:
    """Heuristic: use 'file' to check if the payload is Mach-O."""
    rc, out = run_cmd(["/usr/bin/file", path])
    return rc == 0 and "Mach-O" in out


def list_bundle_machos(app_path: str) -> list[str]:
    """
    Enumerate Mach-O candidates inside a .app:
      - Contents/MacOS
      - Contents/Frameworks
      - Contents/PlugIns
      - Contents/Buddy
    Follow symlinks to real paths; deduplicate by real path.
    """
    roots = [
        os.path.join(app_path, "Contents", "MacOS"),
        os.path.join(app_path, "Contents", "Frameworks"),
        os.path.join(app_path, "Contents", "PlugIns"),
        os.path.join(app_path, "Contents", "Buddy"),
    ]
    seen: set[str] = set()
    results: list[str] = []
    for root in roots:
        if not os.path.isdir(root):
            continue
        for dirpath, _dirnames, filenames in os.walk(root):
            try:
                real_dirpath = os.path.realpath(dirpath)
            except OSError:
                real_dirpath = dirpath
            for name in filenames:
                p = os.path.join(real_dirpath, name)
                rp = os.path.realpath(p)
                if rp in seen:
                    continue
                is_macho = looks_like_macho(p)
                # Include only: explicit Mach-O, typical Mach-O extensions (.dylib/.so/.bundle),
                # or executables that are confirmed Mach-O.
                if name.endswith(MACHO_EXTS) or is_macho or (is_executable(p) and is_macho):
                    seen.add(rp)
                    results.append(rp)
    results.sort()
    return results


def parse_archs_via_lipo(path: str) -> list[str]:
    """
    Parse architecture slices using lipo/file.
    Prefer lipo -info. Fallback to 'file' if necessary.
    Returns a sorted unique list like ['arm64'] or ['arm64', 'arm64e'] etc.
    """
    rc, out = run_cmd(["/usr/bin/lipo", "-info", path])
    if rc == 0 and "Architectures in the fat file" in out:
        # e.g. "Architectures in the fat file: QtGui are: arm64 arm64e"
        m = re.search(r'are:\s+(.+)$', out.strip())
        if m:
            return sorted(set(m.group(1).split()))
    if rc == 0 and "Non-fat file" in out:
        # e.g. "Non-fat file: foo is architecture: arm64"
        m = re.search(r'architecture:\s+(\S+)', out)
        if m:
            return [m.group(1)]

    # Fallback to 'file'
    rc2, out2 = run_cmd(["/usr/bin/file", path])
    if rc2 == 0:
        # e.g. "Mach-O 64-bit dynamically linked shared library arm64"
        archs = re.findall(r'\b(arm64e?|x86_64)\b', out2)
        if archs:
            return sorted(set(archs))
    return []


_OT_LIB_RE = re.compile(r'^\s+(/.+?)\s+\(')


def deps_from_otool(path: str) -> list[str]:
    """
    Extract absolute dependency paths from 'otool -L'.
    Only returns absolute paths found in the listing.
    """
    rc, out = run_cmd(["/usr/bin/otool", "-L", path])
    deps: list[str] = []
    if rc != 0:
        return deps
    for line in out.splitlines():
        m = _OT_LIB_RE.match(line)
        if m:
            deps.append(m.group(1))
    return deps


def classify_archs(archs: list[str]) -> tuple[str, str]:
    """
    Classify architecture set:
      - OK   : exactly {'arm64'}
      - WARN : contains 'arm64' plus others (e.g., {'arm64', 'arm64e'})
      - FAIL : does not contain 'arm64' (e.g., {'arm64e'} or {'x86_64'})
    Returns (state, reason).
    """
    s = set(archs)
    if not s:
        return "FAIL", "no-arch-detected"
    if s == {"arm64"}:
        return "OK", "pure-arm64"
    if "arm64" in s and (("arm64e" in s) or ("x86_64" in s)):
        return "WARN", "fat-mix:" + ",".join(sorted(s))
    return "FAIL", "unsupported-arch:" + ",".join(sorted(s))


def has_code_signature(path: str) -> bool:
    """
    Check whether Mach-O has LC_CODE_SIGNATURE.
    Note: this indicates a code signature load command exists; it does NOT validate it.
    """
    rc, out = run_cmd(["/usr/bin/otool", "-l", path])
    return (rc == 0 and "LC_CODE_SIGNATURE" in out)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check Mach-O binaries for pure arm64 (no arm64e/x86_64)."
    )
    parser.add_argument("target", help=".app path or a single Mach-O path")
    parser.add_argument(
        "--no-deps", action="store_true",
        help="Do not follow external dependencies (ignore 'otool -L')."
    )
    parser.add_argument(
        "--deps-filter", default=DEFAULT_DEPS_FILTER,
        help=f"Regex for which absolute dependency paths to include (default: {DEFAULT_DEPS_FILTER})"
    )
    parser.add_argument(
        "--show-code-sign", action="store_true",
        help="Also indicate whether LC_CODE_SIGNATURE exists (informational)."
    )
    args = parser.parse_args()

    target = os.path.abspath(args.target)
    is_app = target.endswith(".app") and os.path.isdir(target)

    # Collect Mach-O files in the bundle or single target
    if is_app:
        macho_files = list_bundle_machos(target)
        if not macho_files:
            print(ERRC("No Mach-O files found under .app bundle"), file=sys.stderr)
            return 2
    else:
        if not os.path.exists(target):
            print(ERRC(f"Not found: {target}"), file=sys.stderr)
            return 2
        macho_files = [target]

    # Optionally add external dependencies filtered by regex
    deps_pat = None if args.no_deps else re.compile(args.deps_filter)
    all_targets: set[str] = set(macho_files)
    if deps_pat:
        for mf in list(macho_files):
            for dep in deps_from_otool(mf):
                if deps_pat.search(dep):
                    all_targets.add(dep)

    # Header
    print(DIM("# pureARM64: verify pure arm64 (detect arm64e/x86_64)"))
    print(DIM(f"# target: {target}"))
    print(DIM(f"# deps: {'OFF' if deps_pat is None else 'ON'}"))
    if deps_pat is not None:
        print(DIM(f"# deps-filter: {deps_pat.pattern}"))
    print()

    # Evaluate each file
    warn = 0
    fail = 0
    for p in sorted(all_targets):
        archs = parse_archs_via_lipo(p)
        state, reason = classify_archs(archs)
        head = {"OK": OKC("[OK] "), "WARN": WRNC("[WARN] "), "FAIL": ERRC("[FAIL] ")}[state]
        tail = f"  ({reason})"
        if args.show_code_sign:
            tail += "  " + (DIM("[csig]") if has_code_signature(p) else DIM("[no-sig]"))
        arch_str = ",".join(archs) if archs else "?"
        print(f"{head}{p}\n      arch={arch_str}{tail}")

        if state == "WARN":
            warn += 1
        elif state == "FAIL":
            fail += 1

    # Summary and exit code
    total = len(all_targets)
    ok = total - warn - fail
    print()
    print(f"Summary: {OKC('OK='+str(ok))}, {WRNC('WARN='+str(warn))}, {ERRC('FAIL='+str(fail))}")

    if fail > 0:
        return 2
    if warn > 0:
        return 1
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print(ERRC("\nInterrupted"), file=sys.stderr)
        sys.exit(130)
