#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
File: "macbuild/bundle_qtconf.py"

Author: ChatGPT + Kazzz-S

Utilities to generate and embed a proper qt.conf into a macOS .app bundle
for KLayout (or any Qt-based app), supporting two strategies:

- ST/HW (Qt embedded in the bundle): relative qt.conf (Prefix=.., Plugins=PlugIns)
- LW (use system-wide Qt): absolute qt.conf (Plugins=<absolute path>)

Policy:
- The bundle creator ("macbuild/build4mac.py") decides the target stack at build time.
- The distributed app contains exactly ONE qt.conf (no scripts post-distribution).

Command-line test usage:
    python bundle_qtconf.py --mode lw --stack macports --qt 5
    python bundle_qtconf.py --app ./dist/klayout.app --mode st --plugins /opt/local/libexec/qt5/plugins

Typical usage:

    from pathlib import Path
    from bundle_qtconf import generate_qtconf, QtConfError

    # 1) LW + MacPorts Qt5 (print-only)
    try:
        text = generate_qtconf(
            mode="lw",
            lw_stack="macports",
            lw_qt_major=5,
        )
        print(text)
    except QtConfError as e:
        print(f"Failed: {e}")

    # 2) LW + Homebrew Qt6 (write into app)
    try:
        text = generate_qtconf(
            app_path="dist/klayout.app",
            mode="lw",
            lw_stack="homebrew",
            lw_qt_major=6,
            arch_hint="arm64",  # "x86_64" for Intel; "auto" works too
        )
        print(text)
    except QtConfError as e:
        print(f"Failed: {e}")

    # 3) LW + Anaconda (Automator-safe: pass explicit prefix if needed)
    try:
        text = generate_qtconf(
            app_path=Path("dist/klayout.app"),
            mode="lw",
            lw_stack="anaconda",
            conda_prefix="/opt/anaconda3",
        )
        print(text)
    except QtConfError as e:
        print(f"Failed: {e}")

    # 4) ST/HW (Qt embedded in the bundle)
    try:
        text = generate_qtconf(
            app_path="dist/klayout.app",
            mode="st",  # or "hw"
            embedded_plugins_src="/opt/local/libexec/qt5/plugins",
            validate=True,
        )
        print(text)
    except QtConfError as e:
        print(f"Failed: {e}")
"""

from __future__ import annotations

import os
import shutil
import subprocess
import argparse
from pathlib import Path
from typing import Iterable, Optional, Tuple, List, Union


class QtConfError(RuntimeError):
    """Raised when qt.conf generation or validation fails."""


# -----------------------------------------------------------------------------
# Utility helpers
# -----------------------------------------------------------------------------
def _app_paths(app_path: Path) -> Tuple[Path, Path, Path]:
    """Return (Resources, PlugIns, MacOS) directories for the .app bundle."""
    app_path = app_path.resolve()
    contents = app_path / "Contents"
    resources = contents / "Resources"
    plugins = contents / "PlugIns"
    macos = contents / "MacOS"
    return resources, plugins, macos


def _ensure_dir(p: Path) -> None:
    p.mkdir(parents=True, exist_ok=True)


def choose_homebrew_root(arch_hint: str = "auto") -> Path:
    """Choose Homebrew prefix based on architecture hint."""
    if arch_hint == "arm64" or (arch_hint == "auto" and Path("/opt/homebrew").is_dir()):
        return Path("/opt/homebrew")
    return Path("/usr/local")


def _is_executable(p: Path) -> bool:
    try:
        return p.is_file() and os.access(str(p), os.X_OK)
    except Exception:
        return False


def _expand_candidates_with_glob(candidates: List[Path]) -> List[Path]:
    expanded: List[Path] = []
    for c in candidates:
        s = str(c)
        if "*" in s or "?" in s or "[" in s:
            try:
                expanded.extend(Path(x) for x in sorted(map(str, c.parent.glob(c.name))))
            except Exception:
                pass
        else:
            expanded.append(c)
    return expanded


def _first_existing_platforms_dir(candidates: List[Path]) -> Optional[Path]:
    for c in _expand_candidates_with_glob(candidates):
        if (c / "platforms").is_dir():
            return c
    return None


def _home_dir() -> Path:
    """Return user's home directory, safe for Automator/launchd environments."""
    try:
        h = os.environ.get("HOME")
        if h:
            return Path(h)
    except Exception:
        pass
    return Path.home()


