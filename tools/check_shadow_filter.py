"""Compile Run3's shadow receiver variants with the Windows SDK FXC (no GPU/window)."""
import argparse
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fxc', required=True, type=Path)
    parser.add_argument('--ogre-media', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    source = args.output / 'shadow_receiver.hlsl'
    source.write_text('''#define OGRE_HLSL 5
#define OGRE_FRAGMENT_SHADER 1
#include "OgreUnifiedShader.h"
#include "SGXLib_IntegratedPSSM.glsl"
SAMPLER2DSHADOW(map0, 0);
SAMPLER2DSHADOW(map1, 1);
SAMPLER2DSHADOW(map3, 2);
float4 main(float4 p0 : TEXCOORD0, float4 p1 : TEXCOORD1,
            float4 p3 : TEXCOORD2) : SV_Target {
    float visibility;
#if PSSM_NUM_SPLITS == 3
    SGX_ComputeShadowFactor_PSSM3(0.5, float4(0.2,0.5,0.8,1),
        p0, map0, float2(0.001,0.001), p1, map1, float2(0.001,0.001),
        p3, map3, float2(0.001,0.001), visibility);
#else
    SGX_ShadowPCF4(map0, p0, float2(0.001,0.001), visibility);
#endif
    return float4(visibility,visibility,visibility,1);
}
''', encoding='utf-8')
    for samples in (2, 4):
        for splits in (1, 3):
            for reversed_z in (False, True):
                name = f'pcf{samples * samples}-tiles{splits}-reversed{int(reversed_z)}'
                command = [str(args.fxc), '/nologo', '/T', 'ps_5_0', '/E', 'main',
                           '/I', str(args.ogre_media / 'Main'),
                           '/I', str(Path(__file__).resolve().parents[1] / 'media/Lighting'),
                           '/D', 'PSSM_SAMPLE_CMP=1', '/D', 'SHADOWLIGHT_COUNT=6',
                           '/D', f'PCF_XSAMPLES={samples}.0',
                           '/D', f'PSSM_NUM_SPLITS={splits}',
                           '/Fo', str(args.output / f'{name}.cso')]
                if reversed_z:
                    command += ['/D', 'OGRE_REVERSED_Z=1']
                command.append(str(source))
                result = subprocess.run(command, capture_output=True, text=True)
                if result.returncode:
                    raise SystemExit(f'{name}:\n{result.stdout}\n{result.stderr}')
                print(f'{name}: PASS')


if __name__ == '__main__':
    main()
