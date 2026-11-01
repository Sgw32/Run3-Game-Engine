/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2014 Torus Knot Software Ltd
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
//-----------------------------------------------------------------------------
// Program Name: SGXLib_IntegratedPSSM
// Program Desc: Run3 stable shadow tiles, receiver-plane PCF and tile blending.
// Program Type: Vertex/Pixel shader
// Language: GLSL
//-----------------------------------------------------------------------------

#ifdef PSSM_ARRAY_TEXTURE
#   ifdef PSSM_SAMPLE_CMP
    #define SAMPLER_TYPE sampler2DArrayShadow
#   else
    #define SAMPLER_TYPE sampler2DArray
#   endif
#define PSSM_LAYER_ARG(x) x,
#else
#   ifdef PSSM_SAMPLE_CMP
    #define SAMPLER_TYPE sampler2DShadow
#   else
    #define SAMPLER_TYPE sampler2D
#   endif
#define PSSM_LAYER_ARG(x)
#endif

// default to 2x2 PCF
#ifndef PCF_XSAMPLES
#define PCF_XSAMPLES 2.0
#endif

//-----------------------------------------------------------------------------
#ifdef SHADOWLIGHT_COUNT
void SGX_ApplyShadowFactor_Modulative(in vec4 ambient,
					  in float fShadowFactor[SHADOWLIGHT_COUNT],
					  inout vec4 diffCol
#ifdef USE_SPECULAR
					  , inout vec4 specCol
#endif
					  )
{
	float shadowFactor = fShadowFactor[0];
	for(int i = 1; i < SHADOWLIGHT_COUNT; ++i)
		shadowFactor *= fShadowFactor[i];

	diffCol.rgb = mix(ambient.rgb, diffCol.rgb, shadowFactor);

#ifdef USE_SPECULAR
	specCol.rgb *= shadowFactor;
#endif
}
#endif
//-----------------------------------------------------------------------------
#ifdef PSSM_SAMPLE_COLOUR
void SGX_ShadowPCF4(in SAMPLER_TYPE shadowMap, in vec4 shadowMapPos, in vec2 invTexSize, PSSM_LAYER_ARG(in float layer) out float c)
{
#	ifdef PSSM_ARRAY_TEXTURE
	c = texture2DArray(shadowMap, vec3(shadowMapPos.xy/shadowMapPos.w, layer)).x;
#	else
	c = texture2DProj(shadowMap, shadowMapPos).x;
#	endif
}
#else
float sampleDepth(in SAMPLER_TYPE shadowMap, vec2 uv, PSSM_LAYER_ARG(in float layer) float depth)
{
#ifdef PSSM_ARRAY_TEXTURE
	// we can assume PSSM_SAMPLE_CMP is always defined when using array textures
	return shadow2D(shadowMap, vec4(uv, layer, depth));
#elif defined(PSSM_SAMPLE_CMP)
#	if defined(OGRE_GLSL) && OGRE_GLSL < 130
	return shadow2D(shadowMap, vec3(uv, depth)).r;
#	else
	return shadow2D(shadowMap, vec3(uv, depth));
#	endif
#else
	return (depth <= texture2D(shadowMap, uv).r) ? 1.0 : 0.0;
#endif
}
void SGX_ShadowPCF4(in SAMPLER_TYPE shadowMap, in f32vec4 shadowMapPos, in vec2 invTexSize, PSSM_LAYER_ARG(in float layer) out float c)
{
	if (shadowMapPos.w <= 0.0) { c = 1.0; return; }
	shadowMapPos = shadowMapPos / shadowMapPos.w;
#if !defined(OGRE_REVERSED_Z) && !defined(OGRE_HLSL) && !defined(VULKAN)
	shadowMapPos.z = shadowMapPos.z * 0.5 + 0.5; // convert -1..1 to 0..1
#endif
	vec2 uv = shadowMapPos.xy;
	// Clamping out-of-frustum depth projects a false silhouette onto a plane
	// beyond the light. Missing coverage must be unshadowed instead.
	if (min(uv.x, uv.y) < 0.0 || max(uv.x, uv.y) > 1.0 ||
	    shadowMapPos.z < 0.0 || shadowMapPos.z > 1.0) { c = 1.0; return; }
	float depth = shadowMapPos.z;
	// Compare each tap against the receiver plane at that tap. This avoids
	// grazing-angle acne without a large constant bias that detaches shadows.
	vec3 dx = dFdx(shadowMapPos.xyz);
	vec3 dy = dFdy(shadowMapPos.xyz);
	float determinant = dx.x * dy.y - dx.y * dy.x;
	vec2 gradient = vec2_splat(0.0);
	if (abs(determinant) > 0.00000001)
		gradient = vec2(dy.y * dx.z - dx.y * dy.z,
		                dx.x * dy.z - dy.x * dx.z) / determinant;
	gradient = clamp(gradient, vec2_splat(-0.02), vec2_splat(0.02));
	c = 0.0;
	float totalWeight = 0.0;
	float offset = PCF_XSAMPLES / 2.0 - 0.5;
	for (float y = -offset; y <= offset; y += 1.0)
		for (float x = -offset; x <= offset; x += 1.0) {
			vec2 delta = invTexSize * vec2(x, y) * 1.5;
			float weight = (offset + 1.0 - abs(x)) * (offset + 1.0 - abs(y));
			float comparison = clamp(depth + dot(gradient, delta), 0.0, 1.0);
			c += weight * sampleDepth(shadowMap, uv + delta, PSSM_LAYER_ARG(layer) comparison);
			totalWeight += weight;
		}
	c /= totalWeight;
	float border = min(min(uv.x, uv.y), min(1.0 - uv.x, 1.0 - uv.y));
	c = mix(1.0, c, smoothstep(0.0, 8.0 * max(invTexSize.x, invTexSize.y), border));
}
#endif

