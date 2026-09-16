# Run3 audio migration

Step 7 replaces the two historical audio paths with one Run3-owned contract.
Gameplay includes `run3/audio/Audio.hpp`, not miniaudio, OpenAL, ALUT, Audiere,
or oalufmod headers.

## Runtime contract and ownership

`IAudioEngine` exposes typed move-only `SoundHandle` values; master, music,
effects, and voice buses; listener position/velocity/orientation; play, stop,
pause, loop, gain, pitch, source position/velocity, attenuation distances,
fades, streaming, seeking, state, and frame update. A handle owns one bounded
voice slot and releases it on destruction. Generation-tagged tokens prevent a
stale handle from addressing a reused slot. The default capacity is 32 voices
and can be changed through `AudioEngineConfig`.

`SoundRuntime` replaces the map-scoped fixed arrays in `Run3SoundRuntime` and
releases every effect on map clear/destruction. `MusicPlayer` replaces the
Audiere singleton with one streaming RAII handle and explicit loop, fade,
transition, gain, pitch, seek, and cleanup behavior.

The deterministic null backend implements the same state/lifetime contract
without opening or decoding a device. The production backend is
[miniaudio 0.11.25](https://github.com/mackron/miniaudio/tree/0.11.25), pinned
by the vcpkg manifest. `miniaudio.h` and `MINIAUDIO_IMPLEMENTATION` are private
to `run3_audio_miniaudio`; the implementation macro exists in one source file.
The pinned vcpkg port verifies the upstream archive with SHA-512
`8cdfe5cd66dd84628430a24026b307c21158b4776492eec234c2ce3cf0da3ae26fe8162f3ed285502f6002fdf252ccb660f7c216e044e3c306b75b0997700b45`.
If construction throws, `createAudioEngineWithFallback` logs the reason and
returns the null backend, so audio failure cannot stop gameplay.

## Playable map bridge

`MapAudioRuntime` is the first live bridge from authored map data to the new
backend. It reads the scene selected by `scene.cfg`, accepts the legacy quoted
and unquoted attribute syntax, applies the scene multiplier, and starts
unnamed `<ambient>` entries as spatial effects with authored loop and distance
values. It also reads the active literal `playMusic`/`setMusicVolume` calls in
the chapter startup file without executing Lua. In `tlwcao`, this starts nine
ambient sources and streams `run3/sounds/machining.mp3`.

Named `objname` ambient sources are event-controlled. They are counted and
logged but deliberately deferred instead of starting alarms, radios, and
scripted effects prematurely. Full Lua/sequence control remains Step 8.

The player bridge alternates the four authored `concrete1.wav` through
`concrete4.wav` samples while the capsule is grounded and moving. Walking and
running use separate cadences, and airborne/noclip movement is silent. Surface
trigger selection remains Step 8; `concrete` is the documented compatibility
default for the currently playable `tlwcao` slice.

`tests/fixtures/audio_map` is a text-only miniature map. It validates spatial
source parsing, coordinate multiplier handling, named-source deferral, startup
music, footstep cadence/noclip behavior, and unload cleanup through the null
backend. An optional attached-content test verifies the real `tlwcao` files
and references. Listening instructions are in
[RUNNING.md](../RUNNING.md#audible-tlwcao-test).

The pinned miniaudio resource-manager initialization-failure path has an
upstream use-after-free, reproduced with a missing buffered file. The adapter
preflights each filesystem source with `ma_decoder_init_file`; missing or
corrupt input is rejected before it can reach that path. The guard is covered
by ASan/UBSan.

## Coordinates and old-stack boundary

Run3 and Ogre use +Y up and a forward vector toward -Z. The old OpenAL path
passed position, velocity, forward, and up components unchanged. All miniaudio
source/listener writes now pass through the single `fromGameCoordinates`
boundary (currently identity). Distance values stay in game units, matching
the existing listener/source convention.

`SoundManager.cpp`, `Run3SoundRuntime.cpp`, and the old `MusicPlayer.cpp` are
explicit retired entries in the reviewed vcproj inventory. They and their
headers remain as historical behavior evidence but are absent from every live
target. The live repository boundary test rejects OpenAL/ALUT/Audiere headers,
libraries, oalufmod, and the removed legacy-audio feature gate.

## Offline content conversion

`tools/convert_audio.py` scans quoted active OGG/XM/MOD/IT references, resolves
their real case, hashes every input, invokes FFmpeg to a temporary output, and
atomically places FLAC results below `converted-content/audio-flac/`. It copies
the content-license inventory and writes
`audio-conversion-manifest-v1.json` with converter, input/output paths, sizes,
and SHA-256 values. Originals are read-only and game references are never
edited by the tool.

The 2026-09-16 inventory found five active sources and no missing paths:

| Source | Input SHA-256 | Derived FLAC SHA-256 |
|---|---|---|
| `run3/sounds/cmib.xm` | `1016bcb301bdcfcaf9d77ffe9645b25d0c869488cd8ecd5759a5678898d2494a` | `f706ae9dd75eadbd1b27145ac8a7940d290dd69dded8a1c69ee50815948b5ad3` |
| `run3/sounds/ELYSIUM.MOD` | `73eaff8065dbc987045499b1bcf6d179a3043419a43306483caa96c93dfd4c37` | `d8c8491c52c623160424c3f2c7b88da02b75a62c77fd3eb914d99bd04f3e08ef` |
| `run3/sounds/mrgspnv2.xm` | `4479be028c71543aafe672ffe6537232306ed5c9c199b46708af529949741f9b` | `165d1f43d87cca3637df8b3978cb5762441a1c9075f0168fca4bbe5a14e281b4` |
| `run3/sounds/mv_pheno.xm` | `e531652e088edd1583d60fe300ed7070759e24036900e39aaf9f885149255a0f` | `00d026c197fdeb659551f382728d123af71f38d9d4cd6d8bed1f20ac3d6c8fd8` |
| `run3/sounds/near_lab.ogg` | `5b37ff3d4d9b2351395fefb85702ec0b5de4a49ce4e59284f9e271d57050d60c` | `95118f503de8adb274f0937a976032b3b6656773f47e32c19d612cacb5c2c6d4` |

These ignored outputs were generated with FFmpeg 6.1.1 and successfully
decoded through the miniaudio backend. No content reference was changed.

Before changing references, the author must compare each original/FLAC pair
for the intended start, end, loop point, duration, channel balance, pitch,
tempo, and audible defects, and record acceptance in the generated manifest or
an adjacent review log. Rights/attribution review is a separate publication
gate; derived audio must not be committed or redistributed merely because the
technical check passes.
