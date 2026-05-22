#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pc;

layout(set = 0, binding = 1) uniform BonePalette {
    mat4 bones[128];
} u_bones;

layout(location = 0) in vec3  inPos;
layout(location = 1) in vec2  inUV;
layout(location = 2) in uvec4 inJoints;
layout(location = 3) in vec4  inWeights;

layout(location = 0) out vec2 vUV;

void main() {
    // Linear-blend skinning: weighted sum of up to four bone matrices.
    // With the Day 7 identity palette and glTF's normalised weights (sum to 1.0),
    // skin reduces to the identity matrix and inPos passes through unchanged.
    mat4 skin = inWeights.x * u_bones.bones[inJoints.x]
              + inWeights.y * u_bones.bones[inJoints.y]
              + inWeights.z * u_bones.bones[inJoints.z]
              + inWeights.w * u_bones.bones[inJoints.w];

    gl_Position = pc.mvp * skin * vec4(inPos, 1.0);
    vUV = inUV;
}
