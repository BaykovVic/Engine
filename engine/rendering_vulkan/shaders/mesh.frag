#version 450
// Blinn-Phong with material parameters, up to 4 scene lights and the sky
// backdrop mode — the Vulkan port of the OpenGL backend's shader.
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

layout(push_constant) uniform Push {
    mat4 model;
    vec4 baseColor; // w = skyMode
    vec4 emissive;  // w = roughness
    vec4 params;    // x = metallic, y = hasTexture
} pc;

layout(location = 0) out vec4 fragColor;

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

    float roughness = pc.emissive.w;
    float metallic = pc.params.x;
    vec3 n = normalize(vNormal);
    vec3 v = normalize(frame.cameraPos.xyz - vWorldPos);
    vec3 albedo = pc.baseColor.rgb;
    if (pc.params.y > 0.5) {
        albedo *= texture(uAlbedo, vUv).rgb;
    }
    vec3 result = albedo * 0.22;
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
        float ndl = max(dot(n, l), 0.0);
        vec3 h = normalize(l + v);
        float shininess = mix(96.0, 4.0, roughness);
        float spec = pow(max(dot(n, h), 0.0), shininess) * (1.0 - roughness * 0.7);
        vec3 diffuse = albedo * (1.0 - metallic);
        vec3 specColor = mix(vec3(0.04), albedo, metallic);
        result += (diffuse * ndl + specColor * spec * ndl) *
                  frame.lightColor[i].rgb * attenuation;
    }
    result += pc.emissive.rgb;
    fragColor = vec4(result, 1.0);
}
