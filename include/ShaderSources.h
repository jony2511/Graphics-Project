#ifndef SHADER_SOURCES_H
#define SHADER_SOURCES_H

inline const char* SCENE_VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec3 VertexColor;
out vec2 TexCoords;
out vec3 GouraudColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

uniform int uShadingModel; // 0: Phong Shading (per-fragment), 1: Gouraud Shading (per-vertex)
uniform vec3 uViewPos;

// 1. Directional Light (Sun / Moon)
uniform vec3 uDirLightDir;
uniform vec3 uDirLightColor;

// 2. Two Area Lights (Sky Fill + Ground Meadow Bounce)
uniform vec3 uAmbientColor;
uniform vec3 uGroundBounceColor;

// 3. Point Light (Burner Flame inside Balloon)
uniform vec3 uPointLightPos;
uniform vec3 uPointLightColor;
uniform float uPointLightIntensity;

// 4. Spotlight (Launch-Pad Night Light on Mast)
uniform vec3 uSpotLightPos;
uniform vec3 uSpotLightDir;
uniform vec3 uSpotLightColor;
uniform float uSpotLightCutOff;
uniform float uSpotLightOuterCutOff;
uniform float uSpotLightIntensity;

// Material Properties
uniform float uSpecularStrength;
uniform float uShininess;
uniform float uEmissive;

void main() {
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    VertexColor = aColor;
    TexCoords = aTexCoords;
    gl_Position = uProjection * uView * vec4(FragPos, 1.0);

    // Compute Gouraud Shading (Per-Vertex Lighting)
    if (uShadingModel == 1) {
        if (uEmissive > 0.0) {
            GouraudColor = aColor;
        } else {
            vec3 norm = normalize(Normal);
            vec3 viewDir = normalize(uViewPos - FragPos);

            // --- Two Area Lights (Sky Fill & Ground Bounce) ---
            float hemi = clamp(norm.y * 0.5 + 0.5, 0.0, 1.0);
            vec3 skyFill = uAmbientColor * 1.15;
            vec3 groundBounce = uGroundBounceColor;
            vec3 ambient = mix(groundBounce, skyFill, hemi) * aColor;

            // --- Directional Light (Sun / Moon) ---
            vec3 dirLightVec = normalize(-uDirLightDir);
            float dirDiff = max(dot(norm, dirLightVec), 0.0);
            vec3 dirDiffuse = dirDiff * uDirLightColor * aColor;

            vec3 dirReflect = reflect(-dirLightVec, norm);
            float dirSpec = pow(max(dot(viewDir, dirReflect), 0.0), uShininess);
            vec3 dirSpecular = uSpecularStrength * dirSpec * uDirLightColor;

            // --- Point Light (Burner Flame) ---
            vec3 pointLightVec = normalize(uPointLightPos - FragPos);
            float pointDist = length(uPointLightPos - FragPos);
            float pointAtten = 1.0 / (1.0 + 0.12 * pointDist + 0.035 * (pointDist * pointDist));
            float pointDiff = max(dot(norm, pointLightVec), 0.0);
            vec3 pointDiffuse = pointDiff * uPointLightColor * aColor * pointAtten * uPointLightIntensity;

            vec3 pointReflect = reflect(-pointLightVec, norm);
            float pointSpec = pow(max(dot(viewDir, pointReflect), 0.0), uShininess);
            vec3 pointSpecular = uSpecularStrength * 0.5 * pointSpec * uPointLightColor * pointAtten * uPointLightIntensity;

            // --- Spotlight (Launch-Pad Floodlight) ---
            vec3 spotLightVec = normalize(uSpotLightPos - FragPos);
            float spotDist = length(uSpotLightPos - FragPos);
            float theta = dot(spotLightVec, normalize(-uSpotLightDir));
            float epsilon = uSpotLightCutOff - uSpotLightOuterCutOff;
            float spotCone = clamp((theta - uSpotLightOuterCutOff) / epsilon, 0.0, 1.0);
            float spotAtten = 1.0 / (1.0 + 0.06 * spotDist + 0.018 * (spotDist * spotDist));

            float spotDiff = max(dot(norm, spotLightVec), 0.0);
            vec3 spotDiffuse = spotDiff * uSpotLightColor * aColor * spotAtten * spotCone * uSpotLightIntensity;

            vec3 spotReflect = reflect(-spotLightVec, norm);
            float spotSpec = pow(max(dot(viewDir, spotReflect), 0.0), uShininess);
            vec3 spotSpecular = uSpecularStrength * spotSpec * uSpotLightColor * spotAtten * spotCone * uSpotLightIntensity;

            GouraudColor = ambient + dirDiffuse + dirSpecular + pointDiffuse + pointSpecular + spotDiffuse + spotSpecular;
        }
    } else {
        GouraudColor = vec3(0.0);
    }
}
)";

