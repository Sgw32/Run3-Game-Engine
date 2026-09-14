#!/usr/bin/env python3
"""Print a deterministic, read-only Run3 source and content inventory.

The script reads Git's tracked-file list and an optional local content tree. It
does not create, modify, copy, convert, or delete any file. Redirecting stdout
to a report is an explicit action performed by the calling shell, not here.
"""

from __future__ import annotations

import argparse
import collections
import dataclasses
import hashlib
import html
import os
from pathlib import Path
import re
import subprocess
import sys
import unicodedata


SOURCE_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".inl",
}

BUILD_DESCRIPTOR_NAMES = {"cmakelists.txt", "cmakepresets.json"}
BUILD_DESCRIPTOR_SUFFIXES = {
    ".bat",
    ".cmd",
    ".cmake",
    ".props",
    ".sh",
    ".sln",
    ".targets",
    ".vcproj",
    ".vcxproj",
}

DEPENDENCIES = {
    "Ogre": (r"#\s*include\s*[<\"][^>\"]*ogre", r"\bogre::"),
    "OIS": (r"#\s*include\s*[<\"][^>\"]*ois", r"\bois::"),
    "Newton / OgreNewt": (r"\bogrenewt\b", r"\bnewton(?:world|body|collision|material|mesh|joint)?\b"),
    "CEGUI": (r"\bcegui\b", r"\bcegui::"),
    "Lua / luabind": (r"\bluabind\b", r"\blua_(?:state|open|close|pcall|call|push|to|get|set|new|register)"),
    "OpenAL / ALUT": (r"#\s*include\s*[<\"](?:al/|openal|alut)", r"\b(?:alut|alsource|albuffer|allistener)\w*"),
    "Audiere": (r"\baudiere\b",),
    "TinyXML 1": (r"\btinyxml\b", r"\btixml\w*"),
    "Hydrax": (r"\bhydrax\b",),
    "SkyX": (r"\bskyx\b",),
    "NVIDIA Cg": (r"#\s*include\s*[<\"][^>\"]*(?:cg/|cg\.h|cgcontext)", r"\bcg(?:context|program|parameter|profile)\b"),
    "DirectShow / WMV": (r"\bdirectshow\b", r"\b(?:igraphbuilder|imediacontrol|imediaevent|strmiids|wmv)\b"),
    "Theora video": (r"\btheora\b",),
    "AIR3": (r"\bair3\b",),
    "Win32 optional devices": (r"\b(?:createnamedpipe|connectnamedpipe|createfilea|createfilew|dcb|commtimeouts)\b", r"#\s*include\s*[<\"]windows\.h"),
}

WINDOWS_ABSOLUTE_PATH = re.compile(
    r"(?<![A-Za-z0-9_])(?:[A-Za-z]:[\\/])[^\"'<>;\r\n]+?"
    r"(?=(?::\s)|[\"'<>;\r\n]|$)"
)
WINDOWS_ROOTED_PATH = re.compile(
    r"(?<![\\A-Za-z0-9_:])\\(?:[A-Za-z][A-Za-z0-9_. !()\-]*\\)+"
    r"[A-Za-z0-9_. !()\\/\-]+"
)
POSIX_ABSOLUTE_PATH = re.compile(
    r"(?<![A-Za-z0-9_:])/(?:home|usr|opt|var|mnt|Users|Applications)/[^\"'<>;\r\n]+"
)


@dataclasses.dataclass(frozen=True)
class FileRecord:
    relative_path: str
    absolute_path: Path
    size: int
    sha256: str


def parse_args() -> argparse.Namespace:
    script_root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=script_root,
        help="Run3 Git worktree (default: script parent worktree)",
    )
    parser.add_argument(
        "--content-root",
        type=Path,
        help=(
            "local content tree; defaults to Games/The Long Way/TheLongWay/media "
            "when present"
        ),
    )
    parser.add_argument(
        "--no-content",
        action="store_true",
        help="inventory only tracked repository files",
    )
    parser.add_argument(
        "--file-hashes",
        action="store_true",
        help="also print the SHA-256 value of every inventoried file",
    )
    parser.add_argument(
        "--max-reference-paths",
        type=int,
        default=12,
        help="sample paths shown per dependency; use 0 to show every path",
    )
    return parser.parse_args()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def make_record(root: Path, relative_path: str) -> FileRecord | None:
    candidate = root.joinpath(*Path(relative_path).parts)
    if candidate.is_symlink() or not candidate.is_file():
        return None
    stat = candidate.stat()
    return FileRecord(relative_path, candidate, stat.st_size, sha256_file(candidate))


