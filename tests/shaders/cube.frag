#version 460
layout(set=1,binding=0) uniform samplerCube cube;
layout(location=0) out vec4 color;
void main() {
    vec3 directions[6]=vec3[](vec3(1,0,0),vec3(-1,0,0),vec3(0,1,0),vec3(0,-1,0),vec3(0,0,1),vec3(0,0,-1));
    int face=min(int(gl_FragCoord.x*6.0/1280.0),5);
    color=texture(cube,directions[face]);
}
