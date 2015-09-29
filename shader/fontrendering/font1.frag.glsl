precision mediump float;
uniform sampler2D tex;
varying vec2 tcoord;
varying vec4 v_color;
void main() {
    gl_FragColor = vec4(v_color.xyz, v_color.a * texture2D(tex, tcoord).a);
}
