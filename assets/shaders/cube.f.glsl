#version 330

uniform float uniAmbientLight;

in vec3 vertNormal;
in vec4 vertColor;

out vec4 fragColor;

void main() {
    vec3 normal = normalize(vertNormal);
    vec3 lightDir = normalize(vec3(1.0, 0.0, 0.0));
    float light = clamp(dot(lightDir, normal), 0.0, 1.0);
    light = max(uniAmbientLight, light);
    fragColor = vertColor;
    fragColor.rgb *= light;
}
