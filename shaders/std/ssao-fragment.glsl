uniform mat4 projection;
uniform mat4 camera_view;

uniform sampler2D depth_sampler;
uniform sampler2D normal_sampler;
uniform sampler2D noise_sampler;

uniform int NUM_SSAO_SAMPLES;
uniform vec3 ssao_samples[64];
uniform float ssao_kernel_radius;
uniform float ssao_power;

const vec2 noise_scale = vec2(1280.0 / 8.0, 720.0 / 8.0);

in vec4 fPosition; // Position in world space

out float ambient_occlusion;

float linearize_depth(vec2 uv) {
  float n = 1.0;  // camera z near
  float f = 10.0; // camera z far
  float z = texture(depth_sampler, uv).r;
  return (2.0 * n) / (f + n - z * (f - n));
}

void main() {
    vec3 normal = texture(normal_sampler, gl_FragCoord.xy).xyz;

    // Orientate kernel sample hemisphere
    vec3 rvec = texture(noise_sampler, gl_FragCoord.xy * noise_scale).xyz;
    vec3 tangent = normalize(rvec - normal * dot(rvec, fNormal));
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal); // World space to tangent space (tilted world space ... )

    for (int i = 0; i < NUM_SSAO_SAMPLES; i++) {
        // 1. Get sample point
        vec4 point = vec4(vec3(fPosition.xyz + tbn * ssao_samples[i] * ssao_kernel_radius), 1.0);
        // 2. Project the sample
        point = projection * camera_view * point;
        point.xy /= point.w;
        point.xy = point.xy * 0.5 + 0.5;
        // 3. Lookup the sample's real depth
        float point_depth = linearize_depth(point.xy);
        // 4. Range check cuts samples outside the kernel hemisphere
        if (abs(point_depth - point.z) > ssao_kernel_radius) { continue; }
        // 5. Compare depths
        if (point_depth < point.z) { ambient_occlusion += 1.0; }
    }
    ambient_occlusion = 1.0 - (ambient_occlusion / float(NUM_SSAO_SAMPLES));
    ambient_occlusion = pow(ambient_occlusion, ssao_power);
}