varying vec2 tcoord;
uniform sampler2D tex;
uniform vec3 colorMod;
void main(void) 
{
		const float s = 1.0/3.0;
    gl_FragColor = texture2D(tex,tcoord);
		//gl_FragColor.xyz = cross(gl_FragColor.xyz, colorMod.xyz);
		gl_FragColor.xyz = mix(colorMod, gl_FragColor.xyz, s*dot(colorMod,vec3(1.0)));
}
