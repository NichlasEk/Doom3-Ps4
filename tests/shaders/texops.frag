#version 460
layout(set=1,binding=0) uniform sampler2D tex;
layout(location=0) out vec4 color;
void main() {
    int cell = (int(gl_FragCoord.x) / 16) % 2;
    vec2 uv = vec2((float(cell)+0.5)/2.0, 0.5);
    vec4 fetched = texelFetch(tex, ivec2(cell,0), 0);
    vec4 grad = textureGrad(tex, uv, vec2(0.01,0), vec2(0,0.01));
    bool valid = all(equal(textureSize(tex,0),ivec2(2,1))) && all(equal(fetched,grad));
    color = valid ? vec4(fetched.rgb,1) : vec4(0,0,1,1);
}
