#extension GL_OES_EGL_image_external : require
/* Use filter on texture tex */
uniform samplerExternalOES tex;
uniform sampler2D guitex;
uniform sampler2D blobstex;
varying vec2 texcoord;
uniform vec2 texelsize;
void main(void) {

	vec4 tm1m1 = texture2D(tex,texcoord+vec2(-1,-1)*texelsize);
	vec4 tm10 = texture2D(tex,texcoord+vec2(-1,0)*texelsize);
	vec4 tm1p1 = texture2D(tex,texcoord+vec2(-1,1)*texelsize);
	vec4 tp1m1 = texture2D(tex,texcoord+vec2(1,-1)*texelsize);
	vec4 tp10 = texture2D(tex,texcoord+vec2(1,0)*texelsize);
	vec4 tp1p1 = texture2D(tex,texcoord+vec2(1,1)*texelsize);
	vec4 t0m1 = texture2D(tex,texcoord+vec2(0,-1)*texelsize);
	vec4 t0p1 = texture2D(tex,texcoord+vec2(0,-1)*texelsize);

	vec4 xdiff = -1.0*tm1m1 + -2.0*tm10 + -1.0*tm1p1 + 1.0*tp1m1 + 2.0*tp10 + 1.0*tp1p1;
	vec4 ydiff = -1.0*tm1m1 + -2.0*t0m1 + -1.0*tp1m1 + 1.0*tm1p1 + 2.0*t0p1 + 1.0*tp1p1;
	vec4 tot = sqrt(xdiff*xdiff+ydiff*ydiff);

	vec4 col = tot;
  gl_FragColor = clamp(col,vec4(0),vec4(1));


	vec4 gui = texture2D(guitex, texcoord);
	vec4 blob = texture2D(blobstex, -texcoord);
	gl_FragColor.rgb = mix(gl_FragColor.rgb, gui.rgb, gui.a );
	gl_FragColor.rgb = mix(gl_FragColor.rgb, blob.rgb, blob.a );
	gl_FragColor.a = 1.0;
}
