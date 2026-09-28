"""Bounded installed LightingLab runs; fails on shader errors, not just exit code."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import time
from PIL import Image, ImageStat


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--renderers', nargs='+', default=['d3d11', 'gl3plus'])
    parser.add_argument('--pipelines', nargs='+', default=['legacy-forward', 'deferred', 'pbr', 'fast-forward'])
    parser.add_argument('--frames', default=60, type=int)
    parser.add_argument('--shadow-quality', default='off')
    parser.add_argument('--resolution', default='1280x720')
    parser.add_argument('--fullscreen', action='store_true')
    parser.add_argument('--resize', action='store_true')
    parser.add_argument('--map')
    parser.add_argument('--content-root', type=Path)
    parser.add_argument('--content-overlay', type=Path)
    parser.add_argument('--reload', action='store_true')
    parser.add_argument('--timeout', type=float, default=240)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    results = []
    for renderer in args.renderers:
        for pipeline in args.pipelines:
            case = output / f'{renderer}-{pipeline}'
            if case.exists():
                parser.error(f'Refusing to mix old and new screenshots: {case}')
            case.mkdir()
            command = [str(args.executable.resolve()), '--renderer', renderer,
                       '--lighting-pipeline', pipeline,
                       '--shadow-quality', args.shadow_quality, '--frames', str(args.frames),
                       '--lighting-capture', '--audio-backend', 'null', '--resolution', args.resolution,
                       '--user-dir', str(case), '--fullscreen' if args.fullscreen else '--windowed']
            if args.map:
                if not args.content_root:
                    parser.error('--map needs --content-root')
                command += ['--map', args.map, '--content-root', str(args.content_root.resolve()),
                            '--scene-quality', 'high', '--texture-quality', 'high']
            else:
                command += ['--lighting-lab']
            if args.content_overlay:
                command += ['--content-overlay', str(args.content_overlay.resolve())]
            if args.resize:
                command += ['--lighting-resize']
            if args.reload:
                command += ['--lighting-reload']
            start = time.monotonic()
            try:
                run = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                     timeout=args.timeout, text=True, errors='replace')
                code, log = run.returncode, run.stdout
            except subprocess.TimeoutExpired as error:
                code = -1
                captured = error.stdout or b''
                if isinstance(captured, bytes):
                    captured = captured.decode('utf-8', errors='replace')
                log = captured + '\n' + str(error)
            (case / 'process.log').write_text(log, encoding='utf-8')
            errors = re.findall(r'^.*(?:\[Run3 error\]|error [XC]\d+|Error compiling|failed to compile|compile error|ERROR:).*$', log, re.M)
            screenshot = case / 'logs/lighting.png'
            result = {'renderer': renderer, 'pipeline': pipeline, 'exit': code,
                      'wall_seconds_including_startup': time.monotonic()-start,
                      'shader_errors': errors, 'command': command,
                      'passed': code == 0 and not errors and screenshot.is_file()}
            if screenshot.is_file():
                result['screenshot_sha256'] = hashlib.sha256(screenshot.read_bytes()).hexdigest()
                with Image.open(screenshot) as capture:
                    deviation = ImageStat.Stat(capture.convert('RGB')).stddev
                result['image_channel_stddev'] = deviation
                # An exit code and PNG alone previously let a blank deferred
                # target pass. This is a sanity gate, not visual-parity proof.
                result['image_nonuniform'] = max(deviation) >= 2.0
                result['passed'] &= result['image_nonuniform']
            report = case / 'logs/lighting.json'
            if report.is_file():
                result['measurements'] = json.loads(report.read_text())
            results.append(result)
            print(f'{renderer}/{pipeline}: {"PASS" if result["passed"] else "FAIL"}', flush=True)
    (output / 'results.json').write_text(json.dumps(results, indent=2)+'\n')
    return 0 if all(row['passed'] for row in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
