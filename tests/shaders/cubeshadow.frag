#version 460
layout(set=1,binding=8) uniform samplerCubeShadow tex;
layout(location=0) out vec4 color;
void main() {
    const vec3 dirs[6] = vec3[](vec3(1,0,0),vec3(-1,0,0),vec3(0,1,0),vec3(0,-1,0),vec3(0,0,1),vec3(0,0,-1));
    int face = min(int(gl_FragCoord.x*6.0/1280.0),5);
    float reference = gl_FragCoord.y < 240 ? 0.125 : (gl_FragCoord.y < 480 ? 0.5 : 0.875);
    float visibility = texture(tex, vec4(dirs[face],reference));
    color = vec4(1-visibility,visibility,0,1);
}
