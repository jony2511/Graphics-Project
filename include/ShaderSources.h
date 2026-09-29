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

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    VertexColor = aColor;
    TexCoords = aTexCoords;
    gl_Position = uProjection * uView * vec4(FragPos, 1.0);
}
)";

inline const char* SCENE_FRAGMENT_SHADER = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec3 VertexColor;
in vec2 TexCoords;

out vec4 FragColor;

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

void main() {
    if (uEmissive > 0.0) {
        FragColor = vec4(VertexColor, uAlpha);
        return;
    }

    vec3 norm = normalize(Normal);
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

    vec3 result = ambient + dirDiffuse + dirSpecular + pointDiffuse + pointSpecular + spotDiffuse + spotSpecular;

    // --- Natural Soft Distance Fog & Horizon Haze ---
    float dist = length(uViewPos - FragPos);
    float fogStart = 70.0;
    float fogEnd = 240.0;
    float fogFactor = clamp((dist - fogStart) / (fogEnd - fogStart), 0.0, 0.75);
    vec3 fogColor = uSkyColor;
    result = mix(result, fogColor, fogFactor * uFogDensity);

    FragColor = vec4(result, uAlpha);
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

#endif // SHADER_SOURCES_H
