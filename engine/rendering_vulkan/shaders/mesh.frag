#version 450
// Cook-Torrance metal-rough PBR — the Vulkan port of the OpenGL backend's
// shader. Tangent-space normal/parallax mapping reconstructs the tangent
// basis from screen-space derivatives, so the engine vertex format stays
// position(3) + normal(3) + uv(2). PBR maps live in their own descriptor
// sets (set 1..6); absent slots bind a neutral default on the host side.
layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUv;

layout(set = 0, binding = 0) uniform Frame {
    mat4 viewProjection;
    vec4 cameraPos;
    vec4 lightVec[4];
    vec4 lightColor[4];
    vec4 lightMeta[4];
    vec4 counts;
} frame;

layout(set = 1, binding = 0) uniform sampler2D uAlbedo;
layout(set = 2, binding = 0) uniform sampler2D uNormalMap;
layout(set = 3, binding = 0) uniform sampler2D uRoughnessMap;
layout(set = 4, binding = 0) uniform sampler2D uMetallicMap;
layout(set = 5, binding = 0) uniform sampler2D uOcclusionMap;
layout(set = 6, binding = 0) uniform sampler2D uHeightMap;

layout(push_constant) uniform Push {
    mat4 model;
    vec4 baseColor; // w = skyMode
    vec4 emissive;  // w = roughness
    vec4 params;    // x = metallic
    vec4 params2;   // xy = uvTiling, z = parallaxDepth
} pc;

layout(location = 0) out vec4 fragColor;

const float PI = 3.14159265359;

float distributionGGX(float ndh, float rough) {
    float a = rough * rough;
    float a2 = a * a;
    float d = ndh * ndh * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 1e-7);
}
float geometrySchlick(float ndv, float rough) {
    float r = rough + 1.0;
    float k = (r * r) / 8.0;
    return ndv / (ndv * (1.0 - k) + k);
}
vec3 fresnelSchlick(float vdh, vec3 f0) {
    return f0 + (1.0 - f0) * pow(clamp(1.0 - vdh, 0.0, 1.0), 5.0);
}

void main() {
    if (pc.baseColor.w > 0.5) {
        // Sky backdrop: baseColor.rgb = horizon, emissive.rgb = zenith.
        vec3 dir = normalize(vWorldPos - frame.cameraPos.xyz);
        float t = clamp(dir.y * 1.6 + 0.18, 0.0, 1.0);
        vec3 skyColor = mix(pc.baseColor.rgb, pc.emissive.rgb, t);
        for (int i = 0; i < int(frame.counts.x); ++i) {
            if (frame.lightMeta[i].x < 0.5) {
                float towardSun =
                    max(dot(dir, -normalize(frame.lightVec[i].xyz)), 0.0);
                skyColor += frame.lightColor[i].rgb *
                            (pow(towardSun, 600.0) * 1.4 +
                             pow(towardSun, 10.0) * 0.10);
                break;
            }
        }
        fragColor = vec4(skyColor, 1.0);
        return;
    }

    vec3 ng = normalize(vNormal);
    vec3 v = normalize(frame.cameraPos.xyz - vWorldPos);
    vec2 uv = vUv * pc.params2.xy;

    // Tangent basis from screen-space derivatives — no per-vertex tangents.
    vec3 dp1 = dFdx(vWorldPos);
    vec3 dp2 = dFdy(vWorldPos);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);
    vec3 dp2perp = cross(dp2, ng);
    vec3 dp1perp = cross(ng, dp1);
    vec3 tang = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 bitan = dp2perp * duv1.y + dp1perp * duv2.y;
    float invmax = inversesqrt(max(dot(tang, tang), dot(bitan, bitan)));
    mat3 tbn = mat3(tang * invmax, bitan * invmax, ng);

    // Parallax offset along the tangent-space view direction.
    float parallaxDepth = pc.params2.z;
    if (parallaxDepth > 0.0) {
        vec3 vTan = normalize(v * tbn); // transpose(tbn) * v
        float height = texture(uHeightMap, uv).r;
        uv += (vTan.xy / max(vTan.z, 0.3)) * (height - 0.5) * parallaxDepth;
    }

    vec3 albedo = pc.baseColor.rgb * texture(uAlbedo, uv).rgb;
    vec3 mapN = texture(uNormalMap, uv).xyz * 2.0 - 1.0;
    vec3 n = normalize(tbn * mapN);
    float rough = clamp(pc.emissive.w * texture(uRoughnessMap, uv).r, 0.045, 1.0);
    float metal = clamp(pc.params.x * texture(uMetallicMap, uv).r, 0.0, 1.0);
    float ao = texture(uOcclusionMap, uv).r;

    vec3 f0 = mix(vec3(0.04), albedo, metal);
    float ndv = max(dot(n, v), 1e-4);
    vec3 lo = vec3(0.0);
    for (int i = 0; i < int(frame.counts.x); ++i) {
        vec3 l;
        float attenuation = 1.0;
        if (frame.lightMeta[i].x < 0.5) {
            l = normalize(-frame.lightVec[i].xyz);
        } else {
            vec3 toLight = frame.lightVec[i].xyz - vWorldPos;
            float dist = length(toLight);
            l = toLight / max(dist, 1e-4);
            attenuation = clamp(1.0 - dist / frame.lightMeta[i].y, 0.0, 1.0);
            attenuation *= attenuation;
        }
        vec3 h = normalize(l + v);
        float ndl = max(dot(n, l), 0.0);
        float ndh = max(dot(n, h), 0.0);
        float vdh = max(dot(v, h), 0.0);
        float d = distributionGGX(ndh, rough);
        float g = geometrySchlick(ndv, rough) * geometrySchlick(ndl, rough);
        vec3 f = fresnelSchlick(vdh, f0);
        vec3 spec = (d * g * f) / max(4.0 * ndv * ndl, 1e-4);
        vec3 kd = (vec3(1.0) - f) * (1.0 - metal);
        vec3 radiance = frame.lightColor[i].rgb * attenuation;
        lo += (kd * albedo + spec) * radiance * ndl;
    }
    vec3 ambient = albedo * ao * 0.22;
    fragColor = vec4(ambient + lo + pc.emissive.rgb, 1.0);
}