def tracked_records(repo_root: Path) -> tuple[list[FileRecord], list[str]]:
    command = [
        "git",
        "-C",
        str(repo_root),
        "ls-files",
        "--recurse-submodules",
        "-z",
    ]
    result = subprocess.run(command, check=True, capture_output=True)
    paths = [
        value.decode("utf-8", errors="surrogateescape")
        for value in result.stdout.split(b"\0")
        if value
    ]

    records: list[FileRecord] = []
    skipped: list[str] = []
    for relative_path in sorted(paths):
        record = make_record(repo_root, relative_path)
        if record is None:
            skipped.append(relative_path)
        else:
            records.append(record)
    return records, skipped


def content_records(content_root: Path) -> tuple[list[FileRecord], list[str]]:
    records: list[FileRecord] = []
    skipped: list[str] = []
    for directory, directory_names, file_names in os.walk(
        content_root, followlinks=False
    ):
        base = Path(directory)
        directory_names[:] = sorted(
            name for name in directory_names if not (base / name).is_symlink()
        )
        for name in sorted(file_names):
            candidate = base / name
            relative_path = candidate.relative_to(content_root).as_posix()
            if candidate.is_symlink():
                skipped.append(relative_path)
                continue
            stat = candidate.stat()
            records.append(
                FileRecord(
                    relative_path,
                    candidate,
                    stat.st_size,
                    sha256_file(candidate),
                )
            )
    records.sort(key=lambda item: item.relative_path)
    return records, skipped


def normalized_case(path: str) -> str:
    return unicodedata.normalize("NFC", path).casefold()


def case_collisions(records: list[FileRecord]) -> list[list[str]]:
    groups: dict[str, list[str]] = collections.defaultdict(list)
    for record in records:
        groups[normalized_case(record.relative_path)].append(record.relative_path)
    return sorted(
        (sorted(paths) for paths in groups.values() if len(paths) > 1),
        key=lambda paths: normalized_case(paths[0]),
    )


def extension_statistics(
    records: list[FileRecord],
) -> list[tuple[str, int, int]]:
    statistics: dict[str, list[int]] = collections.defaultdict(lambda: [0, 0])
    for record in records:
        suffix = Path(record.relative_path).suffix.casefold() or "<none>"
        statistics[suffix][0] += 1
        statistics[suffix][1] += record.size
    return sorted(
        ((suffix, values[0], values[1]) for suffix, values in statistics.items()),
        key=lambda item: (-item[1], item[0]),
    )


def tree_manifest_sha256(records: list[FileRecord]) -> str:
    digest = hashlib.sha256()
    for record in sorted(records, key=lambda item: item.relative_path):
        digest.update(record.relative_path.encode("utf-8", errors="surrogateescape"))
        digest.update(b"\0")
        digest.update(str(record.size).encode("ascii"))
        digest.update(b"\0")
        digest.update(bytes.fromhex(record.sha256))
        digest.update(b"\n")
    return digest.hexdigest()


def format_bytes(value: int) -> str:
    units = ("B", "KiB", "MiB", "GiB", "TiB")
    amount = float(value)
    for unit in units:
        if amount < 1024.0 or unit == units[-1]:
            return f"{amount:.2f} {unit} ({value} bytes)"
        amount /= 1024.0
    raise AssertionError("unreachable")


def print_tree_report(label: str, records: list[FileRecord], skipped: list[str]) -> None:
    print(f"\n## {label}")
    print(f"Files: {len(records)}")
    print(f"Total size: {format_bytes(sum(record.size for record in records))}")
    print(f"Tree manifest SHA-256: {tree_manifest_sha256(records)}")
    print(f"Skipped symlink/non-file entries: {len(skipped)}")
    print("Extensions:")
    for suffix, count, size in extension_statistics(records):
        print(f"  {suffix}: {count} files, {format_bytes(size)}")

    collisions = case_collisions(records)
    print(f"Case-colliding path groups: {len(collisions)}")
    for paths in collisions:
        print("  - " + " | ".join(paths))


