#version 460
layout(set=1,binding=0) uniform sampler2D tex;
layout(location=0) out vec4 color;
void main() {
    ivec2 at = ivec2(gl_FragCoord.xy) * ivec2(513,509) / ivec2(1280,720);
    color = all(equal(textureSize(tex,0),ivec2(513,509))) ? vec4(texelFetch(tex,at,0).rgb,1) : vec4(0,0,1,1);
}
