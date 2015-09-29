uniform mat4 u_mvp;
attribute vec3 a_position;
attribute vec4 a_color;
attribute vec2 a_st;
varying vec2 tcoord;
varying vec4 v_color;
void main(void) {
	tcoord = a_st;
	gl_Position = u_mvp * vec4(a_position,1);
	v_color = a_color;
}