inline const char* SCENE_FRAGMENT_SHADER = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec3 VertexColor;
in vec2 TexCoords;
in vec3 GouraudColor;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gPosition;

uniform int uShadingModel; // 0: Phong Shading (per-fragment), 1: Gouraud Shading (per-vertex)
uniform vec3 uViewPos;

// 1. Directional Light (Sun / Moon)
uniform vec3 uDirLightDir;
uniform vec3 uDirLightColor;

// 2. Two Area Lights (Sky Fill + Ground Meadow Bounce)
uniform vec3 uAmbientColor;
uniform vec3 uGroundBounceColor;
uniform vec3 uSkyColor;
uniform float uFogDensity;

// 3. Point Light (Burner Flame inside Balloon)
uniform vec3 uPointLightPos;
uniform vec3 uPointLightColor;
uniform float uPointLightIntensity;

// 4. Spotlight (Launch-Pad Night Light on Mast)
uniform vec3 uSpotLightPos;
uniform vec3 uSpotLightDir;
uniform vec3 uSpotLightColor;
uniform float uSpotLightCutOff;
uniform float uSpotLightOuterCutOff;
uniform float uSpotLightIntensity;

// Material Properties
uniform float uSpecularStrength;
uniform float uShininess;
uniform float uAlpha;
uniform float uEmissive;
uniform float uReflectivity;

