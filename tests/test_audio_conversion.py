#!/usr/bin/env python3

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class AudioConversionInventoryTest(unittest.TestCase):
    def test_list_only_finds_active_reference_without_writing(self) -> None:
        tool = Path(sys.argv[1]).resolve()
        with tempfile.TemporaryDirectory(prefix="run3-audio-conversion-") as root:
            content = Path(root) / "content"
            output = Path(root) / "output"
            (content / "run3" / "sounds").mkdir(parents=True)
            source = content / "run3" / "sounds" / "Theme.XM"
            source.write_bytes(b"fixture tracker bytes")
            scripts = content / "run3" / "lua"
            scripts.mkdir(parents=True)
            (scripts / "active.lua").write_text(
                'playMusic("run3/sounds/theme.xm", "true")\n'
                '-- playMusic("run3/sounds/ignored.mod", "true")\n',
                encoding="utf-8",
            )
            before = source.read_bytes()
            result = subprocess.run(
                [
                    sys.executable,
                    str(tool),
                    "--content-root",
                    str(content),
                    "--output-root",
                    str(output),
                    "--list-only",
                ],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            manifest = json.loads(result.stdout[result.stdout.index("{") :])
            self.assertEqual(len(manifest["sources"]), 1)
            self.assertEqual(manifest["sources"][0]["source_path"],
                             "run3/sounds/Theme.XM")
            self.assertFalse(manifest["sources"][0]["case_matches"])
            self.assertEqual(manifest["sources"][0]["reference_update"],
                             "pending-listening-check")
            self.assertEqual(source.read_bytes(), before)
            self.assertFalse(output.exists())


if __name__ == "__main__":
    # Keep the tool path out of unittest's own argument parser.
    tool_argument = sys.argv[1]
    sys.argv[:] = [sys.argv[0], tool_argument]
    unittest.main(argv=[sys.argv[0]])