# -----------------------------------------------------------------------------
# LW plugin dir resolvers (MacPorts / Homebrew / Anaconda)
# -----------------------------------------------------------------------------
def find_plugins_dir_lw(
    lw_stack: str,
    lw_qt_major: Optional[int] = None,
    arch_hint: str = "auto",
    conda_prefix: Optional[Path] = None,
) -> Path:
    """Resolve the absolute Qt plugins directory for LW mode."""
    stack = lw_stack.lower().strip()

    # --- MacPorts ---
    if stack == "macports":
        if lw_qt_major not in (5, 6):
            raise QtConfError("MacPorts requires lw_qt_major to be 5 or 6.")
        return Path(f"/opt/local/libexec/qt{lw_qt_major}/plugins")

    # --- Homebrew ---
    if stack == "homebrew":
        if lw_qt_major not in (5, 6):
            raise QtConfError("Homebrew requires lw_qt_major to be 5 or 6.")
        hb = choose_homebrew_root(arch_hint)

        def _looks_like_qt6(p):
            s = str(p)
            return "/qt6/" in s or s.endswith("/share/qt/plugins") or "/qtbase/" in s

        def _looks_like_qt5(p):
            s = str(p)
            return "/qt@5/" in s or "/qt5/" in s

        candidates: List[Path] = []

        if lw_qt_major == 6:
            # Prefer qtpaths from qt or qtbase (Qt6 split)
            for formula in ("qt", "qtbase"):
                qtpaths_bin = hb / "opt" / formula / "bin" / "qtpaths"
                if _is_executable(qtpaths_bin):
                    try:
                        out = subprocess.check_output([str(qtpaths_bin), "--plugin-dir"], text=True).strip()
                        p = Path(out)
                        if (p / "platforms").is_dir():
                            return p
                    except Exception:
                        pass
            candidates += [
                hb / "opt" / "qt" / "share" / "qt" / "plugins",
                hb / "opt" / "qtbase" / "share" / "qt" / "plugins",
                hb / "Cellar" / "qt" / "*" / "share" / "qt" / "plugins",
                hb / "Cellar" / "qtbase" / "*" / "share" / "qt" / "plugins",
                hb / "opt" / "qt" / "lib" / "qt6" / "plugins",
                hb / "opt" / "qt" / "plugins",
            ]
            found = _first_existing_platforms_dir(candidates)
            if found and _looks_like_qt6(found):
                return found
        else:
            qtpaths_bin = hb / "opt" / "qt@5" / "bin" / "qtpaths"
            if _is_executable(qtpaths_bin):
                try:
                    out = subprocess.check_output([str(qtpaths_bin), "--plugin-dir"], text=True).strip()
                    p = Path(out)
                    if (p / "platforms").is_dir() and _looks_like_qt5(p):
                        return p
                except Exception:
                    pass
            candidates += [
                hb / "opt" / "qt@5" / "plugins",
                hb / "opt" / "qt@5" / "lib" / "qt5" / "plugins",
                hb / "Cellar" / "qt@5" / "*" / "plugins",
                hb / "Cellar" / "qt@5" / "*" / "lib" / "qt5" / "plugins",
            ]
            for c in _expand_candidates_with_glob(candidates):
                if (c / "platforms").is_dir() and _looks_like_qt5(c):
                    return c

        raise QtConfError(
            f"Homebrew Qt{lw_qt_major} plugins not found under {hb}. Checked: "
            + ", ".join(str(p) for p in _expand_candidates_with_glob(candidates))
        )

    # --- Anaconda / Miniconda / Mambaforge / Miniforge ---
    if stack == "anaconda":
        def _env_plugins_candidates(env_root, qt_major):
            if qt_major == 6:
                return [Path(env_root) / "lib" / "qt6" / "plugins"]
            else:
                return [Path(env_root) / "plugins"]

        def _base_preferred_envs(base_root, qt_major):
            names = ["klayout-qt6"] if qt_major == 6 else ["klayout-qt5"]
            env_roots = [Path(base_root) / "envs" / n for n in names]
            cands = []
            for er in env_roots:
                cands.extend(_env_plugins_candidates(er, qt_major))
            return cands

        def _scan_all_envs(base_root, qt_major):
            cands = []
            envs_dir = Path(base_root) / "envs"
            if envs_dir.is_dir():
                for er in sorted(envs_dir.iterdir()):
                    if not er.is_dir():
                        continue
                    n = er.name.lower()
                    if qt_major == 6 and "qt6" in n:
                        cands.extend(_env_plugins_candidates(er, 6))
                    elif qt_major == 5 and "qt5" in n:
                        cands.extend(_env_plugins_candidates(er, 5))
                for er in sorted(envs_dir.iterdir()):
                    if er.is_dir():
                        cands.extend(_env_plugins_candidates(er, qt_major))
            return cands

        def _base_generic_candidates(base_root):
            return [
                Path(base_root) / "plugins",
                Path(base_root) / "lib" / "qt" / "plugins",
                Path(base_root) / "lib" / "qt5" / "plugins",
                Path(base_root) / "lib" / "qt6" / "plugins",
            ]

        qt_major = lw_qt_major or 6

        roots: List[Path] = []
        if conda_prefix:
            roots.append(Path(conda_prefix))
        env_prefix = os.environ.get("CONDA_PREFIX", "")
        if env_prefix:
            roots.append(Path(env_prefix))
        home = _home_dir()
        roots += [
            Path("/opt/anaconda3"),
            Path("/usr/local/anaconda3"),
            home / "opt" / "anaconda3",
            home / "anaconda3",
            Path("/opt/miniconda3"),
            Path("/usr/local/miniconda3"),
            home / "miniconda3",
            Path("/opt/mambaforge"),
            home / "mambaforge",
            Path("/opt/miniforge3"),
            home / "miniforge3",
            Path("/Applications/anaconda3"),
            Path("/Applications/miniconda3"),
            Path("/Applications/mambaforge"),
            Path("/Applications/miniforge3"),
        ]

        plugin_candidates: List[Path] = []

        if conda_prefix:
            cp = Path(conda_prefix)
            if (cp / "conda-meta").is_dir() and not (cp / "envs").is_dir():
                plugin_candidates.extend(_env_plugins_candidates(cp, qt_major))

        for base in roots:
            b = Path(base)
            try:
                b = b.resolve()
            except Exception:
                pass
            plugin_candidates.extend(_base_preferred_envs(b, qt_major))
            plugin_candidates.extend(_scan_all_envs(b, qt_major))
            plugin_candidates.extend(_base_generic_candidates(b))

        # Highest priority: Intel GUI installer layout
        apps_direct = Path("/Applications/anaconda3/plugins")
        if apps_direct.exists():
            plugin_candidates.insert(0, apps_direct)

        found = _first_existing_platforms_dir(plugin_candidates)
        if found:
            return found

        raise QtConfError(
            "Anaconda plugins not found. Checked: "
            + ", ".join(str(p) for p in _expand_candidates_with_glob(plugin_candidates))
        )

    raise QtConfError(f"Unknown lw_stack: {lw_stack}")


