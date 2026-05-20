#version 450

layout(location = 0) out vec3 frag_color;

// Hardcoded triangle in NDC. Vertex buffer comes Day 4.
vec2 positions[3] = vec2[](
    vec2( 0.0, -0.6),
    vec2( 0.6,  0.5),
    vec2(-0.6,  0.5)
);

// Vigil palette: GOLD_BRIGHT, BLOOD, VOID_BG
vec3 colors[3] = vec3[](
    vec3(0.941, 0.816, 0.565),
    vec3(0.545, 0.102, 0.102),
    vec3(0.031, 0.027, 0.039)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    frag_color  = colors[gl_VertexIndex];
}
