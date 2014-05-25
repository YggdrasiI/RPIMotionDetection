attribute vec2 vertex;
varying vec2 texcoord;
void main(void) {
   texcoord = 0.5 * (1.0 - vertex );
   gl_Position = vec4(vertex, 0.0, 1.0);
}
