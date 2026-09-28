"""Small provenance fixtures: no dependency on author-owned content."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('lighting_content', Path(__file__).parents[1] / 'tools/lighting_content.py')
lighting = importlib.util.module_from_spec(spec)
spec.loader.exec_module(lighting)


class ProvenanceTests(unittest.TestCase):
    def test_derived_copy_is_exact_and_scoped(self):
        with tempfile.TemporaryDirectory(prefix='run3-lighting-') as directory:
            root = Path(directory) / 'original'
            for name in ('run3/core/m.cfg', 'run3/maps/test/scene.xml', 'run3/lua/untouched.lua'):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'<scene/>\r\n')
            before = {p.relative_to(root): lighting.digest(p) for p in root.rglob('*') if p.is_file()}
            derived = Path(directory) / 'derived'
            result = lighting.derive(root, derived)
            self.assertEqual(len(result['files']), 2)
            for entry in result['files']:
                self.assertEqual(entry['source_sha256'], entry['output_sha256'])
            self.assertFalse((derived / 'run3/lua').exists())
            self.assertEqual(before, {p.relative_to(root): lighting.digest(p) for p in root.rglob('*') if p.is_file()})
            with self.assertRaises(ValueError):
                lighting.derive(root, derived)
            with self.assertRaises(ValueError):
                lighting.derive(root, root / 'derived')


if __name__ == '__main__':
    unittest.main()
