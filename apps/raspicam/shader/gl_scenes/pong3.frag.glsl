#extension GL_OES_EGL_image_external : require
/* Use filter on texture tex */
uniform samplerExternalOES tex;
uniform sampler2D guitex;
uniform sampler2D blobstex;
varying vec2 texcoord;

vec4 rgb2hsv(vec4 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec4(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x, c.a);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main(void) 
{
	vec4 hsv = rgb2hsv( texture2D(tex,texcoord) );
	//Skin colors are nearby 0-1jump => shift values 
	//hsv.x = fract(hsv.x + 0.3);

	gl_FragColor.rgb = 0.5*(1.0 - hsv.bgr);

	vec4 gui = texture2D(guitex, texcoord);
	vec4 blob = texture2D(blobstex, -texcoord);
	gl_FragColor.rgb = mix(gl_FragColor.rgb, gui.rgb, gui.a );
	gl_FragColor.rgb = mix(gl_FragColor.rgb, blob.rgb, blob.a );
	gl_FragColor.a = 1.0;
}
