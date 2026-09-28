#version 460
layout(set=0,binding=0,std140) uniform Transform { vec4 transform; };
layout(set=1,binding=0,std140) uniform Color { vec4 color; };
layout(location=0) in float pixelX;
layout(location=0) out vec4 result;
void main() {
    if ((int(gl_FragCoord.x) & 1) != 0) discard;
    float dx = dFdx(pixelX);
    result = abs(dx - 1.0) < 0.01 ? color * transform.x : vec4(1,0,0,1);
}
