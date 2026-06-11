#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aInstPos;
layout (location = 2) in float aSpeed;

uniform float aspect;
uniform float radius;

out float vSpeed;
out vec2 vUV;

void main() {
    vSpeed = aSpeed;
    vUV = aPos * 0.5 + 0.5;

    vec2 finalPos = aInstPos + aPos * radius;
    gl_Position = vec4(finalPos.x / aspect, finalPos.y, 0.0, 1.0);
}