// Cascades are nested light-space tiles, not camera-depth slices. Select and
// blend by actual coverage so turning the viewer cannot sweep split planes
// across the world. The outer 20 percent overlaps the next tile.
float run3TileBlend(vec4 position)
{
	if (position.w <= 0.0) return 1.0;
	vec3 p = position.xyz / position.w;
#if !defined(OGRE_REVERSED_Z) && !defined(OGRE_HLSL) && !defined(VULKAN)
	p.z = p.z * 0.5 + 0.5;
#endif
	if (p.z < 0.0 || p.z > 1.0) return 1.0;
	vec2 edge = abs(p.xy * 2.0 - 1.0);
	return smoothstep(0.8, 0.98, max(edge.x, edge.y));
}

//-----------------------------------------------------------------------------
void SGX_ComputeShadowFactor_PSSM3(in float32_t fDepth,
							in vec4 vSplitPoints,	
							in vec4 lightPosition0,
							in SAMPLER_TYPE shadowMap0,
							in vec2 invShadowMapSize0,
							#if PSSM_NUM_SPLITS > 2
							in vec4 lightPosition1,
							in SAMPLER_TYPE shadowMap1,
							in vec2 invShadowMapSize1,
							#endif
							#if PSSM_NUM_SPLITS > 3
							in vec4 lightPosition2,
							in SAMPLER_TYPE shadowMap2,
							in vec2 invShadowMapSize2,
							#endif
							in vec4 lightPosition3,
							in SAMPLER_TYPE shadowMap3,
							in vec2 invShadowMapSize3,
							out float oShadowFactor
							#ifdef DEBUG_PSSM
							, out vec4 oDiffuse
							#endif
							)
{
	float nextFactor;
	SGX_ShadowPCF4(shadowMap0, lightPosition0, invShadowMapSize0, PSSM_LAYER_ARG(0.0) oShadowFactor);
	float blend = run3TileBlend(lightPosition0);
#if PSSM_NUM_SPLITS > 2
	if (blend > 0.0) {
		SGX_ShadowPCF4(shadowMap1, lightPosition1, invShadowMapSize1, PSSM_LAYER_ARG(1.0) nextFactor);
		oShadowFactor = mix(oShadowFactor, nextFactor, blend);
	}
	blend *= run3TileBlend(lightPosition1);
#endif
#if PSSM_NUM_SPLITS > 3
	if (blend > 0.0) {
		SGX_ShadowPCF4(shadowMap2, lightPosition2, invShadowMapSize2, PSSM_LAYER_ARG(2.0) nextFactor);
		oShadowFactor = mix(oShadowFactor, nextFactor, blend);
	}
	blend *= run3TileBlend(lightPosition2);
#endif
	if (blend > 0.0) {
		SGX_ShadowPCF4(shadowMap3, lightPosition3, invShadowMapSize3, PSSM_LAYER_ARG(float(PSSM_NUM_SPLITS - 1)) nextFactor);
		oShadowFactor = mix(oShadowFactor, nextFactor, blend);
	}
}