# -----------------------------------------------------------------------------
# Core functions
# -----------------------------------------------------------------------------
def _validate_libqcocoa(plugins_dir: Path) -> None:
    """Ensure libqcocoa.dylib exists under <plugins_dir>/platforms."""
    lib = plugins_dir / "platforms" / "libqcocoa.dylib"
    if not lib.is_file():
        raise QtConfError(f"libqcocoa.dylib not found: {lib}")


def copy_embedded_plugins(
    embedded_plugins_src: Path,
    bundle_plugins_dir: Path,
    subdirs: Iterable[str] = ("platforms",),
    overwrite: bool = True,
) -> None:
    """Copy selected plugin subdirectories into the bundle."""
    embedded_plugins_src = embedded_plugins_src.resolve()
    bundle_plugins_dir = bundle_plugins_dir.resolve()
    _ensure_dir(bundle_plugins_dir)
    for d in subdirs:
        src = embedded_plugins_src / d
        dst = bundle_plugins_dir / d
        if not src.is_dir():
            raise QtConfError(f"Missing plugin subdir at source: {src}")
        if dst.exists() and overwrite:
            shutil.rmtree(dst)
        shutil.copytree(src, dst)


def make_qtconf_text_relative() -> str:
    """Return relative qt.conf text for ST/HW bundles."""
    return (
        "[Paths]\n"
        "Prefix=..\n"
        "Plugins=PlugIns\n"
        "# Uncomment if QML is embedded:\n"
        "# Imports=Resources/qml\n"
        "# Qml2Imports=Resources/qml\n"
    )


