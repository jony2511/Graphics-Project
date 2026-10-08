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

out vec4 FragColor;

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

void main() {
    if (uEmissive > 0.0) {
        FragColor = vec4(VertexColor, uAlpha);
        return;
    }

    vec3 result;

    if (uShadingModel == 1) {
        // Gouraud Shading: Hardware-interpolated per-vertex lighting
        result = GouraudColor;
    } else {
        // Phong Shading: Per-fragment lighting calculation
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
// Real-Time Screen-Space Ray Tracing Shaders
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

// Camera uniforms
uniform vec3 uCamPos;
uniform vec3 uCamFront;
uniform vec3 uCamRight;
uniform vec3 uCamUp;
uniform float uTanHalfFov;
uniform float uAspectRatio;

// Scene uniforms
uniform float uTime;
uniform vec3 uBalloonPos;
uniform int uBurnerActive;
uniform float uFlameFlicker;

uniform vec3 uBg1Pos;
uniform vec3 uBg2Pos;
uniform vec3 uBg3Pos;

// Lighting uniforms
uniform vec3 uDirLightDir;
uniform vec3 uDirLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uGroundBounceColor;
uniform vec3 uSkyColor;
uniform vec3 uSpotLightPos;
uniform vec3 uSpotLightDir;
uniform vec3 uSpotLightColor;
uniform float uSpotLightIntensity;
uniform float uFogDensity;

struct Ray {
    vec3 orig;
    vec3 dir;
};

struct HitInfo {
    float t;
    vec3 pos;
    vec3 normal;
    vec3 albedo;
    float roughness;
    float reflectivity;
    int matType; // 0: terrain/road/platform, 1: pond water, 2: balloon envelope, 3: basket, 4: burner flame, 5: cottage, 6: tree, 7: haystack
};

// 14 Vibrant Rainbow Colors for Main Balloon
vec3 getRainbowColor(int idx) {
    if (idx == 0) return vec3(0.92, 0.12, 0.18);
    if (idx == 1) return vec3(0.98, 0.45, 0.08);
    if (idx == 2) return vec3(0.98, 0.72, 0.10);
    if (idx == 3) return vec3(0.95, 0.90, 0.12);
    if (idx == 4) return vec3(0.55, 0.88, 0.14);
    if (idx == 5) return vec3(0.10, 0.72, 0.32);
    if (idx == 6) return vec3(0.08, 0.80, 0.62);
    if (idx == 7) return vec3(0.06, 0.75, 0.92);
    if (idx == 8) return vec3(0.12, 0.48, 0.90);
    if (idx == 9) return vec3(0.18, 0.22, 0.85);
    if (idx == 10) return vec3(0.48, 0.16, 0.82);
    if (idx == 11) return vec3(0.78, 0.14, 0.74);
    if (idx == 12) return vec3(0.94, 0.20, 0.55);
    return vec3(0.94, 0.32, 0.28);
}

// Background Balloon Color Palettes
vec3 getSunsetColor(int idx) {
    if (idx == 0) return vec3(0.96, 0.38, 0.14);
    if (idx == 1) return vec3(0.98, 0.62, 0.18);
    if (idx == 2) return vec3(0.92, 0.24, 0.22);
    if (idx == 3) return vec3(0.98, 0.78, 0.26);
    if (idx == 4) return vec3(0.85, 0.20, 0.32);
    if (idx == 5) return vec3(0.96, 0.48, 0.16);
    if (idx == 6) return vec3(0.98, 0.86, 0.40);
    return vec3(0.90, 0.18, 0.20);
}

vec3 getOceanColor(int idx) {
    if (idx == 0) return vec3(0.08, 0.52, 0.82);
    if (idx == 1) return vec3(0.12, 0.76, 0.88);
    if (idx == 2) return vec3(0.06, 0.32, 0.68);
    if (idx == 3) return vec3(0.25, 0.85, 0.82);
    if (idx == 4) return vec3(0.15, 0.45, 0.78);
    if (idx == 5) return vec3(0.35, 0.90, 0.95);
    if (idx == 6) return vec3(0.04, 0.25, 0.55);
    return vec3(0.18, 0.68, 0.80);
}

vec3 getEmeraldColor(int idx) {
    if (idx == 0) return vec3(0.12, 0.68, 0.30);
    if (idx == 1) return vec3(0.48, 0.82, 0.20);
    if (idx == 2) return vec3(0.08, 0.48, 0.24);
    if (idx == 3) return vec3(0.68, 0.88, 0.28);
    if (idx == 4) return vec3(0.16, 0.58, 0.36);
    if (idx == 5) return vec3(0.35, 0.75, 0.25);
    if (idx == 6) return vec3(0.06, 0.38, 0.18);
    return vec3(0.55, 0.85, 0.22);
}

// Ray-Sphere Intersection
bool intersectSphere(Ray r, vec3 center, float radius, inout float tNear, out vec3 normal) {
    vec3 oc = r.orig - center;
    float b = dot(oc, r.dir);
    float c = dot(oc, oc) - radius * radius;
    float disc = b * b - c;
    if (disc < 0.0) return false;
    float sqrtDisc = sqrt(disc);
    float t = -b - sqrtDisc;
    if (t < 0.002) t = -b + sqrtDisc;
    if (t > 0.002 && t < tNear) {
        tNear = t;
        normal = normalize((r.orig + t * r.dir) - center);
        return true;
    }
    return false;
}

// Ray-AABB Box Intersection
bool intersectAABB(Ray r, vec3 bMin, vec3 bMax, inout float tNear, out vec3 normal) {
    vec3 invD = 1.0 / (r.dir + vec3(1e-8));
    vec3 t0 = (bMin - r.orig) * invD;
    vec3 t1 = (bMax - r.orig) * invD;
    vec3 tMin = min(t0, t1);
    vec3 tMax = max(t0, t1);
    float enter = max(max(tMin.x, tMin.y), tMin.z);
    float exit  = min(min(tMax.x, tMax.y), tMax.z);
    if (exit < max(enter, 0.002)) return false;
    float t = enter > 0.002 ? enter : exit;
    if (t > 0.002 && t < tNear) {
        tNear = t;
        vec3 hitP = r.orig + t * r.dir;
        vec3 center = (bMin + bMax) * 0.5;
        vec3 halfDim = (bMax - bMin) * 0.5;
        vec3 d = (hitP - center) / halfDim;
        vec3 absD = abs(d);
        if (absD.x > absD.y && absD.x > absD.z)
            normal = vec3(sign(d.x), 0.0, 0.0);
        else if (absD.y > absD.z)
            normal = vec3(0.0, sign(d.y), 0.0);
        else
            normal = vec3(0.0, 0.0, sign(d.z));
        return true;
    }
    return false;
}

// Ray-Plane Intersection (Ground at y = 0)
bool intersectGroundPlane(Ray r, inout float tNear, out vec3 normal) {
    if (abs(r.dir.y) < 1e-6) return false;
    float t = -r.orig.y / r.dir.y;
    if (t > 0.002 && t < tNear) {
        tNear = t;
        normal = vec3(0.0, 1.0, 0.0);
        return true;
    }
    return false;
}

// Scene Intersection Routine
bool traceScene(Ray r, inout HitInfo hit) {
    hit.t = 1e9;
    vec3 tempNorm;

    // 1. Terrain Ground & Village Pond
    float tG = hit.t;
    if (intersectGroundPlane(r, tG, tempNorm)) {
        vec3 p = r.orig + tG * r.dir;
        if (abs(p.x) < 450.0 && abs(p.z) < 450.0) {
            hit.t = tG;
            hit.pos = p;

            // Check Village Pond (Elliptical mirror water reservoir)
            vec2 pd = p.xz - vec2(46.0, 36.0);
            float ellip = (pd.x * pd.x) / (26.0 * 26.0) + (pd.y * pd.y) / (14.0 * 14.0);
            if (ellip <= 1.0) {
                hit.normal = normalize(vec3(0.035 * sin(p.x * 1.5 + uTime * 3.0), 1.0, 0.035 * cos(p.z * 1.8 + uTime * 2.5)));
                hit.albedo = vec3(0.12, 0.32, 0.38);
                hit.roughness = 0.08;
                hit.reflectivity = 0.72; // Pond reflects sky, balloon & village!
                hit.matType = 1;
            } else {
                hit.normal = vec3(0.0, 1.0, 0.0);
                hit.reflectivity = 0.0;
                // Launch pad platform
                if (abs(p.x) < 8.0 && abs(p.z) < 8.0) {
                    hit.albedo = vec3(0.56, 0.38, 0.24);
                    hit.roughness = 0.75;
                } else {
                    // Curved dirt road
                    float roadDist = abs(p.x - (p.z * 0.28 + 6.0 * sin(p.z * 0.04)));
                    if (p.z > -10.0 && roadDist < 3.2) {
                        hit.albedo = vec3(0.58, 0.48, 0.35);
                        hit.roughness = 0.90;
                    } else {
                        // Rolling green meadow grass
                        hit.albedo = mix(vec3(0.24, 0.54, 0.20), vec3(0.32, 0.62, 0.24), 0.5 + 0.5 * sin(p.x * 0.12) * cos(p.z * 0.12));
                        hit.roughness = 0.85;
                    }
                }
                hit.matType = 0;
            }
        }
    }

    // 2. Hero Hot Air Balloon
    // A. Main Envelope Sphere (R = 4.8)
    vec3 bEnvCenter = uBalloonPos + vec3(0.0, 6.8, 0.0);
    float tEnv = hit.t;
    if (intersectSphere(r, bEnvCenter, 4.8, tEnv, tempNorm)) {
        hit.t = tEnv;
        hit.pos = r.orig + tEnv * r.dir;
        hit.normal = tempNorm;
        vec3 lp = hit.pos - bEnvCenter;
        int stripe = int(floor(mod(atan(lp.z, lp.x) / 3.14159265 * 7.0 + 7.0, 14.0)));
        hit.albedo = getRainbowColor(stripe);
        hit.roughness = 0.38;
        hit.reflectivity = 0.06;
        hit.matType = 2;
    }

    // B. Skirt Sphere (R = 1.6)
    vec3 bSkirtCenter = uBalloonPos + vec3(0.0, 2.8, 0.0);
    float tSkirt = hit.t;
    if (intersectSphere(r, bSkirtCenter, 1.6, tSkirt, tempNorm)) {
        hit.t = tSkirt;
        hit.pos = r.orig + tSkirt * r.dir;
        hit.normal = tempNorm;
        vec3 lp = hit.pos - bSkirtCenter;
        int stripe = int(floor(mod(atan(lp.z, lp.x) / 3.14159265 * 7.0 + 7.0, 14.0)));
        hit.albedo = getRainbowColor(stripe) * 0.88;
        hit.roughness = 0.40;
        hit.reflectivity = 0.05;
        hit.matType = 2;
    }

    // C. Wicker Basket AABB
    vec3 bMin = uBalloonPos + vec3(-1.2, -0.6, -1.2);
    vec3 bMax = uBalloonPos + vec3(1.2, 0.35, 1.2);
    float tBox = hit.t;
    if (intersectAABB(r, bMin, bMax, tBox, tempNorm)) {
        hit.t = tBox;
        hit.pos = r.orig + tBox * r.dir;
        hit.normal = tempNorm;
        hit.albedo = vec3(0.48, 0.32, 0.20);
        hit.roughness = 0.85;
        hit.reflectivity = 0.0;
        hit.matType = 3;
    }

    // D. Burner Flame (Emissive sphere)
    if (uBurnerActive == 1) {
        float tFlame = hit.t;
        if (intersectSphere(r, uBalloonPos + vec3(0.0, 1.3, 0.0), 0.45 * uFlameFlicker, tFlame, tempNorm)) {
            hit.t = tFlame;
            hit.pos = r.orig + tFlame * r.dir;
            hit.normal = tempNorm;
            hit.albedo = vec3(1.0, 0.65, 0.15) * 2.8;
            hit.roughness = 0.1;
            hit.reflectivity = 0.0;
            hit.matType = 4;
        }
    }

    // 3. Background Hot Air Balloons
    // BG1: Sunset
    vec3 bg1Center = uBg1Pos + vec3(0.0, 6.8 * 0.68, 0.0);
    float tBg1 = hit.t;
    if (intersectSphere(r, bg1Center, 4.8 * 0.68, tBg1, tempNorm)) {
        hit.t = tBg1;
        hit.pos = r.orig + tBg1 * r.dir;
        hit.normal = tempNorm;
        vec3 lp = hit.pos - bg1Center;
        int s = int(floor(mod(atan(lp.z, lp.x) / 3.14159265 * 4.0 + 4.0, 8.0)));
        hit.albedo = getSunsetColor(s);
        hit.roughness = 0.45;
        hit.reflectivity = 0.04;
        hit.matType = 2;
    }

    // BG2: Ocean
    vec3 bg2Center = uBg2Pos + vec3(0.0, 6.8 * 0.56, 0.0);
    float tBg2 = hit.t;
    if (intersectSphere(r, bg2Center, 4.8 * 0.56, tBg2, tempNorm)) {
        hit.t = tBg2;
        hit.pos = r.orig + tBg2 * r.dir;
        hit.normal = tempNorm;
        vec3 lp = hit.pos - bg2Center;
        int s = int(floor(mod(atan(lp.z, lp.x) / 3.14159265 * 4.0 + 4.0, 8.0)));
        hit.albedo = getOceanColor(s);
        hit.roughness = 0.45;
        hit.reflectivity = 0.04;
        hit.matType = 2;
    }

    // BG3: Emerald
    vec3 bg3Center = uBg3Pos + vec3(0.0, 6.8 * 0.46, 0.0);
    float tBg3 = hit.t;
    if (intersectSphere(r, bg3Center, 4.8 * 0.46, tBg3, tempNorm)) {
        hit.t = tBg3;
        hit.pos = r.orig + tBg3 * r.dir;
        hit.normal = tempNorm;
        vec3 lp = hit.pos - bg3Center;
        int s = int(floor(mod(atan(lp.z, lp.x) / 3.14159265 * 4.0 + 4.0, 8.0)));
        hit.albedo = getEmeraldColor(s);
        hit.roughness = 0.45;
        hit.reflectivity = 0.04;
        hit.matType = 2;
    }

    // 4. Village Homestead Cottages
    // Cottage 1 (Walls)
    float tC1 = hit.t;
    if (intersectAABB(r, vec3(18.5, 0.0, 28.5), vec3(23.5, 3.2, 33.5), tC1, tempNorm)) {
        hit.t = tC1;
        hit.pos = r.orig + tC1 * r.dir;
        hit.normal = tempNorm;
        hit.albedo = vec3(0.68, 0.52, 0.38);
        hit.roughness = 0.85;
        hit.reflectivity = 0.0;
        hit.matType = 5;
    }
    // Cottage 1 (Thatched Roof)
    float tR1 = hit.t;
    if (intersectAABB(r, vec3(18.0, 3.2, 28.0), vec3(24.0, 5.0, 34.0), tR1, tempNorm)) {
        hit.t = tR1;
        hit.pos = r.orig + tR1 * r.dir;
        hit.normal = tempNorm;
        hit.albedo = vec3(0.58, 0.46, 0.26);
        hit.roughness = 0.90;
        hit.reflectivity = 0.0;
        hit.matType = 5;
    }
    // Cottage 2
    float tC2 = hit.t;
    if (intersectAABB(r, vec3(35.5, 0.0, 65.5), vec3(40.5, 3.2, 70.5), tC2, tempNorm)) {
        hit.t = tC2;
        hit.pos = r.orig + tC2 * r.dir;
        hit.normal = tempNorm;
        hit.albedo = vec3(0.64, 0.50, 0.36);
        hit.roughness = 0.85;
        hit.reflectivity = 0.0;
        hit.matType = 5;
    }
    // Cottage 3
    float tC3 = hit.t;
    if (intersectAABB(r, vec3(-20.5, 0.0, 43.5), vec3(-15.5, 3.2, 48.5), tC3, tempNorm)) {
        hit.t = tC3;
        hit.pos = r.orig + tC3 * r.dir;
        hit.normal = tempNorm;
        hit.albedo = vec3(0.66, 0.52, 0.38);
        hit.roughness = 0.85;
        hit.reflectivity = 0.0;
        hit.matType = 5;
    }

    // 5. Village Banyan Trees (Crown Foliage Spheres)
    float tT1 = hit.t;
    if (intersectSphere(r, vec3(13.8, 5.5, 32.5), 3.5, tT1, tempNorm)) {
        hit.t = tT1;
        hit.pos = r.orig + tT1 * r.dir;
        hit.normal = tempNorm;
        hit.albedo = vec3(0.14, 0.48, 0.16);
        hit.roughness = 0.80;
        hit.reflectivity = 0.0;
        hit.matType = 6;
    }
    float tT2 = hit.t;
    if (intersectSphere(r, vec3(-18.0, 5.8, 24.0), 3.8, tT2, tempNorm)) {
        hit.t = tT2;
        hit.pos = r.orig + tT2 * r.dir;
        hit.normal = tempNorm;
        hit.albedo = vec3(0.16, 0.44, 0.14);
        hit.roughness = 0.80;
        hit.reflectivity = 0.0;
        hit.matType = 6;
    }
    float tT3 = hit.t;
    if (intersectSphere(r, vec3(35.0, 5.2, 18.0), 3.4, tT3, tempNorm)) {
        hit.t = tT3;
        hit.pos = r.orig + tT3 * r.dir;
        hit.normal = tempNorm;
        hit.albedo = vec3(0.12, 0.50, 0.18);
        hit.roughness = 0.80;
        hit.reflectivity = 0.0;
        hit.matType = 6;
    }
    float tT4 = hit.t;
    if (intersectSphere(r, vec3(12.0, 5.6, 68.0), 3.6, tT4, tempNorm)) {
        hit.t = tT4;
        hit.pos = r.orig + tT4 * r.dir;
        hit.normal = tempNorm;
        hit.albedo = vec3(0.15, 0.46, 0.15);
        hit.roughness = 0.80;
        hit.reflectivity = 0.0;
        hit.matType = 6;
    }

    // 6. Golden Straw Haystacks
    float tH1 = hit.t;
    if (intersectSphere(r, vec3(27.5, 1.2, 34.0), 1.8, tH1, tempNorm)) {
        hit.t = tH1;
        hit.pos = r.orig + tH1 * r.dir;
        hit.normal = tempNorm;
        hit.albedo = vec3(0.86, 0.74, 0.32);
        hit.roughness = 0.90;
        hit.reflectivity = 0.0;
        hit.matType = 7;
    }
    float tH2 = hit.t;
    if (intersectSphere(r, vec3(44.0, 1.4, 65.0), 2.0, tH2, tempNorm)) {
        hit.t = tH2;
        hit.pos = r.orig + tH2 * r.dir;
        hit.normal = tempNorm;
        hit.albedo = vec3(0.86, 0.74, 0.32);
        hit.roughness = 0.90;
        hit.reflectivity = 0.0;
        hit.matType = 7;
    }

    return (hit.t < 1e8);
}

// Ray-Traced Shadow Ray
bool isInShadow(vec3 hitPt, vec3 lightDir, float maxDist) {
    Ray sRay = Ray(hitPt + lightDir * 0.05, lightDir);
    float tS = maxDist;
    vec3 dummyNorm;

    // Check Hero Balloon
    if (intersectSphere(sRay, uBalloonPos + vec3(0.0, 6.8, 0.0), 4.8, tS, dummyNorm)) return true;
    if (intersectAABB(sRay, uBalloonPos + vec3(-1.2, -0.6, -1.2), uBalloonPos + vec3(1.2, 0.35, 1.2), tS, dummyNorm)) return true;

    // Check Cottages
    if (intersectAABB(sRay, vec3(18.5, 0.0, 28.5), vec3(23.5, 5.0, 33.5), tS, dummyNorm)) return true;
    if (intersectAABB(sRay, vec3(35.5, 0.0, 65.5), vec3(40.5, 5.0, 70.5), tS, dummyNorm)) return true;

    // Check Trees
    if (intersectSphere(sRay, vec3(13.8, 5.5, 32.5), 3.5, tS, dummyNorm)) return true;
    if (intersectSphere(sRay, vec3(-18.0, 5.8, 24.0), 3.8, tS, dummyNorm)) return true;
    if (intersectSphere(sRay, vec3(35.0, 5.2, 18.0), 3.4, tS, dummyNorm)) return true;
    if (intersectSphere(sRay, vec3(12.0, 5.6, 68.0), 3.6, tS, dummyNorm)) return true;

    return false;
}

// Procedural Atmospheric Sky & Clouds
vec3 sampleSky(Ray r) {
    float skyGradient = clamp(r.dir.y * 1.5, 0.0, 1.0);
    vec3 sky = mix(uSkyColor * 1.15, uSkyColor * 0.75, skyGradient);

    // Sun Disc & Warm Solar Glow
    vec3 sunDir = normalize(-uDirLightDir);
    float sunDot = max(dot(r.dir, sunDir), 0.0);
    float sunGlow = pow(sunDot, 16.0) * 0.5 + pow(sunDot, 256.0) * 2.2;
    sky += uDirLightColor * sunGlow;

    // Cloud Layer
    if (r.dir.y > 0.01) {
        float cloudT = (140.0 - r.orig.y) / r.dir.y;
        if (cloudT > 0.0) {
            vec2 cUV = (r.orig.xz + r.dir.xz * cloudT) * 0.005 + vec2(uTime * 0.006, 0.0);
            float cN = sin(cUV.x * 6.28) * cos(cUV.y * 6.28);
            if (cN > 0.18) sky += vec3(0.9, 0.92, 0.95) * clamp((cN - 0.18) * 2.5, 0.0, 0.7);
        }
    }
    return sky;
}

// Full Analytic Ray-Traced Shading
vec3 shadeHit(Ray r, HitInfo hit) {
    if (hit.matType == 4) return hit.albedo; // Emissive burner flame

    // Directional Sun Light & Ray-Traced Shadows
    vec3 dirLightVec = normalize(-uDirLightDir);
    bool inSunShadow = isInShadow(hit.pos, dirLightVec, 280.0);
    float sunShadowFactor = inSunShadow ? 0.0 : 1.0;

    // Hemisphere Ambient Fill
    float hemi = clamp(hit.normal.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 ambient = mix(uGroundBounceColor, uAmbientColor * 1.15, hemi) * hit.albedo;

    // Diffuse Sunlight
    float diff = max(dot(hit.normal, dirLightVec), 0.0);
    vec3 diffuse = diff * uDirLightColor * hit.albedo * sunShadowFactor;

    // Specular Highlight (Blinn-Phong)
    vec3 viewDir = normalize(uCamPos - hit.pos);
    vec3 halfDir = normalize(dirLightVec + viewDir);
    float specPower = mix(16.0, 128.0, 1.0 - hit.roughness);
    float spec = pow(max(dot(hit.normal, halfDir), 0.0), specPower);
    vec3 specular = spec * uDirLightColor * (1.0 - hit.roughness) * 0.45 * sunShadowFactor;

    // Point Light (Burner Flame inside balloon)
    vec3 burnerPos = uBalloonPos + vec3(0.0, 1.3, 0.0);
    vec3 pointLightVec = burnerPos - hit.pos;
    float pointDist = length(pointLightVec);
    pointLightVec = normalize(pointLightVec);
    float pointAtten = 1.0 / (1.0 + 0.12 * pointDist + 0.035 * pointDist * pointDist);
    float pointDiff = max(dot(hit.normal, pointLightVec), 0.0);
    float burnerIntensity = (uBurnerActive == 1) ? (2.4 * uFlameFlicker) : 0.2;
    vec3 pointDiffuse = pointDiff * vec3(1.0, 0.62, 0.12) * hit.albedo * pointAtten * burnerIntensity;

    // Spotlight (Launch-Pad Mast Floodlight)
    vec3 spotPos = vec3(9.8 - 0.35, 8.2, -9.8 + 0.35);
    vec3 spotVec = spotPos - hit.pos;
    float spotDist = length(spotVec);
    spotVec = normalize(spotVec);
    float theta = dot(spotVec, normalize(-uSpotLightDir));
    float spotCutOff = cos(radians(34.0));
    float spotOuterCutOff = cos(radians(48.0));
    float epsilon = spotCutOff - spotOuterCutOff;
    float spotCone = clamp((theta - spotOuterCutOff) / epsilon, 0.0, 1.0);
    float spotAtten = 1.0 / (1.0 + 0.06 * spotDist + 0.018 * spotDist * spotDist);
    float spotDiff = max(dot(hit.normal, spotVec), 0.0);
    vec3 spotDiffuse = spotDiff * uSpotLightColor * hit.albedo * spotAtten * spotCone * uSpotLightIntensity;

    return ambient + diffuse + specular + pointDiffuse + spotDiffuse;
}

void main() {
    // Generate primary camera ray in world coordinates
    vec2 uv = TexCoord * 2.0 - 1.0;
    vec3 rayDir = normalize(uCamFront + (uv.x * uTanHalfFov * uAspectRatio) * uCamRight + (uv.y * uTanHalfFov) * uCamUp);
    Ray primRay = Ray(uCamPos, rayDir);

    HitInfo hit;
    vec3 col = vec3(0.0);

    if (traceScene(primRay, hit)) {
        col = shadeHit(primRay, hit);

        // Secondary Reflection Bounce (Pond Water & Reflective Surfaces)
        if (hit.reflectivity > 0.02) {
            vec3 reflDir = reflect(primRay.dir, hit.normal);
            Ray reflRay = Ray(hit.pos + hit.normal * 0.03, reflDir);
            HitInfo reflHit;
            vec3 reflCol = vec3(0.0);
            if (traceScene(reflRay, reflHit)) {
                reflCol = shadeHit(reflRay, reflHit);
            } else {
                reflCol = sampleSky(reflRay);
            }
            float fresnel = 0.20 + 0.80 * pow(1.0 - max(dot(-primRay.dir, hit.normal), 0.0), 4.0);
            col = mix(col, reflCol, fresnel * hit.reflectivity);
        }

        // Distance Fog
        float fogStart = 200.0;
        float fogEnd = 640.0;
        float fogFactor = clamp((hit.t - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
        col = mix(col, uSkyColor, fogFactor * uFogDensity);
    } else {
        col = sampleSky(primRay);
    }

    FragColor = vec4(col, 1.0);
}
)";

#endif // SHADER_SOURCES_H
