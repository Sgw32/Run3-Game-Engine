// Included after Ogre's Cook-Torrance library by the RTSS dependency list.
// Legacy RGB specular masks modulate F0; they do not become a metallic map.
void Run3_ApplySpecularMask(in vec3 mask, inout PixelParams pixel)
{
    pixel.f0 *= clamp(mask, vec3_splat(0.0), vec3_splat(1.0));
}
