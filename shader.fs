#version 330 core
in float vSpeed;
in vec2 vUV;
out vec4 FragColor;

vec3 getHeatMapColor(float t) {
    vec3 c1 = vec3(0.0, 0.0, 1.0);
    vec3 c2 = vec3(0.0, 1.0, 1.0);
    vec3 c3 = vec3(0.0, 1.0, 0.0);
    vec3 c4 = vec3(1.0, 1.0, 0.0);
    vec3 c5 = vec3(1.0, 0.0, 0.0);

    if (t < 0.25) return mix(c1, c2, t * 4.0);
    if (t < 0.5)  return mix(c2, c3, (t - 0.25) * 4.0);
    if (t < 0.75) return mix(c3, c4, (t - 0.5) * 4.0);
    return mix(c4, c5, (t - 0.75) * 4.0);
}

void main() {
    float dist = length(vUV - vec2(0.5)) * 2.0;

    // Anti-aliasing
    float alpha = 1.0 - smoothstep(0.85, 1.0, dist);
    if (dist > 1.0) discard;

   float velocityMax = 1.5;
    float t = clamp(vSpeed / velocityMax, 0.0, 1.0);

    FragColor = vec4(getHeatMapColor(t), alpha);
}