def make_qtconf_text_absolute(plugins_dir: Path) -> str:
    """Return absolute qt.conf text for LW bundles."""
    return f"[Paths]\nPlugins={plugins_dir}\n"


def generate_qtconf(
    app_path: Optional[Union[str, Path]] = None,
    *,
    mode: str,
    embedded_plugins_src: Optional[Union[str, Path]] = None,
    lw_stack: Optional[str] = None,
    lw_qt_major: Optional[int] = None,
    arch_hint: str = "auto",
    conda_prefix: Optional[Union[str, Path]] = None,
    validate: bool = True,
) -> str:
    """Generate qt.conf content (and optionally write it to the bundle)."""
    app_path_p: Optional[Path] = Path(app_path).resolve() if app_path else None
    qtconf_text: str

    if mode in ("st", "hw"):
        qtconf_text = make_qtconf_text_relative()
        if app_path_p:
            resources, plugins, _macos = _app_paths(app_path_p)
            _ensure_dir(resources)
            if embedded_plugins_src:
                copy_embedded_plugins(Path(embedded_plugins_src), plugins)
                if validate:
                    _validate_libqcocoa(plugins)
            (resources / "qt.conf").write_text(qtconf_text, encoding="utf-8")

    elif mode == "lw":
        if lw_stack is None:
            raise QtConfError("lw_stack is required for LW mode (macports|homebrew|anaconda).")
        plugins_dir = find_plugins_dir_lw(
            lw_stack=lw_stack,
            lw_qt_major=lw_qt_major,
            arch_hint=arch_hint,
            conda_prefix=Path(conda_prefix) if conda_prefix else None,
        )
        if validate:
            _validate_libqcocoa(plugins_dir)
        qtconf_text = make_qtconf_text_absolute(plugins_dir)
        if app_path_p:
            resources, _, _ = _app_paths(app_path_p)
            _ensure_dir(resources)
            (resources / "qt.conf").write_text(qtconf_text, encoding="utf-8")

    else:
        raise QtConfError(f"Unknown mode: {mode}")

    return qtconf_text


# -----------------------------------------------------------------------------
# CLI for testing
# -----------------------------------------------------------------------------
def main() -> None:
    """Standalone CLI for testing or dry-run output."""
    parser = argparse.ArgumentParser(description="Generate qt.conf or print its content.")
    parser.add_argument("--app", help="Path to the .app bundle (optional; if omitted, only print the content)")
    parser.add_argument("--mode", choices=["st", "hw", "lw"], required=True, help="Bundle mode")
    parser.add_argument("--stack", choices=["macports", "homebrew", "anaconda"], help="LW: Qt stack type")
    parser.add_argument("--qt", type=int, choices=[5, 6], help="LW: Qt major version (5 or 6)")
    parser.add_argument("--arch", default="auto", choices=["auto", "arm64", "x86_64"], help="LW: arch hint for Homebrew")
    parser.add_argument("--plugins", help="ST/HW: source path of Qt plugins")
    parser.add_argument("--no-validate", action="store_true", help="Skip validation of libqcocoa.dylib existence")
    parser.add_argument("--conda-prefix", help="LW(Anaconda) only: explicit CONDA_PREFIX to use")

    args = parser.parse_args()

    try:
        qtconf_text = generate_qtconf(
            app_path=args.app,
            mode=args.mode,
            embedded_plugins_src=args.plugins,
            lw_stack=args.stack,
            lw_qt_major=args.qt,
            arch_hint=args.arch,
            conda_prefix=args.conda_prefix,
            validate=not args.no_validate,
        )
        if args.app:
            print(f"[OK] qt.conf written to bundle: {args.app}")
        print("----- qt.conf content -----")
        print(qtconf_text.strip())
        print("---------------------------")
    except QtConfError as e:
        print(f"[ERROR] {e}")
        raise SystemExit(1)


if __name__ == "__main__":
    main()
