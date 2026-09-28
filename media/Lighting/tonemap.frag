OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
#include "OgreUnifiedShader.h"
SAMPLER2D(sceneColour, 0);
uniform float exposure;
MAIN_PARAMETERS
IN(vec2 uv, TEXCOORD0)
MAIN_DECLARATION
{
  vec3 x = max(texture2D(sceneColour, uv).rgb * exposure, vec3_splat(0.0));
  // Filmic fit, then exact linear-to-sRGB transfer. The non-sRGB window
  // receives encoded output once, after HDR lighting/exposure.
  vec3 mapped = clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14), 0.0, 1.0);
  vec3 low = mapped * 12.92;
  vec3 high = 1.055 * pow(mapped, vec3_splat(1.0/2.4)) - 0.055;
  gl_FragColor = vec4(mix(low, high, step(vec3_splat(0.0031308), mapped)), 1.0);
}
