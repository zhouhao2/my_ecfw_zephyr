#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Apply platform patches shipped with my_ecfw_zephyr onto zephyr/ and ls_sdk/.

Usage (from west workspace root, or from this repo):
  python my_ecfw_zephyr/scripts/apply_patches.py
  python my_ecfw_zephyr/scripts/apply_patches.py --check
  python my_ecfw_zephyr/scripts/apply_patches.py --reverse
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def find_workspace(start: Path) -> Path:
    """Resolve west workspace root containing zephyr/ and ls_sdk/."""
    for p in [start, *start.parents]:
        if (p / "zephyr").is_dir() and (p / "ls_sdk").is_dir():
            return p
        if (p / ".west").is_dir() and (p / "zephyr").is_dir():
            return p
    raise SystemExit(
        f"Cannot find workspace with zephyr/ + ls_sdk/ above {start}"
    )


def patch_files(patches_root: Path, name: str) -> list[Path]:
    d = patches_root / name
    if not d.is_dir():
        return []
    return sorted(d.glob("*.patch"))


def git_apply(repo: Path, patch: Path, reverse: bool, check: bool) -> int:
    # Windows checkouts may turn *.patch into CRLF; strip CR into a temp file.
    raw = patch.read_bytes()
    normalized = raw.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    use_path = patch
    tmp: Path | None = None
    if normalized != raw:
        tmp = patch.with_suffix(patch.suffix + ".lf.tmp")
        tmp.write_bytes(normalized)
        use_path = tmp
        print(f"  note: normalized CRLF -> LF for apply ({patch.name})")

    cmd = ["git", "-C", str(repo), "apply", "--whitespace=nowarn", "--ignore-whitespace"]
    if check:
        cmd.append("--check")
    if reverse:
        cmd.append("--reverse")
    cmd.append(str(use_path))
    print(" ", " ".join(cmd))
    try:
        r = subprocess.run(cmd)
        return r.returncode
    finally:
        if tmp is not None:
            tmp.unlink(missing_ok=True)


def already_applied(repo: Path, patch: Path) -> bool:
    """True if reverse --check succeeds (patch content already in tree)."""
    raw = patch.read_bytes()
    normalized = raw.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    use_path = patch
    tmp: Path | None = None
    if normalized != raw:
        tmp = patch.with_suffix(patch.suffix + ".lf.tmp")
        tmp.write_bytes(normalized)
        use_path = tmp
    try:
        r = subprocess.run(
            [
                "git",
                "-C",
                str(repo),
                "apply",
                "--reverse",
                "--check",
                "--whitespace=nowarn",
                "--ignore-whitespace",
                str(use_path),
            ],
            capture_output=True,
        )
        return r.returncode == 0
    finally:
        if tmp is not None:
            tmp.unlink(missing_ok=True)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--workspace",
        type=Path,
        default=None,
        help="West workspace root (default: auto-detect)",
    )
    ap.add_argument(
        "--check",
        action="store_true",
        help="Only verify patches apply cleanly",
    )
    ap.add_argument(
        "--reverse",
        action="store_true",
        help="Remove previously applied patches",
    )
    ap.add_argument(
        "--force",
        action="store_true",
        help="Apply even if reverse-check says already applied",
    )
    args = ap.parse_args()

    script_dir = Path(__file__).resolve().parent
    app_root = script_dir.parent
    patches_root = app_root / "patches"
    ws = args.workspace.resolve() if args.workspace else find_workspace(app_root)

    mapping = {
        "zephyr": ws / "zephyr",
        "ls_sdk": ws / "ls_sdk",
    }

    print(f"workspace: {ws}")
    print(f"patches:   {patches_root}")

    rc = 0
    for name, repo in mapping.items():
        if not repo.is_dir():
            print(f"SKIP {name}: missing {repo}")
            rc = 1
            continue
        files = patch_files(patches_root, name)
        if not files:
            print(f"SKIP {name}: no patches")
            continue
        print(f"\n[{name}] -> {repo}")
        for patch in files:
            print(f"  patch: {patch.name}")
            if not args.reverse and not args.check and not args.force:
                if already_applied(repo, patch):
                    print("  already applied — skip")
                    continue
            code = git_apply(repo, patch, reverse=args.reverse, check=args.check)
            if code != 0:
                print(f"  FAILED ({code})")
                rc = code
            else:
                print("  OK")
    return rc


if __name__ == "__main__":
    sys.exit(main())
