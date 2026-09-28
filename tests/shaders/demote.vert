#version 460
layout(location=0) in vec2 pos;
layout(location=1) in vec3 unusedColor;
layout(set=0,binding=0,std140) uniform Transform { vec4 transform; };
layout(location=0) out float pixelX;
void main() {
    gl_Position = vec4(pos * transform.xy + transform.zw, 0, 1);
    pixelX = (pos.x + 1.0) * 640.0;
}
