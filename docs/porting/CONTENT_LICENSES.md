# The Long Way content permissions and usage policy

This file records project-owner declarations and discovered notices. It is an
inventory template, not a replacement for the original license texts and not
legal advice.

Current porting disposition (2026-09-05): by owner direction, detailed
ownership/provenance work is deferred to the build-prototype stage and is not
a Step 2 gate. Until that work resumes, content-related evaluation is limited
to the authorized `media/` subtree below; the renderer shell remains
content-independent.

## Owner declaration

On 2026-09-05, the project owner/user declared that files below this exact local
path may be used without restriction:

```text
C:\Run3-Game-Engine\Games\The Long Way\TheLongWay\media
```

The same user previously identified themselves as the author of The Long Way.
This declaration covers use for porting reference, evaluation, testing, and
other project work. The owner also requested a stricter engineering policy:
textures, models, sounds, music, and other game assets should be referenced
only exceptionally.

This permission does not make the local `Games/` directory a build dependency
and does not put it into Git. Before publishing a standalone asset package,
preserve the discovered third-party notices and obtain a durable signed or
committed permission statement if the release process requires one.

## Authorized media snapshot

| Property | Value |
|---|---|
| Root | `Games/The Long Way/TheLongWay/media` |
| Files | 246 |
| Size | 22,881,252 bytes |
| Tree manifest SHA-256 | `a556af65053d717176322aa55118d54f2bc9197d1256b738a0f865895de386ec` |
| Case-colliding path groups | 0 |

The most common extensions are 50 PNG, 49 material, 28 JPG, 19 program,
18 mesh, 15 GLSL, 15 HLSL, 10 TGA, 7 DDS, and 6 ZIP files. Use
`python tools/inventory.py --file-hashes` when individual file hashes are
needed; do not commit the generated full listing by default.

The media snapshot also contains `packs/Run3Mat.exe` (299,008 bytes). It was
hashed as inert data and was not executed. Do not use it as a porting or build
dependency; replace any required transformation with a reviewed,
source-available tool in a later explicitly scoped step.

## Exceptional-reference policy

- Public CI, unit tests, and the default build must work without `Games/`.
- Prefer tiny original/synthetic fixtures for regression tests.
- Use a The Long Way asset only when it tests a format, behavior, visual result,
  or compatibility failure that a synthetic fixture cannot represent honestly.
- Reference assets by a user-supplied content root and relative path. Never
  hard-code this machine's absolute path.
- Do not copy, convert, rename, or edit an original during inventory. Later
  conversion tools must write to ignored build/output directories.
- Do not embed game assets in source, test binaries, screenshots, or logs unless
  the exception is recorded below.
- Keep attribution and license notices with any permitted distribution.

## Existing notices discovered beside the game

These files are outside the authorized `media/` subtree but may describe items
inside it. Their exact asset mapping is not yet known, so retain all notices:

| Notice | Reported terms | SHA-256 |
|---|---|---|
| `Docs/kremlin_license.txt` | BoltCutterDesign font/graphics notice; GPL v3-or-later language plus distribution conditions | `a43cee6223aad12366097241e50880af305280d13479e86b5cbf91f73f764da9` |
| `Docs/hydrax_license.txt` | Hydrax copyright notice; LGPL v2-or-later | `4266be39c7bd923719069500b13b1f6ef4a3427b6740bd3721b5f0fe0dd1e5dc` |
| `Docs/gilman_license.txt` | Creative Commons Attribution 2.5 notice and named creators | `9e299c8a073e15b8566940f00c4b594b14e56fb624a946e85bd6fc49d7e4d1dc` |
| `Docs/README.txt` | Run3 v0.73 authorship/component overview | `30824f97ba1902941e477d7f35b107c325e771c3cbf68c2ef15dbbe4c3606483` |
| `readme.txt` | The Long Way/SGL-Team game readme | `cfdc3cdd44e0926051116e840ee5e355df23e393122168a8afde7cbe09d80d5b` |

## Asset provenance template

| Relative path or group | Creator/source | Permission/license | Required attribution | Verified by/date | Notes |
|---|---|---|---|---|---|
| `fonts/` | `<map to notices>` | Owner declaration plus applicable original notice | `<pending>` | `<pending>` | Do not distribute until notice mapping is checked |
| `DeferredShadingMedia/` | `<pending>` | Owner declaration plus applicable upstream terms | `<pending>` | `<pending>` | Likely derived from historical rendering samples; verify provenance |
| `materials/` | Project content/mixed shader sources | Owner declaration; embedded/upstream notices may apply | `<pending>` | `<pending>` | Keep Cg/HLSL/GLSL provenance |
| `models/` | `<pending>` | Owner declaration plus applicable original notices | `<pending>` | `<pending>` | Exceptional reference only |
| `packs/` | `<inspect archive manifests>` | Owner declaration plus contained-file notices | `<pending>` | `<pending>` | Do not infer archive contents from extension alone |

## Asset-use exception log

Complete one row before a port test or tool directly references an original
asset. Empty means the default remains content-independent.

| Relative asset | Purpose | Why a synthetic fixture is insufficient | Consumer/test | Redistribution | Reviewer/date |
|---|---|---|---|---|---|
| `<none yet>` |  |  |  |  |  |
