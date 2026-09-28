OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
#include "OgreUnifiedShader.h"
#ifdef OGRE_HLSL
SAMPLER2D(diffuseBuffer, 0);
SAMPLER2D(normalDepthBuffer, 1);
SAMPLER2D(surfaceBuffer, 2);
SAMPLER2D(shadowBuffer, 3);
#else
// Explicit uniforms avoid Ogre 14.5's layout-binding reflection gap for MRT
// compositor samplers on GL3+. Unit bindings are supplied by the adapter.
uniform sampler2D diffuseBuffer;
uniform sampler2D normalDepthBuffer;
uniform sampler2D surfaceBuffer;
uniform sampler2D shadowBuffer;
#endif
uniform mat4 inverseProjection;
uniform mat4 projection;
uniform float farClip;
uniform vec3 ambient;
uniform int lightCount;
// w: 0 directional, 1 point, 2 spot. All coordinates are view-space.
uniform vec4 lightPosition[64];
uniform vec4 lightDirection[64];
uniform vec4 lightColour[64];
uniform vec4 attenuation[64];
uniform vec4 cone[64];
#ifdef OGRE_HLSL
void main(in vec2 uv : TEXCOORD0, out float4 result : SV_Target, out float depth : SV_Depth)
#else
IN(vec2 uv, TEXCOORD0)
// OgreUnifiedShader already declares the location-zero colour output. A
// second unlocated output is assigned location one on separable GL programs.
#define result gl_FragColor
void main()
#endif
{
  vec4 nd = texture2D(normalDepthBuffer, uv);
  vec4 ds = texture2D(diffuseBuffer, uv);
  vec4 surface = texture2D(surfaceBuffer, uv);
  float directionalShadow = texture2D(shadowBuffer,uv).r;
  vec4 ray = mul(inverseProjection, vec4(uv.x*2.0-1.0, 1.0-uv.y*2.0, 1.0, 1.0));
  vec3 pos = normalize(ray.xyz/ray.w) * nd.w * farClip;
  vec3 n = normalize(nd.xyz);
  vec3 v = normalize(-pos);
  vec3 lit = ds.rgb * ambient * surface.a;
  for (int i=0; i<lightCount; ++i) {
    vec3 offset = lightPosition[i].xyz-pos;
    float distanceToLight = length(offset);
    vec3 l = lightPosition[i].w == 0.0 ? -lightDirection[i].xyz : offset/max(distanceToLight,0.001);
    float weight = 1.0;
    if (lightPosition[i].w != 0.0) {
      vec4 a = attenuation[i];
      weight = distanceToLight < a.x ? 1.0/max(a.y+a.z*distanceToLight+a.w*distanceToLight*distanceToLight,0.001) : 0.0;
      if (lightPosition[i].w == 2.0) {
        float cosine = dot(-l,lightDirection[i].xyz);
        weight *= pow(clamp((cosine-cone[i].y)/max(cone[i].x-cone[i].y,0.0001),0.0,1.0),cone[i].z);
      }
    }
    float lambert = max(dot(n,l),0.0);
    float spec = lambert>0.0 ? pow(max(dot(n,normalize(l+v)),0.0),max(ds.w,1.0)) : 0.0;
    float visibility = lightColour[i].w > 0.0 ? directionalShadow : 1.0;
    lit += (ds.rgb*lambert + surface.rgb*spec) * lightColour[i].rgb * weight * visibility;
  }
  // Restore hardware depth for the subsequent forward transparent pass.
  vec4 clip = mul(projection,vec4(pos,1.0));
#ifdef OGRE_HLSL
  depth = nd.w>0.0 ? clip.z/clip.w : 1.0;
#else
  gl_FragDepth = nd.w>0.0 ? (clip.z/clip.w)*0.5+0.5 : 1.0;
#endif
  result = vec4(nd.w>0.0 ? lit : ambient,1.0);
}
