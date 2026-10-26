#ifndef ALLOW_INCLUDES
#define ALLOW_INCLUDES
#extension GL_ARB_shading_language_include : enable
#endif
#include "cm_helpers.glsl"
#include "gradient.glsl"
#if USE_ROUNDING
#include "rounding.glsl"
#endif

// OpenMotif XmeDrawShadows, raised: top and left are light, including the outer
// top-right and bottom-left corners. The other two corners are 45-degree miters.
// px, py are 0-based pixel indices; w, h are the box size in pixels.
bool motifRaisedLight(float px, float py, float w, float h, float b) {
    return (py < b && px + py < w) || (px < b && px + py < h);
}

// Same 8-bit recipe as the CDE panel CSS: light is halfway to white, dark is half the face.
vec3 motifShade(vec3 face, bool lightSide) {
    vec3 bytes = floor(clamp(face, vec3(0.0), vec3(1.0)) * 255.0 + 0.5);
    vec3 outBytes = lightSide ? bytes + floor((vec3(255.0) - bytes) * 0.5) : floor(bytes * 0.5);
    return outBytes / 255.0;
}

#if USE_MIRROR
vec4[2]
#else
vec4
#endif
    getBorder(vec2 v_texcoord, float alpha, vec2 fullSizeUntransformed, float radiusOuter, float thick, float radius, float roundingPower, vec2 topLeft, vec2 fullSize,
              int gradientLength, vec4 gradient[10], float angle, int gradient2Length, vec4 gradient2[10], float angle2, float gradientLerp, int bevel
#if USE_CM
              ,
              int sourceTF, int targetTF, mat3 convertMatrix, vec2 srcTFRange, vec2 dstTFRange
#if USE_ICC
              ,
              highp sampler3D iccLut3D, float iccLutSize
#else
#if USE_TONEMAP || USE_SDR_MOD
              ,
              mat3 targetPrimariesXYZ
#endif
#if USE_TONEMAP
              ,
              float maxLuminance, float dstMaxLuminance, float dstRefLuminance, float srcRefLuminance, int tonemapMode
#endif
#if USE_SDR_MOD
              ,
              float sdrSaturation, float sdrBrightnessMultiplier
#endif
#endif
#endif
    ) {
    vec2 pixCoord         = vec2(gl_FragCoord);
    vec2 pixCoordOuter    = pixCoord;
    vec2 originalPixCoord = v_texcoord;
    originalPixCoord *= fullSizeUntransformed;
    float additionalAlpha = 1.0;

    vec4  pixColor = vec4(1.0, 1.0, 1.0, 1.0);

    bool  done = false;

    pixCoord -= topLeft + fullSize * 0.5;
    pixCoord *= vec2(lessThan(pixCoord, vec2(0.0))) * -2.0 + 1.0;
    pixCoordOuter = pixCoord;
    pixCoord -= fullSize * 0.5 - radius;
    pixCoordOuter -= fullSize * 0.5 - radiusOuter;

    // center the pixels don't make it top-left
    pixCoord += vec2(1.0, 1.0) / fullSize;
    pixCoordOuter += vec2(1.0, 1.0) / fullSize;

#if USE_ROUNDING
    if (min(pixCoord.x, pixCoord.y) > 0.0 && radius > 0.0) {
        float dist      = pow(pow(pixCoord.x, roundingPower) + pow(pixCoord.y, roundingPower), 1.0 / roundingPower);
        float distOuter = pow(pow(pixCoordOuter.x, roundingPower) + pow(pixCoordOuter.y, roundingPower), 1.0 / roundingPower);
        float h         = (thick / 2.0);

        if (dist < radius - h) {
            // lower
            float normalized = smoothstep(0.0, 1.0, (dist - radius + thick + SMOOTHING_CONSTANT) / (SMOOTHING_CONSTANT * 2.0));
            additionalAlpha *= normalized;
            done = true;
        } else if (min(pixCoordOuter.x, pixCoordOuter.y) > 0.0) {
            // higher
            float normalized = 1.0 - smoothstep(0.0, 1.0, (distOuter - radiusOuter + SMOOTHING_CONSTANT) / (SMOOTHING_CONSTANT * 2.0));
            additionalAlpha *= normalized;
            done = true;
        } else if (distOuter < radiusOuter - h) {
            additionalAlpha = 1.0;
            done            = true;
        }
    }
#endif

    // now check for other shit
    if (!done) {
        // distance to all straight bb borders
        float distanceT = originalPixCoord[1];
        float distanceB = fullSizeUntransformed[1] - originalPixCoord[1];
        float distanceL = originalPixCoord[0];
        float distanceR = fullSizeUntransformed[0] - originalPixCoord[0];

        // get the smallest
        float smallest = min(min(distanceT, distanceB), min(distanceL, distanceR));

        if (smallest > thick)
            discard;
    }

    if (additionalAlpha == 0.0)
        discard;

    pixColor = getColorForCoord(v_texcoord, gradientLength, gradient, angle, gradient2Length, gradient2, angle2, gradientLerp);

    // bevel == 0 leaves the flat gradient, which is the Dracula frame.
    // Rounded-corner arc pixels (done) stay flat: the miter is only defined on the straight edges.
    if (bevel != 0 && !done && thick >= 1.0) {
        float x  = originalPixCoord[0];
        float y  = originalPixCoord[1];
        float w  = fullSizeUntransformed[0];
        float h  = fullSizeUntransformed[1];
        float px = floor(x);
        float py = floor(y);
        float pw = floor(w);
        float ph = floor(h);

        // 5px frame: outer 2px raised, 1px face (the configured border color), inner 2px sunken.
        float b = thick >= 5.0 ? 2.0 : 1.0;
        if (thick < b * 2.0)
            b = 1.0;
        b = min(b, max(floor(min(pw, ph) * 0.5), 1.0));

        float edge = min(min(x, w - x), min(y, h - y));
        if (thick < 2.0 || edge < b)
            pixColor.rgb = motifShade(pixColor.rgb, motifRaisedLight(px, py, pw, ph, b));
        else if (edge >= thick - b) {
            float inset = thick - b;
            float iw    = pw - 2.0 * inset;
            float ih    = ph - 2.0 * inset;
            // OpenMotif drops the shadow when the box is narrower than twice the thickness.
            if (iw >= b * 2.0 && ih >= b * 2.0) {
                bool innerLight = motifRaisedLight(px - inset, py - inset, iw, ih, b);
                pixColor.rgb    = motifShade(pixColor.rgb, !innerLight);
            }
        }
    }

    pixColor.rgb *= pixColor[3];

#if USE_CM
    return doColorManagement(pixColor, alpha * additionalAlpha, sourceTF, targetTF, convertMatrix, srcTFRange, dstTFRange, 0.0
#if USE_ICC
                             ,
                             iccLut3D, iccLutSize
#else
#if USE_TONEMAP || USE_SDR_MOD
                             ,
                             targetPrimariesXYZ
#endif
#if USE_TONEMAP
                             ,
                             maxLuminance, dstMaxLuminance, dstRefLuminance, srcRefLuminance, tonemapMode
#endif
#if USE_SDR_MOD
                             ,
                             sdrSaturation, sdrBrightnessMultiplier
#endif
#endif
    );
#endif

#if USE_MIRROR
    vec4[2] pixColors;
    pixColors[0] = pixColor * alpha * additionalAlpha;
    pixColors[1] = pixColors[0];
    return pixColors;
#else
    pixColor *= alpha * additionalAlpha;
    return pixColor;
#endif
}