void main() {
    if (uEmissive > 0.0) {
        FragColor = vec4(VertexColor, uAlpha);
        gNormal = vec4(0.0, 1.0, 0.0, 0.0);
        gPosition = vec4(FragPos, 0.0);
        return;
    }

    vec3 result;
    vec3 norm = (length(Normal) > 0.001) ? normalize(Normal) : vec3(0.0, 1.0, 0.0);

    if (uShadingModel == 1) {
        // Gouraud Shading: Hardware-interpolated per-vertex lighting
        result = GouraudColor;
    } else {
        // Phong Shading: Per-fragment lighting calculation
        vec3 viewDir = normalize(uViewPos - FragPos);

        // --- Two Area Lights (Sky Fill & Ground Bounce) ---
        float hemi = clamp(norm.y * 0.5 + 0.5, 0.0, 1.0);
        vec3 skyFill = uAmbientColor * 1.15;
        vec3 groundBounce = uGroundBounceColor;
        vec3 ambient = mix(groundBounce, skyFill, hemi) * VertexColor;

        // --- Directional Light (Sun / Moon) ---
        vec3 dirLightVec = normalize(-uDirLightDir);
        float dirDiff = max(dot(norm, dirLightVec), 0.0);
        vec3 dirDiffuse = dirDiff * uDirLightColor * VertexColor;

        vec3 dirReflect = reflect(-dirLightVec, norm);
        float dirSpec = pow(max(dot(viewDir, dirReflect), 0.0), uShininess);
        vec3 dirSpecular = uSpecularStrength * dirSpec * uDirLightColor;

        // --- Point Light (Burner Flame) ---
        vec3 pointLightVec = normalize(uPointLightPos - FragPos);
        float pointDist = length(uPointLightPos - FragPos);
        float pointAtten = 1.0 / (1.0 + 0.12 * pointDist + 0.035 * (pointDist * pointDist));
        float pointDiff = max(dot(norm, pointLightVec), 0.0);
        vec3 pointDiffuse = pointDiff * uPointLightColor * VertexColor * pointAtten * uPointLightIntensity;

        vec3 pointReflect = reflect(-pointLightVec, norm);
        float pointSpec = pow(max(dot(viewDir, pointReflect), 0.0), uShininess);
        vec3 pointSpecular = uSpecularStrength * 0.5 * pointSpec * uPointLightColor * pointAtten * uPointLightIntensity;

        // --- Spotlight (Launch-Pad Floodlight) ---
        vec3 spotLightVec = normalize(uSpotLightPos - FragPos);
        float spotDist = length(uSpotLightPos - FragPos);
        float theta = dot(spotLightVec, normalize(-uSpotLightDir));
        float epsilon = uSpotLightCutOff - uSpotLightOuterCutOff;
        float spotCone = clamp((theta - uSpotLightOuterCutOff) / epsilon, 0.0, 1.0);
        float spotAtten = 1.0 / (1.0 + 0.06 * spotDist + 0.018 * (spotDist * spotDist));

        float spotDiff = max(dot(norm, spotLightVec), 0.0);
        vec3 spotDiffuse = spotDiff * uSpotLightColor * VertexColor * spotAtten * spotCone * uSpotLightIntensity;

        vec3 spotReflect = reflect(-spotLightVec, norm);
        float spotSpec = pow(max(dot(viewDir, spotReflect), 0.0), uShininess);
        vec3 spotSpecular = uSpecularStrength * spotSpec * uSpotLightColor * spotAtten * spotCone * uSpotLightIntensity;

        result = ambient + dirDiffuse + dirSpecular + pointDiffuse + pointSpecular + spotDiffuse + spotSpecular;
    }

    // --- Natural Soft Distance Fog & Horizon Haze ---
    float dist = length(uViewPos - FragPos);
    float fogStart = 200.0;
    float fogEnd = 640.0;
    float fogFactor = clamp((dist - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    vec3 fogColor = uSkyColor;
    result = mix(result, fogColor, fogFactor * uFogDensity);

    FragColor = vec4(result, uAlpha);
    gNormal = vec4(norm, uReflectivity);
    gPosition = vec4(FragPos, max(dist, 0.001));
}
)";

inline const char* HUD_VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

out vec4 VertexColor;

uniform mat4 uProjection;

void main() {
    VertexColor = aColor;
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
}
)";

inline const char* HUD_FRAGMENT_SHADER = R"(
#version 330 core
in vec4 VertexColor;
out vec4 FragColor;

void main() {
    FragColor = VertexColor;
}
)";

// =========================================================================
// Real-Time Screen-Space Ray Tracing Shaders (SSR + RTAO + Soft Shadows)
// =========================================================================

inline const char* RAYTRACING_VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
out vec2 TexCoord;

