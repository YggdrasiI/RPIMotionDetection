uniform sampler2D numerals;
uniform vec2 border;
uniform vec4 scorePosLeft;
uniform vec4 scorePosRight;
const float waves = 2.0;
varying vec2 texcoord;
varying vec2 scorecoordLeft;
varying vec2 scorecoordRight;
void main(void) {
	float x = texcoord.x;
	float y = texcoord.y;
	if (y < 1.0 && y > 0.0 && x < 1.0 && x > 0.0) {
		vec4 ret = vec4(0.0);
		if( x < border.x || x > border.y ){
			ret.r = 1.0;
			ret.a = 0.2;
		}else{
			if( x < scorePosLeft.z && x > scorePosLeft.x && y < scorePosLeft.w && y > scorePosLeft.y ){
				ret += texture2D(numerals, scorecoordLeft);
				ret.a *= ret.a;
			}
			if( x < scorePosRight.z && x > scorePosRight.x && y < scorePosRight.w && y > scorePosRight.y ){
				ret += texture2D(numerals, scorecoordRight);
				ret.a *= ret.a;
			}
		}
		ret.a = min(ret.a,0.2);
		gl_FragColor = ret;
	}
	else {
		gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
	}
}
