"""Read-only lighting inventory and opt-in, provenance-tracked derived content.

No operation writes below the source root. Generation refuses existing outputs.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def materials(root):
    result = {}
    for path in sorted(root.rglob('*.material')):
        text = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(errors='replace'), flags=re.S)
        for match in re.finditer(r'\bmaterial\s+([^\s:{]+)(?:\s*:\s*([^\s{]+))?\s*\{', text):
            start, depth, end = match.end(), 1, match.end()
            while depth and end < len(text):
                depth += (text[end] == '{') - (text[end] == '}')
                end += 1
            body = text[start:end - 1]
            result.setdefault(match[1], []).append({
                'file': path.relative_to(root).as_posix(), 'sha256': digest(path),
                'parent': match[2],
                'max_lights': re.findall(r'\bmax_lights\s+(\d+)', body),
                'iterations': re.findall(r'\biteration\s+([^\n{}]+)', body),
                'programs': re.findall(r'\b(?:vertex|fragment)_program_ref\s+(\S+)', body),
                'fixed_light_bindings': re.findall(r'\bparam_named_auto\s+(\S+)\s+(light_position_object_space(?:_array)?)\s+(\d+)', body),
                'texture_aliases': re.findall(r'\bset_texture_alias\s+(\S+)\s+(\S+)', body),
            })
    return result


def inventory(root):
    catalog = materials(root)
    maps = {}
    errors = []
    for path in sorted((root / 'run3/maps').rglob('scene.cfg')):
        references = collections.Counter()
        mesh_names = collections.Counter()
        lights = collections.Counter()
        files = []
        for line in path.read_text(errors='replace').splitlines():
            match = re.match(r'\s*(Scene|Sequence)\s*=\s*(.*?)\s*$', line)
            if not match:
                continue
            source = path.parent / match[2]
            if not source.is_file():
                errors.append({'file': str(source.relative_to(root)), 'error': 'missing'})
                continue
            text = source.read_text(errors='replace')
            # Inventory never repairs malformed source. Lexical counts remain useful.
            try:
                ET.fromstring(text)
            except ET.ParseError as error:
                errors.append({'file': source.relative_to(root).as_posix(), 'error': str(error)})
            files.append({'file': source.relative_to(root).as_posix(), 'sha256': digest(source)})
            for tag in re.findall(r'<light\b[^>]*>', text):
                kind = re.search(r'\btype\s*=\s*["\']([^"\']+)', tag)
                lights[kind[1] if kind else 'point'] += 1
            references.update(re.findall(r'\b(?:materialName|materialFile|material)\s*=\s*["\']([^"\']+)', text))
            mesh_names.update(re.findall(r'\b(?:meshFile|mesh)\s*=\s*["\']([^"\']+\.mesh)', text))
        maps[path.parent.relative_to(root / 'run3/maps').as_posix()] = {
            'files': files, 'lights': dict(lights), 'explicit_material_references': dict(references),
            'mesh_declarations': dict(mesh_names)}
    # Mesh names are newline-terminated Ogre serialized strings, not arbitrary
    # substrings. This is evidence of mesh use, not proof the mesh is map-live.
    mesh_refs = collections.Counter()
    meshes = {}
    names = set(catalog)
    for path in sorted((root / 'run3').rglob('*.mesh')):
        matched = set()
        for item in re.findall(rb'[\x20-\x7e]{2,}\n', path.read_bytes()):
            name = item[:-1].decode('ascii')
            if name in names:
                mesh_refs[name] += 1
                matched.add(name)
        meshes.setdefault(path.name, []).append({'path': path.relative_to(root).as_posix(),
                                                'materials': sorted(matched)})
    live = set()
    for entry in maps.values():
        live.update(entry['explicit_material_references'])
        entry['mesh_material_candidates'] = {
            mesh: meshes.get(mesh, []) for mesh in entry['mesh_declarations']}
        for candidates in entry['mesh_material_candidates'].values():
            for mesh in candidates:
                live.update(mesh['materials'])
    pending = list(live)
    while pending:
        for definition in catalog.get(pending.pop(), []):
            parent = definition['parent']
            if parent and parent not in live:
                live.add(parent)
                pending.append(parent)
    return {'schema': 1, 'materials': catalog, 'maps': maps,
            'mesh_material_references': dict(sorted(mesh_refs.items())),
            'referenced_materials_and_ancestors': sorted(live),
            'xml_exceptions': errors,
            'evidence_rule': 'Explicit XML use, mesh string references and inheritance are separate evidence; mesh presence alone is not map liveness.'}


def derive(root, output):
    root, output = root.resolve(), output.resolve()
    if output == root or root in output.parents or output in root.parents:
        raise ValueError('Output must be outside the original content tree')
    if output.exists():
        raise ValueError('Output already exists; select a new revision directory')
    sources = []
    for folder in ('run3/core', 'run3/maps'):
        sources.extend(p for p in sorted((root / folder).rglob('*')) if p.is_file())
    if not sources:
        raise ValueError('No core/maps files found')
    if any(p.is_symlink() or root not in p.resolve().parents for p in sources):
        raise ValueError('Source symlinks or escaped paths are not allowed')
    manifest = {'schema': 1, 'variant': 'nextgen', 'source_root': str(root),
                'recipe': 'lighting-v0-byte-identical-staging', 'files': []}
    for source in sources:
        relative = source.relative_to(root)
        destination = output / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        original = source.read_bytes()
        transformed = original
        changes = []
        # Revision zero deliberately copies bytes exactly. Numerical tuning is
        # not justified until matched-camera comparisons have been reviewed.
        destination.write_bytes(transformed)
        manifest['files'].append({'path': relative.as_posix(),
                                  'source_sha256': hashlib.sha256(original).hexdigest(),
                                  'output_sha256': digest(destination), 'changes': changes})
    (output / 'remaster-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('operation', choices=('inventory', 'derive'))
    parser.add_argument('--content-root', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    root, output = args.content_root.resolve(), args.output.resolve()
    if root == output or root in output.parents:
        parser.error('Output must be outside the source content root')
    if args.operation == 'inventory':
        result = inventory(root)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(result, indent=2) + '\n')
        print(f"{len(result['materials'])} material names, {len(result['maps'])} map variants, {len(result['xml_exceptions'])} XML exceptions")
    else:
        result = derive(root, output)
        print(f"Staged {len(result['files'])} originals with source/output hashes: {output}")


if __name__ == '__main__':
    main()
