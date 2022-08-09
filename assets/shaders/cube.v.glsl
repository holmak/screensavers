#version 330

uniform mat4 uniProjection;
uniform mat4 uniModelTransform;
uniform vec4 uniModelColor;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;

out vec3 vertNormal;
out vec4 vertColor;

void main() {
    gl_Position = uniProjection * uniModelTransform * vec4(inPosition.xyz, 1.0);
    vertNormal = (uniModelTransform * vec4(inNormal, 0.0)).xyz;
    vertColor = uniModelColor * inColor;
}
