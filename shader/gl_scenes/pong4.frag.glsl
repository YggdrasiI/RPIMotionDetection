#extension GL_OES_EGL_image_external : require
uniform samplerExternalOES tex;
uniform sampler2D guitex;
uniform sampler2D blobstex;
uniform float offset;
const float waves = 2.0;
varying vec2 texcoord;
void main(void) {
	float x = texcoord.x + 0.05 * sin(offset + (texcoord.y * waves * 2.0 * 3.141592));
	float y = texcoord.y + 0.05 * sin(offset + (texcoord.x * waves * 2.0 * 3.141592));
	x = clamp(0.0,x,1.0);
	y = clamp(0.0,y,1.0);
	vec2 pos = vec2(x, y);

	vec4 gui = texture2D(guitex, texcoord);
	vec4 blob = texture2D(blobstex, -pos);

	gl_FragColor.rgb = mix(texture2D(tex, pos).rgb, gui.rgb, gui.a );
	gl_FragColor.rgb = mix(gl_FragColor.rgb, blob.rgb, blob.a );
	gl_FragColor.a = 1.0;
}
