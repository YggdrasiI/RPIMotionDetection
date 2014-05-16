varying vec2 tcoord;
uniform sampler2D tex;
void main(void) 
{
    float res = texture2D(tex,tcoord).x;
		//if( res == 0.0 ) res = 1.0;

    gl_FragColor = vec4(0.5, res, res, 0.5) ;
}
