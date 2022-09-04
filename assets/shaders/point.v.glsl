#version 330

uniform mat4 uProjection;
uniform mat4 uModelTransform;

layout(location = 0) in vec3 inPosition;

void main() {
    gl_Position = uProjection * uModelTransform * vec4(inPosition, 1.0);
}