void main() {
    TexCoord = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

inline const char* RAYTRACING_FRAGMENT_SHADER = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uGColor;
uniform sampler2D uGNormal;
uniform sampler2D uGPosition;

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uCamPos;
uniform vec3 uDirLightDir;
uniform vec3 uDirLightColor;
uniform vec3 uSkyColor;
uniform float uTime;

// Screen-Space Ray Traced Reflections (SSR)
vec3 traceScreenSpaceReflection(vec3 startPos, vec3 reflDir, float reflectivity) {
    if (reflectivity < 0.20) return vec3(0.0);

    int maxSteps = 42;
    float stepSize = 0.65;
    vec3 currentPos = startPos + reflDir * 0.40;
    vec2 hitUV = vec2(0.0);
    bool hitFound = false;

    for (int i = 0; i < maxSteps; ++i) {
        currentPos += reflDir * stepSize;
        stepSize *= 1.045; // Geometric step expansion for distance reach

        vec4 clipPos = uProjection * uView * vec4(currentPos, 1.0);
        if (clipPos.w <= 0.001) break;
        vec3 ndcPos = clipPos.xyz / clipPos.w;
        vec2 sampleUV = ndcPos.xy * 0.5 + 0.5;

        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
            break;

        vec4 scenePosSample = texture(uGPosition, sampleUV);
        if (scenePosSample.a < 0.001) continue; // sky background

        vec3 scenePos = scenePosSample.xyz;
        float distAlongRay = length(currentPos - startPos);
        float sceneDistAlongRay = length(scenePos - startPos);

        if (distAlongRay >= sceneDistAlongRay && (distAlongRay - sceneDistAlongRay) < (stepSize * 2.2)) {
            // Binary search refinement
            vec3 refineStart = currentPos - reflDir * stepSize;
            vec3 refineEnd = currentPos;
            for (int j = 0; j < 4; ++j) {
                vec3 mid = (refineStart + refineEnd) * 0.5;
                vec4 midClip = uProjection * uView * vec4(mid, 1.0);
                vec2 midUV = (midClip.xy / midClip.w) * 0.5 + 0.5;
                vec3 midScenePos = texture(uGPosition, midUV).xyz;
                if (length(mid - startPos) >= length(midScenePos - startPos)) {
                    refineEnd = mid;
                } else {
                    refineStart = mid;
                }
            }
            vec4 finalClip = uProjection * uView * vec4((refineStart + refineEnd) * 0.5, 1.0);
            hitUV = (finalClip.xy / finalClip.w) * 0.5 + 0.5;
            hitFound = true;
            break;
        }
    }

    if (hitFound) {
        vec2 edgeDist = min(hitUV, 1.0 - hitUV);
        float edgeFade = clamp(min(edgeDist.x, edgeDist.y) * 12.0, 0.0, 1.0);
        return texture(uGColor, hitUV).rgb * edgeFade;
    }
    return vec3(0.0);
}

// Ray-Traced Ambient Occlusion (RTAO)
float computeRTAO(vec3 pos, vec3 norm) {
    float occlusion = 0.0;
    float radius = 1.9;

    vec3 sampleKernel[8] = vec3[8](
        vec3( 0.40,  0.70,  0.30),
        vec3(-0.50,  0.65, -0.25),
        vec3( 0.55,  0.50, -0.45),
        vec3(-0.35,  0.80,  0.35),
        vec3( 0.20,  0.85, -0.20),
        vec3(-0.60,  0.55,  0.20),
        vec3( 0.35,  0.60,  0.55),
        vec3(-0.25,  0.50, -0.70)
    );

    for (int i = 0; i < 8; ++i) {
        vec3 rayDir = normalize(sampleKernel[i]);
        if (dot(rayDir, norm) < 0.0) rayDir = -rayDir;
        vec3 samplePos = pos + (rayDir + norm * 0.4) * (radius * (0.35 + 0.65 * float(i) / 8.0));

        vec4 clipPos = uProjection * uView * vec4(samplePos, 1.0);
        if (clipPos.w <= 0.001) continue;
        vec2 sampleUV = (clipPos.xy / clipPos.w) * 0.5 + 0.5;

        if (sampleUV.x >= 0.0 && sampleUV.x <= 1.0 && sampleUV.y >= 0.0 && sampleUV.y <= 1.0) {
            vec4 scenePosSample = texture(uGPosition, sampleUV);
            if (scenePosSample.a > 0.001) {
                vec3 scenePos = scenePosSample.xyz;
                float distDiff = length(pos - scenePos);
                if (distDiff < radius && scenePos.y >= samplePos.y - 0.20) {
                    float rangeCheck = smoothstep(0.0, 1.0, radius / (distDiff + 0.001));
                    occlusion += rangeCheck;
                }
            }
        }
    }
    return clamp(1.0 - (occlusion / 8.0) * 0.65, 0.35, 1.0);
}

// Ray-Traced Directional Shadow Marching
float computeRayTracedShadow(vec3 pos, vec3 norm, vec3 lightDir) {
    if (dot(norm, lightDir) <= 0.0) return 0.25;

    vec3 rayDir = lightDir;
    int steps = 14;
    float stepSize = 0.90;
    vec3 currentPos = pos + norm * 0.15;
    float shadowFactor = 1.0;

    for (int i = 0; i < steps; ++i) {
        currentPos += rayDir * stepSize;
        stepSize *= 1.05;

        vec4 clipPos = uProjection * uView * vec4(currentPos, 1.0);
        if (clipPos.w <= 0.001) break;
        vec2 sampleUV = (clipPos.xy / clipPos.w) * 0.5 + 0.5;

        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
            break;

        vec4 scenePosSample = texture(uGPosition, sampleUV);
        if (scenePosSample.a > 0.001) {
            vec3 scenePos = scenePosSample.xyz;
            float heightDiff = scenePos.y - currentPos.y;
            if (heightDiff > 0.08 && heightDiff < 4.5) {
                float penumbra = clamp(float(i) / float(steps), 0.2, 1.0);
                shadowFactor = min(shadowFactor, 0.40 + 0.60 * penumbra);
                if (shadowFactor <= 0.41) break;
            }
        }
    }
    return shadowFactor;
}

void main() {
    vec4 gColorSample = texture(uGColor, TexCoord);
    vec4 gNormSample = texture(uGNormal, TexCoord);
    vec4 gPosSample = texture(uGPosition, TexCoord);

    vec3 baseColor = gColorSample.rgb;
    float dist = gPosSample.a;

    // Background sky and sun disc pass through untouched
    if (dist < 0.001) {
        FragColor = vec4(baseColor, 1.0);
        return;
    }

    vec3 worldPos = gPosSample.xyz;
    vec3 worldNorm = normalize(gNormSample.xyz);
    float reflectivity = gNormSample.a;

    // 1. Ray-Traced Ambient Occlusion (RTAO)
    float rtao = computeRTAO(worldPos, worldNorm);

    // 2. Ray-Traced Directional Shadow Rays
    vec3 sunLightDir = normalize(-uDirLightDir);
    float rtShadow = computeRayTracedShadow(worldPos, worldNorm, sunLightDir);

    // Modulate lighting with Ray-Traced Contact Occlusion and Soft Shadows
    vec3 finalColor = baseColor;
    finalColor *= (0.45 + 0.55 * rtShadow);
    finalColor *= rtao;

    // 3. Screen-Space Ray Traced Pond Reflections (SSR)
    if (reflectivity > 0.25) {
        vec3 viewDir = normalize(worldPos - uCamPos);
        vec3 reflDir = reflect(viewDir, worldNorm);
        vec3 ssrColor = traceScreenSpaceReflection(worldPos, reflDir, reflectivity);
        if (length(ssrColor) > 0.01) {
            float fresnel = 0.25 + 0.75 * pow(clamp(1.0 - dot(-viewDir, worldNorm), 0.0, 1.0), 4.0);
            finalColor = mix(finalColor, ssrColor, clamp(fresnel * reflectivity, 0.0, 0.95));
        }
    }

    // 4. Subtle Ray-Traced Ground Bounce Light (Underside of balloon and eaves)
    float groundBounce = clamp(-worldNorm.y * 0.5 + 0.2, 0.0, 0.35);
    finalColor += vec3(0.06, 0.12, 0.05) * groundBounce * rtao;

    FragColor = vec4(finalColor, 1.0);
}
)";

#endif // SHADER_SOURCES_H
