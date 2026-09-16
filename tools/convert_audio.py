#!/usr/bin/env python3
"""Convert actively referenced legacy music to deterministic derived FLAC files.

The source content is read-only. This tool writes only below --output-root and
never edits a Lua/XML/config reference; proposed replacements remain pending in
the generated manifest until a human listening check is recorded.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

MANIFEST_VERSION = 1
SOURCE_EXTENSIONS = {".ogg", ".xm", ".mod", ".it"}
TEXT_EXTENSIONS = {
    ".cfg", ".ini", ".lua", ".material", ".scene", ".seq", ".txt", ".xml"
}
REFERENCE = re.compile(r"[\"']([^\"'\r\n]+\.(?:ogg|xm|mod|it))[\"']", re.IGNORECASE)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def normalized_reference(value: str) -> str | None:
    value = value.replace("\\", "/").strip()
    path = Path(value)
    if not value or path.is_absolute() or ".." in path.parts:
        return None
    return path.as_posix()


def scan_references(content_root: Path) -> dict[str, list[dict[str, object]]]:
    references: dict[str, list[dict[str, object]]] = {}
    for path in sorted(content_root.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in TEXT_EXTENSIONS:
            continue
        try:
            text = path.read_text(encoding="utf-8-sig", errors="replace")
        except OSError:
            continue
        relative_text = path.relative_to(content_root).as_posix()
        for line_number, line in enumerate(text.splitlines(), 1):
            # A whole-line Lua comment cannot be an active playMusic call.
            if path.suffix.lower() == ".lua" and line.lstrip().startswith("--"):
                continue
            for match in REFERENCE.finditer(line):
                logical = normalized_reference(match.group(1))
                if logical is None or Path(logical).suffix.lower() not in SOURCE_EXTENSIONS:
                    continue
                references.setdefault(logical, []).append(
                    {"file": relative_text, "line": line_number}
                )
    return references


def content_index(content_root: Path) -> dict[str, Path]:
    result: dict[str, Path] = {}
    for path in content_root.rglob("*"):
        if path.is_file():
            result.setdefault(path.relative_to(content_root).as_posix().casefold(), path)
    return result


def ffmpeg_version(program: str) -> str:
    result = subprocess.run(
        [program, "-version"], check=True, capture_output=True, text=True
    )
    return result.stdout.splitlines()[0].strip()


def convert(program: str, source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(destination.stem + ".tmp.flac")
    temporary.unlink(missing_ok=True)
    try:
        subprocess.run(
            [
                program,
                "-nostdin",
                "-hide_banner",
                "-loglevel",
                "error",
                "-y",
                "-i",
                str(source),
                "-map_metadata",
                "0",
                "-fflags",
                "+bitexact",
                "-flags:a",
                "+bitexact",
                "-c:a",
                "flac",
                "-compression_level",
                "8",
                str(temporary),
            ],
            check=True,
        )
        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)


def arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--content-root", required=True, type=Path)
    parser.add_argument(
        "--output-root", type=Path, default=Path("converted-content/audio-flac")
    )
    parser.add_argument("--ffmpeg", default="ffmpeg")
    parser.add_argument(
        "--license-metadata", action="append", type=Path, default=[]
    )
    parser.add_argument(
        "--list-only",
        action="store_true",
        help="inventory references and hashes without invoking ffmpeg or writing output",
    )
    return parser.parse_args()


def main() -> int:
    args = arguments()
    content_root = args.content_root.resolve()
    output_root = args.output_root.resolve()
    if not content_root.is_dir():
        raise SystemExit(f"content root is not a directory: {content_root}")
    if output_root == content_root or content_root in output_root.parents:
        raise SystemExit("output root must not be inside the read-only content root")

    references = scan_references(content_root)
    index = content_index(content_root)
    sources: list[dict[str, object]] = []
    failures = 0
    for logical, sites in sorted(references.items(), key=lambda item: item[0].casefold()):
        # Always use the indexed spelling. Windows reports a differently-cased
        # path as existing, which would otherwise hide a Linux case mismatch.
        source = index.get(logical.casefold())
        if source is None:
            sources.append(
                {"logical_path": logical, "status": "missing", "references": sites}
            )
            failures += 1
            continue
        actual = source.relative_to(content_root).as_posix()
        record: dict[str, object] = {
            "logical_path": logical,
            "source_path": actual,
            "source_sha256": sha256(source),
            "source_size": source.stat().st_size,
            "case_matches": actual == logical,
            "references": sites,
            "proposed_reference": str(Path(logical).with_suffix(".flac")).replace(
                "\\", "/"
            ),
            "reference_update": "pending-listening-check",
        }
        if not args.list_only:
            destination = output_root / Path(actual).with_suffix(".flac")
            convert(args.ffmpeg, source, destination)
            record.update(
                {
                    "output_path": destination.relative_to(output_root).as_posix(),
                    "output_sha256": sha256(destination),
                    "output_size": destination.stat().st_size,
                    "status": "converted",
                }
            )
        else:
            record["status"] = "inventoried"
        sources.append(record)

    license_paths = list(args.license_metadata)
    default_license = Path(__file__).resolve().parents[1] / "docs" / "porting" / "CONTENT_LICENSES.md"
    if not license_paths and default_license.is_file():
        license_paths.append(default_license)
    license_records: list[dict[str, object]] = []
    for license_path in license_paths:
        license_path = license_path.resolve()
        if not license_path.is_file():
            raise SystemExit(f"license metadata is not a file: {license_path}")
        record = {
            "name": license_path.name,
            "sha256": sha256(license_path),
            "size": license_path.stat().st_size,
        }
        if not args.list_only:
            destination = output_root / "licenses" / license_path.name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(license_path, destination)
            record["output_path"] = destination.relative_to(output_root).as_posix()
        license_records.append(record)

    manifest: dict[str, object] = {
        "schema": "run3-audio-conversion",
        "version": MANIFEST_VERSION,
        "source_content_is_read_only": True,
        "content_references_modified": False,
        "reference_update_policy": "Apply only after an author listening check",
        "sources": sources,
        "license_metadata": license_records,
    }
    if not args.list_only:
        manifest["converter"] = ffmpeg_version(args.ffmpeg)
        output_root.mkdir(parents=True, exist_ok=True)
        manifest_path = output_root / "audio-conversion-manifest-v1.json"
        manifest_path.write_text(
            json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
        )

    converted = sum(item["status"] != "missing" for item in sources)
    print(
        f"Run3 audio inventory: {converted} referenced source(s), "
        f"{failures} missing; source content unchanged"
    )
    if args.list_only:
        print(json.dumps(manifest, indent=2, ensure_ascii=False))
    else:
        print(f"Derived FLAC output: {output_root}")
        print("Reference updates remain pending listening checks.")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
