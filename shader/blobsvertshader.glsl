attribute vec2 vertex;
attribute vec4 vertexColor;
varying vec4 color;
void main(void) 
{
	color = vertexColor;
	gl_Position = vec4(vertex,1.0,1.0);
}
