#extension GL_OES_EGL_image_external : require
uniform samplerExternalOES tex;
uniform sampler2D guitex;
uniform sampler2D blobstex;
varying vec2 texcoord;
void main(void) {
	vec4 gui = texture2D(guitex, texcoord);
		vec4 blob = texture2D(blobstex, -texcoord);
		gl_FragColor.rgb = mix(texture2D(tex, texcoord).rgb, gui.rgb, gui.a );
		gl_FragColor.rgb = mix(gl_FragColor.rgb, blob.rgb, blob.a );
		gl_FragColor.a = 1.0;
}
