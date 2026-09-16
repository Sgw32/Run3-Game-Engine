#!/usr/bin/env python3
"""Read-only legacy Lua/API inventory and deterministic catalog generator."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
from collections import Counter
from pathlib import Path


SOURCE_SUFFIXES = {".cpp", ".h", ".hpp"}
EXCLUDED_PARTS = {
    ".git", "build", "Games", "OgreSDK", "Run3Dep",
    # Step 4+ code is not part of the Lua 5.0 baseline being inventoried.
    "cmake", "include", "source", "tests", "tools",
}
REGISTER = re.compile(
    r'lua_register\s*\(\s*[^,]+,\s*"([^"]+)"\s*,\s*'
    r'([A-Za-z_][A-Za-z0-9_]*)\s*\)',
    re.DOTALL,
)
C_API = re.compile(
    r"\b(lua(?:L)?_[A-Za-z0-9_]+|luaopen_[A-Za-z0-9_]+)\s*\("
)
LUABIND = re.compile(r"\bluabind\b|#\s*include\s*[<\"][^>\"]*luabind", re.I)
CALL = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")


def source_files(root: Path) -> list[Path]:
    tracked = subprocess.run(
        ["git", "-C", str(root), "ls-files", "-z"],
        check=True,
        capture_output=True,
    ).stdout.decode("utf-8", errors="surrogateescape").split("\0")
    return sorted(
        root / relative
        for relative in tracked
        if relative
        and (root / relative).is_file()
        and (root / relative).suffix.lower() in SOURCE_SUFFIXES
        and not any(part in EXCLUDED_PARTS for part in Path(relative).parts)
    )


def group_for(name: str) -> str:
    lower = name.lower()
    if lower.startswith(("d", "buttongui_", "hud", "cursor_")):
        return "ui"
    if lower.startswith(("playmusic", "togglemusic", "stopmusic", "fadeoutmusic",
                         "setmusic", "emit", "activateMP".lower(),
                         "deactivateMP".lower(), "setmpreg", "addambient",
                         "enableambient", "disableambient")):
        return "audio"
    if lower.startswith("player_") or lower in {"teleport", "teleport_rel", "godmode"}:
        return "player"
    if lower.startswith(("npc", "__all_npc", "destroynpc", "commandleader")):
        return "npc"
    if lower.startswith(("skyx_", "setsky", "dssao", "setcompositor")):
        return "rendering"
    if lower.startswith(("comsend", "turnoff__")):
        return "devices"
    if any(token in lower for token in ("door", "train", "trigger", "timer")):
        return "sequence"
    if lower.startswith(("bod__", "nod__", "freeze", "unfreeze", "transformtophys")):
        return "physics"
    return "world"


def line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def inventory(root: Path, content_root: Path | None) -> dict:
    api_calls: Counter[str] = Counter()
    api_sites: list[dict] = []
    exports: list[dict] = []
    luabind_sites: list[dict] = []
    for path in source_files(root):
        text = path.read_text(encoding="utf-8", errors="replace")
        relative = path.relative_to(root).as_posix()
        for match in C_API.finditer(text):
            name = match.group(1)
            api_calls[name] += 1
            api_sites.append(
                {
                    "api": name,
                    "source": relative,
                    "line": line_number(text, match.start()),
                }
            )
        for match in REGISTER.finditer(text):
            exports.append(
                {
                    "name": match.group(1),
                    "signature": "int(lua_State*)",
                    "callback": match.group(2),
                    "group": group_for(match.group(1)),
                    "source": relative,
                    "line": line_number(text, match.start()),
                }
            )
        for match in LUABIND.finditer(text):
            luabind_sites.append(
                {"source": relative, "line": line_number(text, match.start())}
            )

    exports.sort(key=lambda item: (item["name"], item["source"], item["line"]))
    api_sites.sort(key=lambda item: (item["source"], item["line"], item["api"]))
    duplicate_names = sorted(
        name for name, count in Counter(item["name"] for item in exports).items()
        if count > 1
    )

    script_files: list[Path] = []
    script_calls: Counter[str] = Counter()
    if content_root and content_root.is_dir():
        script_files = sorted(content_root.rglob("*.lua"))
        export_names = {item["name"] for item in exports}
        for path in script_files:
            text = path.read_text(encoding="utf-8", errors="replace")
            script_calls.update(name for name in CALL.findall(text) if name in export_names)

    result = {
        "schema_version": 1,
        "legacy_runtime": "Lua 5.0 C API",
        "target_runtime": "Lua 5.4.8 + sol2 3.5.0#1",
        "source_files_scanned": len(source_files(root)),
        "c_api_calls": dict(sorted(api_calls.items())),
        "c_api_sites": api_sites,
        "luabind_registration_sites": luabind_sites,
        "exports": exports,
        "duplicate_export_names": duplicate_names,
        "content_script_count": len(script_files),
        "content_export_call_counts": dict(sorted(script_calls.items())),
    }
    payload = json.dumps(result, ensure_ascii=False, sort_keys=True,
                         separators=(",", ":")).encode("utf-8")
    result["inventory_sha256"] = hashlib.sha256(payload).hexdigest()
    return result


def write_catalog(path: Path, data: dict) -> None:
    lines = [
        "// Generated by tools/script_api_inventory.py; do not hand edit.",
        "// group, exported name, legacy callback, source, line",
    ]
    for item in data["exports"]:
        lines.append(
            'RUN3_SCRIPT_BINDING("{}", "{}", "{}", "{}", {})'.format(
                item["group"], item["name"], item["callback"],
                item["source"], item["line"]
            )
        )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def write_snapshot(path: Path, data: dict) -> None:
    lines = [
        "# Run3 Lua exported API v1",
        f"count={len(data['exports'])}",
    ]
    for item in data["exports"]:
        lines.append(
            "{} | {} | {} | {}".format(
                item["group"], item["name"], item["signature"],
                item["callback"]
            )
        )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path)
    parser.add_argument("--content-root", type=Path)
    parser.add_argument("--json", type=Path, required=True)
    parser.add_argument("--catalog", type=Path, required=True)
    parser.add_argument("--snapshot", type=Path, required=True)
    args = parser.parse_args()

    root = args.root.resolve()
    data = inventory(root, args.content_root.resolve() if args.content_root else None)
    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n",
                         encoding="utf-8", newline="\n")
    write_catalog(args.catalog, data)
    write_snapshot(args.snapshot, data)
    print(
        f"exports={len(data['exports'])} c_api={len(data['c_api_calls'])} "
        f"luabind={len(data['luabind_registration_sites'])} "
        f"scripts={data['content_script_count']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