def decode_text(path: Path) -> str:
    data = path.read_bytes()
    for encoding in ("utf-8-sig", "windows-1251", "latin-1"):
        try:
            return data.decode(encoding)
        except UnicodeDecodeError:
            continue
    return data.decode("utf-8", errors="replace")


def decode_xml_entities(text: str) -> str:
    for _ in range(3):
        decoded = html.unescape(text)
        if decoded == text:
            return text
        text = decoded
    return text


def project_inventory(repo_root: Path) -> None:
    project_path = repo_root / "Run3.vcproj"
    print("\n## Legacy Visual Studio project")
    if not project_path.is_file():
        print("Run3.vcproj: missing")
        return

    text = decode_xml_entities(decode_text(project_path))
    versions = sorted(set(re.findall(r"\bVersion=\"([^\"]+)\"", text)))
    configurations = re.findall(r"<Configuration\s+Name=\"([^\"]+)\"", text)
    relative_paths = sorted(
        set(re.findall(r"\bRelativePath=\"([^\"]+)\"", text))
    )
    declared_sources = sum(
        Path(value.replace("\\", "/")).suffix.casefold() in SOURCE_SUFFIXES
        for value in relative_paths
    )
    existing = 0
    missing: list[str] = []
    for value in relative_paths:
        normalized = value.replace("\\", "/")
        if normalized.startswith("./"):
            normalized = normalized[2:]
        if (repo_root / normalized).is_file():
            existing += 1
        else:
            missing.append(value)

    print(f"Project format versions: {', '.join(versions) if versions else '<unknown>'}")
    print(f"Configurations: {', '.join(configurations) if configurations else '<none>'}")
    print(f"Unique declared files: {len(relative_paths)}")
    print(f"Declared C/C++ source and header files: {declared_sources}")
    print(f"Declared files present: {existing}")
    print(f"Declared files missing: {len(missing)}")
    for value in missing:
        print(f"  missing: {value}")


def source_dependency_references(
    records: list[FileRecord], max_paths: int
) -> None:
    compiled = {
        name: [re.compile(pattern, re.IGNORECASE) for pattern in patterns]
        for name, patterns in DEPENDENCIES.items()
    }
    matches: dict[str, dict[str, int]] = {
        name: {} for name in DEPENDENCIES
    }

    for record in records:
        if Path(record.relative_path).suffix.casefold() not in SOURCE_SUFFIXES:
            continue
        text = decode_text(record.absolute_path)
        for name, patterns in compiled.items():
            occurrences = sum(len(pattern.findall(text)) for pattern in patterns)
            if occurrences:
                matches[name][record.relative_path] = occurrences

    print("\n## Major legacy dependency references in tracked source")
    for name in DEPENDENCIES:
        paths = matches[name]
        reference_count = sum(paths.values())
        print(f"{name}: {len(paths)} files, {reference_count} references")
        selected = sorted(paths)
        if max_paths > 0:
            selected = selected[:max_paths]
        for path in selected:
            print(f"  {path}: {paths[path]}")
        if max_paths > 0 and len(paths) > max_paths:
            print(f"  ... {len(paths) - max_paths} more files")


def absolute_build_paths(repo_root: Path, records: list[FileRecord], extra: list[Path]) -> None:
    candidates: list[tuple[str, Path]] = []
    for record in records:
        path = Path(record.relative_path)
        if (
            path.name.casefold() in BUILD_DESCRIPTOR_NAMES
            or path.suffix.casefold() in BUILD_DESCRIPTOR_SUFFIXES
        ):
            candidates.append((record.relative_path, record.absolute_path))
    for path in extra:
        if path.is_file():
            try:
                label = path.relative_to(repo_root).as_posix()
            except ValueError:
                label = str(path)
            candidates.append((label, path))

    found: dict[str, list[tuple[str, int]]] = collections.defaultdict(list)
    for label, path in candidates:
        text = decode_text(path)
        if path.suffix.casefold() in {".props", ".targets", ".vcproj", ".vcxproj"}:
            text = decode_xml_entities(text)
        for line_number, line in enumerate(text.splitlines(), start=1):
            drive_spans: list[tuple[int, int]] = []
            for match in WINDOWS_ABSOLUTE_PATH.finditer(line):
                drive_spans.append(match.span())
                found[match.group(0).strip()].append((label, line_number))
            for match in WINDOWS_ROOTED_PATH.finditer(line):
                if any(
                    match.start() >= start and match.end() <= end
                    for start, end in drive_spans
                ):
                    continue
                found[match.group(0).strip()].append((label, line_number))
            for match in POSIX_ABSOLUTE_PATH.finditer(line):
                found[match.group(0).strip()].append((label, line_number))

    print("\n## Absolute paths in build descriptors and legacy logs")
    print(f"Unique paths: {len(found)}")
    print(f"References: {sum(len(locations) for locations in found.values())}")
    for value, locations in sorted(found.items()):
        unique_locations = sorted(set(locations))
        shown = ", ".join(
            f"{label}:{line_number}" for label, line_number in unique_locations[:3]
        )
        if len(unique_locations) > 3:
            shown += f", ... {len(unique_locations) - 3} more locations"
        print(f"  {value} [{len(locations)} references; {shown}]")


