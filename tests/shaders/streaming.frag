#version 460
layout(set=1,binding=0) uniform sampler2D tex;
layout(location=0) out vec4 color;
void main() {
    ivec2 cell = ivec2(gl_FragCoord.xy) / 16;
    // Odd width and multiple rows exercise padding and repeated uploads.
    ivec2 at = ivec2(cell.x % 3, cell.y % 3);
    vec4 fetched = texelFetch(tex, at, 0);
    bool valid = all(equal(textureSize(tex,0),ivec2(3,3)));
    // Make a 2-colour oracle with a 3x3 repeating texture.
    color = valid ? vec4(fetched.rgb,1) : vec4(0,0,1,1);
}
