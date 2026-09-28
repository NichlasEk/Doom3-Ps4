#version 460
layout(set=1,binding=0) uniform sampler2DShadow tex;
layout(location=0) out vec4 color;
void main() {
    int cell = (int(gl_FragCoord.x) / 16) % 2;
    float reference = gl_FragCoord.y < 240 ? 0.125 : (gl_FragCoord.y < 480 ? 0.5 : 0.875);
    float visibility = texture(tex, vec3((float(cell)+0.5)/2.0,0.5,reference));
    color = vec4(1-visibility,visibility,0,1);
}
