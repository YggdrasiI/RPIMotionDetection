varying vec2 tcoord;
uniform sampler2D tex;
uniform float alpha;
void main(void) 
{
    gl_FragColor = texture2D(tex,tcoord);
		if( alpha > 0.0)
	    gl_FragColor.a = alpha;
}
