varying vec2 tcoord;
uniform sampler2D tex;
uniform float alpha;
void main(void) 
{
    float val = texture2D(tex,tcoord).x;
    //gl_FragColor = vec4(val,val,val, 0.5) ;

		/* Coloring of ids */
		vec3 res = vec3( val*25.0, val*49.0, val*3.0+0.75 );
		res = fract(res);
    gl_FragColor = vec4(res, 0.5) ;
		if( alpha > 0.0)
	    gl_FragColor.a = alpha;
}
