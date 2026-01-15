#version 450


layout(location = 0) in vec3 inFragColor;

layout(location = 0) out vec4 outColour;

void main() {
    outColour = vec4(inFragColor, 1.0);
}


