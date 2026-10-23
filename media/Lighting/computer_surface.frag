OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
#include "OgreUnifiedShader.h"

SAMPLER2D(computerSurface, 0);

MAIN_PARAMETERS
IN(vec2 uv, TEXCOORD0)
MAIN_DECLARATION
{
    gl_FragColor = texture2D(computerSurface, uv);
}