def find_legacy_logs(repo_root: Path, content_root: Path | None) -> list[Path]:
    candidates = [repo_root / "Run3.log"]
    if content_root is not None:
        game_root = content_root.parent
        candidates.extend(
            [
                game_root / "Run3.log",
                game_root / "run3" / "core" / "Run3.log",
                game_root / "run3" / "core" / "run3.log",
            ]
        )
    unique: list[Path] = []
    seen: set[tuple[int, int]] = set()
    for candidate in candidates:
        if not candidate.is_file():
            continue
        stat = candidate.stat()
        identity = (stat.st_dev, stat.st_ino)
        if identity not in seen:
            seen.add(identity)
            unique.append(candidate)
    return unique


def print_key_hashes(repo_root: Path, logs: list[Path]) -> None:
    paths = [repo_root / "Run3.vcproj", repo_root / "Run3.sln", *logs]
    print("\n## Key evidence SHA-256 values")
    seen: set[str] = set()
    for path in paths:
        key = os.path.normcase(str(path.absolute()))
        if key in seen or not path.is_file():
            continue
        seen.add(key)
        try:
            label = path.relative_to(repo_root).as_posix()
        except ValueError:
            label = str(path)
        print(f"  {sha256_file(path)}  {label}")


def print_file_hashes(label: str, records: list[FileRecord]) -> None:
    print(f"\n## {label} per-file SHA-256 values")
    for record in records:
        print(f"  {record.sha256}  {record.relative_path}")


def main() -> int:
    args = parse_args()
    repo_root = args.repo_root.resolve()
    if not (repo_root / ".git").exists():
        raise SystemExit(f"not a Git worktree: {repo_root}")
    if args.max_reference_paths < 0:
        raise SystemExit("--max-reference-paths must be zero or greater")

    content_root: Path | None = None
    if not args.no_content:
        requested = args.content_root
        if requested is None:
            requested = (
                repo_root
                / "Games"
                / "The Long Way"
                / "TheLongWay"
                / "media"
            )
        requested = requested.resolve()
        if requested.is_dir():
            content_root = requested

    print("# Run3 read-only inventory")
    print(f"Repository root: {repo_root}")
    print(f"Content root: {content_root if content_root else '<not present or disabled>'}")

    repository_records, repository_skipped = tracked_records(repo_root)
    print_tree_report("Tracked repository files", repository_records, repository_skipped)

    content_file_records: list[FileRecord] = []
    content_skipped: list[str] = []
    if content_root is not None:
        content_file_records, content_skipped = content_records(content_root)
        print_tree_report("Local content files", content_file_records, content_skipped)
    else:
        print("\n## Local content files")
        print("Content tree is absent or disabled; no content files were read.")

    logs = find_legacy_logs(repo_root, content_root)
    print_key_hashes(repo_root, logs)
    project_inventory(repo_root)
    absolute_build_paths(repo_root, repository_records, logs)
    source_dependency_references(repository_records, args.max_reference_paths)

    if args.file_hashes:
        print_file_hashes("Tracked repository", repository_records)
        if content_root is not None:
            print_file_hashes("Local content", content_file_records)

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"inventory failed: {error}", file=sys.stderr)
        raise SystemExit(1) from error
