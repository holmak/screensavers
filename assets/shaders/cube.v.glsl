#version 330

uniform mat4 uniProjection;
uniform mat4 uniModelTransform;
uniform vec4 uniModelColor;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

out vec4 vertColor;

void main() {
    gl_Position = uniProjection * uniModelTransform * vec4(inPosition, 1.0f);
    vertColor = uniModelColor * inColor;
}
