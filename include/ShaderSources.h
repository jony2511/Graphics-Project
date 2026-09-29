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
uniform vec3 uDirLightDir;
uniform vec3 uDirLightColor;
uniform vec3 uAmbientColor;

// Point Light (Burner Flame)
uniform vec3 uPointLightPos;
uniform vec3 uPointLightColor;
uniform float uPointLightIntensity;

// Material
uniform float uSpecularStrength;
uniform float uShininess;

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(uViewPos - FragPos);

    // 1. Ambient (with Sky/Ground hemisphere approximation)
    float hemi = clamp(norm.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 skyColor = uAmbientColor * 1.15;
    vec3 groundBounce = vec3(0.25, 0.38, 0.18) * 0.5;
    vec3 ambient = mix(groundBounce, skyColor, hemi) * VertexColor;

    // 2. Directional Light (Sun / Moon)
    vec3 lightDir = normalize(-uDirLightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uDirLightColor * VertexColor;

    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uShininess);
    vec3 specular = uSpecularStrength * spec * uDirLightColor;

    // 3. Point Light (Burner Flame)
    vec3 pointLightDir = normalize(uPointLightPos - FragPos);
    float pointDist = length(uPointLightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.18 * pointDist + 0.04 * (pointDist * pointDist));
    float pointDiff = max(dot(norm, pointLightDir), 0.0);
    vec3 pointDiffuse = pointDiff * uPointLightColor * VertexColor * attenuation * uPointLightIntensity;

    vec3 result = ambient + diffuse + specular + pointDiffuse;
    FragColor = vec4(result, 1.0);
}
)";

#endif // SHADER_SOURCES_H
