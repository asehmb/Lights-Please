#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(binding = 0, set = 2) uniform sampler2D albedo;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 uvColor = texture(albedo, fragTexCoord);
    outColor = vec4(fragColor, 1.0) * uvColor;

}


